/**
 * @file gateway.c
 * Copyright (C) 2026 Vladimir Roncevic <elektron.ronca@gmail.com>
 *
 * transponder is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * transponder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program_name. If not, see <http://www.gnu.org/licenses/>.
 *
 * @brief Bidirectional gateway connecting Host PC (115200) and Robot Arm (1000000).
 *
 * Comprehensive System Block Scheme & 4-Buffer Architecture:
 *
 *   =============================================================================
 *                         TRANSPONDER FIRMWARE ARCHITECTURE
 *   =============================================================================
 *
 *    HOST PC (115,200 baud)                              ROBOT ARM ATOM (1,000,000 baud)
 *   +--------------------+                              +------------------------------+
 *   | idecobot / Python  |                              | M5Stack ATOM ESP32 / Servos  |
 *   +--------------------+                              +------------------------------+
 *            |        ^                                             ^        |
 *      (TX)  |        |  (RX)                                 (RX1) |        | (TX1)
 *            v        |                                             |        v
 *       [ PE0 / RXD0 ]|                                      [ PD3 / TXD1 ]  |[ PD2 / RXD1 ]
 *            |        |                                             ^        |       |
 *            |        |                                             |        |       |
 *      (USART0_RX_vect) (USART0_UDRE_vect)             (USART1_UDRE_vect)  (USART1_RX_vect)
 *            |        |                                             |        |       |
 *            v        |                                             |        |       v
 *      +------------+ +------------+                   +------------+        |+------------+
 *      | s_rx_ring0 | | s_tx_ring0 |                   | s_tx_ring1 |        || s_rx_ring1 |
 *      |  (Buffer1) | |  (Buffer2) |                   |  (Buffer3) |        ||  (Buffer4) |
 *      | [PC Input] | |[PC Output] |                   | [Arm Input]|        ||[Arm Output]|
 *      +------------+ +------------+                   +------------+        |+------------+
 *            |              ^                                 ^                      |
 *            |              |                                 |                      |
 *            |  (Verified)  |                                 |                      v
 *            |              +-------- [ packet_filter ] <-----+                      |
 *            |                        (Discards 0xFF chatter) |                      |
 *            |                        (Validates 0xFE..0xFA)  |                      |
 *            |                        (Timeout Reset: 25ms) <-+-- [ timer (1ms) ]    |
 *            |                                                |                      |
 *            +------------------------------------------------+                      |
 *                       (Direct Forward: PC -> Arm)                                  |
 *                                                                                    |
 *                                          [ led (PB7 / Pin 13) ] <------------------+
 *                                          (Heartbeat & Activity Blink)
 *   =============================================================================
 */

#include "gateway.h"
#include "error_handler.h"
#include "led.h"
#include "packet_filter.h"
#include "timer.h"
#include "uart0.h"
#include "uart1.h"

/**
 * @brief Maximum silence duration between uplink bytes before incomplete frames are reset.
 */
static const uint32_t GATEWAY_UPLINK_TIMEOUT_MS = 500UL;

/* Static packet filter instance for filtering MyCobot frames. */
static packet_filter_t s_filter;

/* Timestamp of last received uplink byte for timeout detection. */
static uint32_t s_last_uplink_byte_ms = 0;

/**
 * @brief Callback invoked when packet filter completes a verified MyCobot frame.
 *
 * Forwards the verified frame to the Host PC via USART0 TX ring buffer and toggles
 * the status LED to indicate active bidirectional communication.
 *
 * @param frame Pointer to verified contiguous frame bytes (0xFE 0xFE <len> ... 0xFA).
 * @param length Total length of verified frame in bytes.
 */
static void on_mycobot_frame_verified(const uint8_t *frame, uint8_t length) {
  (void)uart0_write_buffer(frame, length);

  led_toggle();
}

/**
 * @brief Forward incoming stream from Host PC (USART0) to Robot Arm (USART1).
 */
static void gateway_process_downlink(void) {
  while (uart0_available()) {
    uint8_t byte = uart0_read();

    (void)uart1_write(byte);
    led_toggle();
  }
}

/**
 * @brief Stream and filter incoming bytes from Robot Arm (USART1).
 */
static void gateway_process_uplink(void) {
  while (uart1_available()) {
    uint8_t byte = uart1_read();

    s_last_uplink_byte_ms = timer_millis();
    packet_filter_feed(&s_filter, byte);
  }
}

/**
 * @brief Evaluate inter-byte timeout and reset filter if transmission stalled.
 */
static void gateway_check_uplink_timeout(void) {
  if (!packet_filter_is_idle(&s_filter)) {
    uint32_t now = timer_millis();

    if ((now - s_last_uplink_byte_ms) >= GATEWAY_UPLINK_TIMEOUT_MS) {
      packet_filter_reset(&s_filter);
    }
  }
}

/**
 * @brief Initialize all gateway subsystems (Timer, UART0, UART1, Packet Filter, LED).
 *
 * Reports diagnostic error codes to error handler if any initialization step fails.
 *
 * @param eh Pointer to error_handler_t instance (may be NULL).
 * @return bool true on success, false if any subsystem initialization failed.
 */
bool gateway_init(error_handler_t *eh) {
  led_init();

  if (!timer_init()) {
    if (eh != (void *)0) {
      (void)error_handler_report(eh, ERR_CODE_INIT_TIMER);
    }

    return false;
  }

  if (!uart0_init(UART0_DEFAULT_BAUD_RATE)) {
    if (eh != (void *)0) {
      (void)error_handler_report(eh, ERR_CODE_INIT_UART0);
    }

    return false;
  }

  if (!uart1_init(UART1_DEFAULT_BAUD_RATE)) {
    if (eh != (void *)0) {
      (void)error_handler_report(eh, ERR_CODE_INIT_UART1);
    }

    return false;
  }

  if (!packet_filter_init(&s_filter, on_mycobot_frame_verified)) {
    if (eh != (void *)0) {
      (void)error_handler_report(eh, ERR_CODE_INIT_FILTER);
    }

    return false;
  }

  /* Initial state: LED OFF so any transfer blinks visibly. */
  led_off();

  return true;
}

/**
 * @brief Process pending stream transfers in both directions (non-blocking).
 *
 * Dispatches downlink and uplink tasks. Timeout check disabled for baseline isolation.
 */
void gateway_process(void) {
  gateway_process_downlink();
  gateway_process_uplink();
  /* gateway_check_uplink_timeout(); */
}
