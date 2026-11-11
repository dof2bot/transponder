<p align="center">
  <img src="docs/transponder_logo.png" alt="Transponder Logo" width="180"/>
</p>

# Transponder Firmware

> **High-Performance Serial Protocol Bridge & Packet Filter Firmware for ATmega2560**  
> Bridging **idecobot** (Host PC) with **mycobot** serial bus servo motors.

---

## 📌 Overview

**Transponder** is a dedicated real-time embedded bridge firmware designed for the **Microchip ATmega2560** microcontroller (Arduino Mega 2560). It acts as an intelligent communication gateway between the **idecobot** control environment running on a Host PC and the serial bus servo network of the **Elephant Robotics MyCobot** robotic arm.

### Key Capabilities

- **Baud Rate Translation**: Bridges standard Host PC communication at **115,200 baud** with high-speed robot arm serial communication at **1,000,000 baud** (1 Mbps).
- **Asymmetric Hardware Decoupling**: Uses a **4-ring-buffer FIFO architecture** (64 bytes each) with non-blocking, interrupt-driven USART transceivers to prevent buffer overrun and register tearing.
- **Intelligent Uplink Packet Filtering**: Integrates a deterministic finite-state machine (FSM) that forwards valid Elephant Robotics frames (`0xFE 0xFE ... 0xFA`) while filtering out and discarding raw Feetech servo bus chatter (`0xFF 0xFF ...`).
- **Precision CTC System Timebase**: Configures 16-bit **Timer1** in Clear Timer on Compare Match (CTC) mode for atomic, rollover-safe millisecond tracking and stream timeout resets.
- **Optical Diagnostic Failsafe**: Features a multi-mode error handler with optical LED pulse signaling on hardware faults.

---

## 🏗️ System Architecture

### Hardware Topology

```mermaid
flowchart LR
    subgraph HostPC["Host PC"]
        IDE["idecobot (Python / UI)"]
    end

    subgraph TransponderMCU["ATmega2560 (Transponder Gateway)"]
        direction TB
        USART0["USART0 Transceiver<br/>(PE0 RXD0 / PE1 TXD0)"]
        FSM["Packet Filter &amp; Gateway Engine"]
        USART1["USART1 Transceiver<br/>(PD2 RXD1 / PD3 TXD1)"]
        LED["Status LED<br/>(PB7 / Pin 13)"]
        TIMER["Timer1 (1ms CTC Tick)"]
    end

    subgraph RobotArm["MyCobot Robot Arm"]
        ATOM["M5Stack ATOM ESP32"]
        BUS["Serial Bus Servos (Feetech)"]
    end

    IDE <-->|"USB Serial (115,200 baud 8N1)"| USART0
    USART0 <--> FSM
    FSM <--> USART1
    USART1 <-->|"Half-Duplex / Serial Bus (1,000,000 baud 8N1)"| ATOM
    ATOM <--> BUS
    FSM -.-> LED
    TIMER -.-> FSM
```

---

## 🔄 Data Flow & 4-Buffer FIFO Architecture

The firmware decouples serial reception and transmission from superloop processing using four 64-byte FIFO ring buffers managed by interrupt service routines (`ISR`):

```mermaid
flowchart TD
    subgraph HostPC["Host PC (115,200 baud)"]
        PC_APP["idecobot"]
    end

    subgraph USART0_Subsystem["USART0 Subsystem"]
        RX0_ISR["USART0_RX_vect"]
        TX0_ISR["USART0_UDRE_vect"]
        BUF1[("Buffer 1: s_rx_ring0<br/>(64 Bytes - PC Input)")]
        BUF2[("Buffer 2: s_tx_ring0<br/>(64 Bytes - PC Output)")]
    end

    subgraph CoreEngine["Gateway Core Engine"]
        DOWNLINK["Downlink Task<br/>(Direct Forwarding)"]
        UPLINK["Uplink Task<br/>(Stateful Stream Processing)"]
        FILTER{"Packet Filter FSM"}
        TIMER1["Timer1 Millis (1ms)"]
        LED_OUT["LED Toggle (Activity Feedback)"]
    end

    subgraph USART1_Subsystem["USART1 Subsystem"]
        BUF3[("Buffer 3: s_tx_ring1<br/>(64 Bytes - Arm Input)")]
        BUF4[("Buffer 4: s_rx_ring1<br/>(64 Bytes - Arm Output)")]
        TX1_ISR["USART1_UDRE_vect"]
        RX1_ISR["USART1_RX_vect"]
    end

    subgraph RobotArm["Robot Arm (1,000,000 baud)"]
        ARM_HW["MyCobot ATOM &amp; Servos"]
    end

    %% Downlink stream
    PC_APP -->|"TXD0 (PE0)"| RX0_ISR
    RX0_ISR --> BUF1
    BUF1 --> DOWNLINK
    DOWNLINK --> BUF3
    BUF3 --> TX1_ISR
    TX1_ISR -->|"TXD1 (PD3)"| ARM_HW

    %% Uplink stream
    ARM_HW -->|"RXD1 (PD2)"| RX1_ISR
    RX1_ISR --> BUF4
    BUF4 --> UPLINK
    UPLINK --> FILTER
    FILTER -->|"Verified 0xFE..0xFA Frame"| BUF2
    FILTER -.->|"Discard 0xFF Chatter"| TRASH["Discarded"]
    BUF2 --> TX0_ISR
    TX0_ISR -->|"RXD0 (PE1)"| PC_APP

    %% Timing and LED feedback
    TIMER1 -.->|"Silence Timeout Reset"| FILTER
    DOWNLINK -.-> LED_OUT
    FILTER -.->|"On Verified Frame"| LED_OUT
```

---

## 🔍 Packet Filter Finite-State Machine (FSM)

The uplink stream from the robotic arm carries both high-level Elephant Robotics responses and low-level servo bus chatter. The **Packet Filter** inspects incoming bytes in $O(1)$ constant time per byte:

```mermaid
stateDiagram-v2
    [*] --> FILTER_STATE_IDLE

    FILTER_STATE_IDLE --> FILTER_STATE_MYCOBOT_HEADER_2: Byte == 0xFE
    FILTER_STATE_IDLE --> FILTER_STATE_SERVO_HEADER_2: Byte == 0xFF
    FILTER_STATE_IDLE --> FILTER_STATE_IDLE: Other byte (ignored)

    %% MyCobot Frame Path
    FILTER_STATE_MYCOBOT_HEADER_2 --> FILTER_STATE_MYCOBOT_LEN: Byte == 0xFE
    FILTER_STATE_MYCOBOT_HEADER_2 --> FILTER_STATE_SERVO_HEADER_2: Byte == 0xFF
    FILTER_STATE_MYCOBOT_HEADER_2 --> FILTER_STATE_IDLE: Invalid

    FILTER_STATE_MYCOBOT_LEN --> FILTER_STATE_MYCOBOT_LEN: Extra 0xFE
    FILTER_STATE_MYCOBOT_LEN --> FILTER_STATE_MYCOBOT_PAYLOAD: Valid Length (1 <= len <= 61)
    FILTER_STATE_MYCOBOT_LEN --> FILTER_STATE_IDLE: Invalid Length

    FILTER_STATE_MYCOBOT_PAYLOAD --> FILTER_STATE_MYCOBOT_PAYLOAD: remaining_bytes > 1
    FILTER_STATE_MYCOBOT_PAYLOAD --> FILTER_STATE_IDLE: remaining_bytes == 0 &amp;&amp; Byte == 0xFA (Emit verified frame)
    FILTER_STATE_MYCOBOT_PAYLOAD --> FILTER_STATE_IDLE: remaining_bytes == 0 &amp;&amp; Byte != 0xFA (Checksum/Footer mismatch)

    %% Feetech Servo Bus Discard Path
    FILTER_STATE_SERVO_HEADER_2 --> FILTER_STATE_SERVO_ID: Byte == 0xFF
    FILTER_STATE_SERVO_HEADER_2 --> FILTER_STATE_MYCOBOT_HEADER_2: Byte == 0xFE
    FILTER_STATE_SERVO_HEADER_2 --> FILTER_STATE_IDLE: Invalid

    FILTER_STATE_SERVO_ID --> FILTER_STATE_SERVO_LEN: Any Servo ID Byte

    FILTER_STATE_SERVO_LEN --> FILTER_STATE_SERVO_DISCARD: Valid Length (1 <= len <= 64)
    FILTER_STATE_SERVO_LEN --> FILTER_STATE_IDLE: Invalid Length

    FILTER_STATE_SERVO_DISCARD --> FILTER_STATE_SERVO_DISCARD: remaining_bytes > 1
    FILTER_STATE_SERVO_DISCARD --> FILTER_STATE_IDLE: remaining_bytes == 0 (Packet discarded)
```

### Protocol Frame Formats

#### 1. Elephant Robotics Frame *(Verified & Forwarded to Host PC)*

```mermaid
flowchart LR
    subgraph Header["Header (2 Bytes)"]
        direction LR
        H1["0xFE (H1)"] --> H2["0xFE (H2)"]
    end
    subgraph LengthField["Length (1 Byte)"]
        LEN["Length (len)"]
    end
    subgraph PayloadField["Payload (len Bytes)"]
        direction LR
        CMD["Command / Data"] --> DATA["... Data (N-1) ..."]
    end
    subgraph Footer["Footer (1 Byte)"]
        END["0xFA (Footer)"]
    end

    Header --> LengthField --> PayloadField --> Footer
```

#### 2. Feetech Servo Bus Frame *(Filtered out & Discarded)*

```mermaid
flowchart LR
    subgraph ServoHeader["Header (2 Bytes)"]
        direction LR
        SH1["0xFF (H1)"] --> SH2["0xFF (H2)"]
    end
    subgraph ServoIdField["Servo ID (1 Byte)"]
        SID["Servo ID"]
    end
    subgraph ServoLenField["Length (1 Byte)"]
        SLEN["Length (L)"]
    end
    subgraph ServoPayload["Payload (L Bytes)"]
        direction LR
        INSTR["Instruction"] --> PARAMS["... Parameters ..."]
    end
    subgraph ChecksumField["Checksum (1 Byte)"]
        CHK["Checksum"]
    end

    ServoHeader --> ServoIdField --> ServoLenField --> ServoPayload --> ChecksumField
```

---

## ⏱️ Communication Sequence

```mermaid
sequenceDiagram
    autonumber
    participant PC as Host PC (idecobot)
    participant USART0 as USART0 (RingBuffer)
    participant GW as Gateway Superloop
    participant FSM as Packet Filter FSM
    participant USART1 as USART1 (RingBuffer)
    participant Arm as Robot Arm (MyCobot)

    Note over PC, Arm: Downlink Transmission (PC -> Arm)
    PC->>USART0: Transmit command bytes (115,200 baud)
    USART0->>GW: uart0_read()
    GW->>USART1: uart1_write() [non-blocking]
    USART1->>Arm: Transmit over serial bus (1,000,000 baud)

    Note over PC, Arm: Uplink Transmission & Filtering (Arm -> PC)
    Arm->>USART1: Stream telemetry & servo responses
    USART1->>GW: uart1_read()
    GW->>FSM: packet_filter_feed(byte)

    alt Raw Servo Chatter (0xFF 0xFF ...)
        FSM->>FSM: Parse header, ID, length and discard
    else Verified MyCobot Frame (0xFE 0xFE ... 0xFA)
        FSM->>GW: on_mycobot_frame_verified(frame, len)
        GW->>USART0: uart0_write_buffer(frame, len)
        USART0->>PC: Forward verified frame (115,200 baud)
    end
```

---

## 📂 Repository Structure

```
.
├── .clang-format               # LLVM-based code formatting configuration
├── .clangd                     # Clangd language server flags & relative include paths
├── docs/                       # Project documentation assets
├── README.md                   # Project documentation
├── transponder.code-workspace  # VS Code / Antigravity IDE workspace setup
└── sw/
    └── transponder/
        ├── main.c              # Firmware entry point & real-time superloop
        ├── build/              # Modular Makefile build system
        │   ├── Makefile        # Top-level build coordination
        │   ├── cflags.mk       # C compiler flags & include paths
        │   ├── subdir.mk       # Source lists & object build rules
        │   ├── tools.mk        # AVR toolchain executable definitions
        │   └── ...
        ├── core/               # Platform-agnostic application core
        │   ├── error_handler/  # Health monitoring, panic modes & optical blink codes
        │   ├── gateway/        # Bidirectional streaming & scheduling engine
        │   ├── interfaces/     # Architecture compatibility headers (progmem_compat.h)
        │   ├── packet_filter/  # Stateful protocol frame parser & chatter filter
        │   ├── protocol/       # Frame protocol definitions (0xFE, 0xFA, 0xFF)
        │   └── ring_buffer/    # Lock-free FIFO circular ring buffer implementation
        └── drivers/            # Hardware-specific peripheral abstraction (AVR)
            ├── led/            # PB7 (Pin 13) digital status LED driver
            ├── timer/          # 16-bit Timer1 CTC 1ms millisecond timebase
            └── uart/           # Hardware USART transceivers
                ├── uart0/      # USART0 transceiver (Host PC @ 115,200 baud)
                └── uart1/      # USART1 transceiver (Robot Arm @ 1,000,000 baud)
```

---

## ⚡ Hardware & Pinout Specifications

| Peripheral | MCU Pin | Arduino Pin | Configuration | Function |
| :--- | :--- | :--- | :--- | :--- |
| **USART0 RX** | `PE0` | Pin 0 (RX0) | Input, Interrupt-driven | Host PC Downlink Stream |
| **USART0 TX** | `PE1` | Pin 1 (TX0) | Output, Interrupt-driven | Host PC Uplink Verified Frames |
| **USART1 RX** | `PD2` | Pin 19 (RX1)| Input, Interrupt-driven | Robot Arm Raw Uplink Stream |
| **USART1 TX** | `PD3` | Pin 18 (TX1)| Output, Interrupt-driven | Robot Arm Downlink Forwarding |
| **Status LED** | `PB7` | Pin 13 | Digital Output | Heartbeat, Activity & Error Panic Pulses |
| **Timer1** | Internal | N/A | CTC Mode (OCR1A = 249) | 1.0 ms System Tick Generation |

### Baud Rate Calculations ($f_{CPU} = 16.000\text{ MHz}$)

Both USART peripherals operate in **Double Speed Asynchronous Mode** ($U2Xn = 1$):

$$\text{UBRR} = \left\lfloor \frac{f_{CPU}}{8 \times \text{BAUD}} - 1 + 0.5 \right\rfloor$$

- **USART0 (115,200 bps)**:
  $$\text{UBRR0} = \frac{16,000,000}{8 \times 115,200} - 1 = 17.36 - 1 \approx 16 \quad (\text{Error: } -2.12\%)$$
- **USART1 (1,000,000 bps)**:
  $$\text{UBRR1} = \frac{16,000,000}{8 \times 1,000,000} - 1 = 2 - 1 = 1 \quad (\text{Error: } \mathbf{0.00\%})$$

---

## 🚨 Error Handling & Optical Panic Codes

When fatal initialization errors occur, the system transitions to `SYSTEM_MODE_PANIC` and drives the board LED in a repeating optical diagnostic pulse pattern (blink count corresponding to the error code, followed by a 1-second pause):

| Error Code | Identifier | Description | Blink Count |
| :---: | :--- | :--- | :---: |
| `0x01` | `ERR_CODE_INIT_LED` | LED GPIO initialization failure | 1 |
| `0x02` | `ERR_CODE_INIT_UART0`| USART0 ring buffer / register init fault | 2 |
| `0x03` | `ERR_CODE_INIT_UART1`| USART1 ring buffer / register init fault | 3 |
| `0x04` | `ERR_CODE_INIT_FILTER`| Packet filter FSM init fault | 4 |
| `0x05` | `ERR_CODE_INIT_TIMER` | Timer1 CTC configuration failure | 5 |
| `0x0A` | `ERR_CODE_HARDWARE_FAULT` | Critical unrecoverable hardware fault | 10 |

---

## 🛠️ Build & Flash Instructions

### Prerequisites

Install the standard AVR development toolchain:

```bash
sudo apt-get update
sudo apt-get install -y gcc-avr avr-libc binutils-avr make avrdude
```

### Compiling

Navigate to the build directory and run `make`:

```bash
cd sw/transponder/build
make clean && make all
```

This produces:
- `transponder.elf`: Compiled ELF binary with debug symbols.
- `transponder.hex`: Intel HEX binary image ready for flash programming.
- `transponder.lss`: Extended assembly disassembly listing.
- `transponder.map`: Memory map detailing Flash and SRAM section allocation.

### Flashing to Target Board

Connect the Arduino Mega 2560 via USB and program using `avrdude`:

```bash
avrdude -v -p atmega2560 -c wiring -P /dev/ttyACM0 -b 115200 -D -U flash:w:transponder.hex:i
```

*(Replace `/dev/ttyACM0` with `/dev/ttyUSB0` depending on your serial interface).*

---

## 📄 License

Transponder is free software: you can redistribute it and/or modify it under the terms of the **GNU General Public License** as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Copyright (C) 2026 Vladimir Roncevic `<elektron.ronca@gmail.com>`.

