/**
 * @file uart1.c
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
 * @brief Hardware USART1 transceiver driver for ATmega2560 (Robot Arm ATOM connection).
 *
 * Implements non-blocking, interrupt-driven bidirectional serial communication
 * over ATmega2560 USART1 (PD2 RXD1, PD3 TXD1).
 *
 * Baud Rate Generator Formula (Asynchronous Double Speed Mode, U2X1 = 1):
 *
 *             f_CPU
 *   UBRR1 = -------------  -  1
 *           8 * BAUD_RATE
 *
 *   Calculation @ f_CPU = 16.000000 MHz for 1,000,000 bps:
 *     UBRR1 = (16,000,000 / (8 * 1,000,000)) - 1 = 2 - 1 = 1 (Exact 0.00% error)
 *
 * 8N1 Frame Format (1 Start, 8 Data, No Parity, 1 Stop):
 *
 *  IDLE   START   D0    D1    D2    D3    D4    D5    D6    D7   STOP   IDLE
 *  -----+       +-----+-----+-----+-----+-----+-----+-----+-----+-----+------
 *       |       | LSB |  .  |  .  |  .  |  .  |  .  |  .  | MSB |     |
 *       +-------+-----+-----+-----+-----+-----+-----+-----+-----+-----+
 *       |<-1b ->|<------------------ 8 Data Bits -------------->|<-1b->|
 *       | Start |                                               | Stop |
 *
 * Hardware Data Flow & Ring Buffer Topology:
 *
 *                                  +----------------------------+
 *                                  |  USART1_RX_vect Interrupt  |
 *                                  +----------------------------+
 *                                                |
 *                                        (Pushes byte into)
 *                                                v
 *  RX1 Pin (PD2) ---> [ UDR1 Shift Reg ] ---> [ s_rx_ring1 (64B) ] ---> uart1_read()
 *
 *  TX1 Pin (PD3) <--- [ UDR1 Shift Reg ] <--- [ s_tx_ring1 (64B) ] <--- uart1_write()
 *                                                ^
 *                                        (Pops byte from)
 *                                                |
 *                                  +----------------------------+
 *                                  | USART1_UDRE_vect Interrupt |
 *                                  +----------------------------+
 */

#include "uart1.h"
#include "ring_buffer.h"
#include <avr/interrupt.h>
#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* Ring buffer memory for USART1 RX FIFO and TX FIFO. */
static uint8_t s_rx_storage1[UART1_RX_BUFFER_SIZE];
static ring_buffer_t s_rx_ring1;
static uint8_t s_tx_storage1[UART1_TX_BUFFER_SIZE];
static ring_buffer_t s_tx_ring1;

/**
 * @brief USART1 Receive Complete Interrupt Service Routine.
 *
 * Pushes incoming data byte from hardware register UDR1 into the RX ring buffer.
 */
ISR(USART1_RX_vect) { (void)ring_buffer_push(&s_rx_ring1, UDR1); }

/**
 * @brief USART1 Data Register Empty Interrupt Service Routine.
 *
 * Pops next byte from TX ring buffer into hardware register UDR1.
 * Automatically clears UDRIE1 when buffer is empty to prevent interrupt storms.
 */
ISR(USART1_UDRE_vect) {
  uint8_t byte;

  if (ring_buffer_pop(&s_tx_ring1, &byte)) {
    UDR1 = byte;
  } else {
    UCSR1B &= ~(1 << UDRIE1);
  }
}

/**
 * @brief Initialize USART1 hardware and internal ring buffers.
 *
 * Configures 1,000,000 8N1 communication using Double Speed Mode (U2X1 = 1, UBRR1 = 1).
 *
 * @param baud_rate Transmission speed in bps (defaults to 1,000,000).
 * @return bool true on success, false on error.
 */
bool uart1_init(uint32_t baud_rate) {
  /* Use default baud rate if not specified. */
  if (baud_rate == 0) {
    baud_rate = UART1_DEFAULT_BAUD_RATE;
  }

  /* Initialize RX and TX ring buffers. */
  if (!ring_buffer_init(&s_rx_ring1, s_rx_storage1, UART1_RX_BUFFER_SIZE) ||
      !ring_buffer_init(&s_tx_ring1, s_tx_storage1, UART1_TX_BUFFER_SIZE)) {
    return false;
  }

  /* Enable Double Speed Mode (U2X1 = 1) before setting baud rate. */
  UCSR1A = (1 << U2X1);

  /* Calculate UBRR value for Double Speed Mode (U2X1 = 1). */
  uint16_t ubrr_val = (uint16_t)(((F_CPU / 8UL) / baud_rate) - 1UL);

  UBRR1H = (uint8_t)(ubrr_val >> 8);
  UBRR1L = (uint8_t)(ubrr_val & 0xFF);

  /* Set frame format: 8 data bits, 1 stop bit, no parity (8N1). */
  UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);

  /* Enable Receiver, Transmitter, and Receive Complete Interrupt. */
  UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);

  return true;
}

/**
 * @brief Check if unread bytes are waiting in the receive ring buffer.
 *
 * @return bool true if at least one byte is available to read.
 */
bool uart1_available(void) { return !ring_buffer_is_empty(&s_rx_ring1); }

/**
 * @brief Read a single byte from the receive ring buffer.
 *
 * @return uint8_t Next unread byte, or 0 if buffer is empty.
 */
uint8_t uart1_read(void) {
  uint8_t byte = 0;

  (void)ring_buffer_pop(&s_rx_ring1, &byte);

  return byte;
}

/**
 * @brief Non-blocking transmit byte over USART1 via TX ring buffer.
 *
 * Enqueues the byte in software buffer and enables UDRIE1 interrupt.
 *
 * @param byte Data byte to transmit.
 * @return bool true if enqueued successfully, false if buffer is full.
 */
bool uart1_write(uint8_t byte) {
  /* Wait if TX buffer is full. Ensure UDRIE1 is enabled so the ISR drains it (prevents deadlock). */
  while (ring_buffer_is_full(&s_tx_ring1)) {
    /* Save global interrupt flag. */
    uint8_t sreg = SREG;

    /* Disable global interrupts. */
    cli();

    /* Enable UDRIE1 interrupt so background ISR drains the buffer. */
    UCSR1B |= (1 << UDRIE1);

    /* Restore global interrupt flag. */
    SREG = sreg;
  }

  /* Save global interrupt flag. */
  uint8_t sreg = SREG;

  /* Disable global interrupts. */
  cli();

  /* Push the byte into the transmit ring buffer. */
  bool ok = ring_buffer_push(&s_tx_ring1, byte);

  /* Enable UDRIE1 interrupt. */
  UCSR1B |= (1 << UDRIE1);

  /* Restore global interrupt flag. */
  SREG = sreg;

  return ok;
}

/**
 * @brief Transmit a contiguous buffer of bytes over USART1.
 *
 * @param buffer Pointer to data buffer.
 * @param length Number of bytes to transmit.
 * @return bool true if all bytes were enqueued successfully.
 */
bool uart1_write_buffer(const uint8_t *buffer, uint8_t length) {
  /* Check for invalid buffer pointer or zero length. */
  if (buffer == (void *)0 || length == 0) {
    return false;
  }

  /* Iterate over each byte in the buffer and enqueue it. */
  for (uint8_t i = 0; i < length; i++) {
    if (!uart1_write(buffer[i])) {
      return false;
    }
  }

  /* Return true if all bytes were enqueued successfully. */
  return true;
}

/**
 * @brief Flush and clear the receive ring buffer.
 */
void uart1_flush_rx(void) { (void)ring_buffer_clear(&s_rx_ring1); }

