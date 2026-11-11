/**
 * @file uart0.c
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
 * @brief Hardware USART0 transceiver driver for ATmega2560 (Host PC connection).
 *
 * Implements non-blocking, interrupt-driven bidirectional serial communication
 * over ATmega2560 USART0 (PE0 RXD0, PE1 TXD0).
 *
 * Baud Rate Generator Formula (Asynchronous Double Speed Mode, U2X0 = 1):
 *
 *             f_CPU
 *   UBRR0 = -------------  -  1
 *           8 * BAUD_RATE
 *
 *   Calculation @ f_CPU = 16.000000 MHz for 115,200 bps:
 *     UBRR0 = (16,000,000 / (8 * 115,200)) - 1 = 17.36 - 1 = 16.36 -> 16 (-2.1% error)
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
 *                                  |  USART0_RX_vect Interrupt  |
 *                                  +----------------------------+
 *                                                |
 *                                        (Pushes byte into)
 *                                                v
 *  RX Pin (PE0) ---> [ UDR0 Shift Reg ] ---> [ s_rx_ring0 (64B) ] ---> uart0_read()
 *
 *  TX Pin (PE1) <--- [ UDR0 Shift Reg ] <--- [ s_tx_ring0 (64B) ] <--- uart0_write()
 *                                                ^
 *                                        (Pops byte from)
 *                                                |
 *                                  +----------------------------+
 *                                  | USART0_UDRE_vect Interrupt |
 *                                  +----------------------------+
 */

#include "uart0.h"
#include "ring_buffer.h"
#include <avr/interrupt.h>
#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* Ring buffer memory for USART0 RX FIFO and TX FIFO. */
static uint8_t s_rx_storage0[UART0_RX_BUFFER_SIZE];
static ring_buffer_t s_rx_ring0;
static uint8_t s_tx_storage0[UART0_TX_BUFFER_SIZE];
static ring_buffer_t s_tx_ring0;

/**
 * @brief USART0 Receive Complete Interrupt Service Routine.
 *
 * Pushes incoming data byte from hardware register UDR0 into the RX ring buffer.
 */
ISR(USART0_RX_vect) { (void)ring_buffer_push(&s_rx_ring0, UDR0); }

/**
 * @brief USART0 Data Register Empty Interrupt Service Routine.
 *
 * Pops next byte from TX ring buffer into hardware register UDR0.
 * Automatically clears UDRIE0 when buffer is empty to prevent interrupt storms.
 */
ISR(USART0_UDRE_vect) {
  uint8_t byte;

  if (ring_buffer_pop(&s_tx_ring0, &byte)) {
    UDR0 = byte;
  } else {
    UCSR0B &= ~(1 << UDRIE0);
  }
}

/**
 * @brief Initialize USART0 hardware and internal ring buffers.
 *
 * Configures 115200 8N1 communication using Double Speed Mode (U2X0 = 1).
 *
 * @param baud_rate Transmission speed in bps (defaults to 115200).
 * @return bool true on success, false on error.
 */
bool uart0_init(uint32_t baud_rate) {
  /* Use default baud rate if not specified. */
  if (baud_rate == 0) {
    baud_rate = UART0_DEFAULT_BAUD_RATE;
  }

  /* Initialize RX and TX ring buffers. */
  if (!ring_buffer_init(&s_rx_ring0, s_rx_storage0, UART0_RX_BUFFER_SIZE) ||
      !ring_buffer_init(&s_tx_ring0, s_tx_storage0, UART0_TX_BUFFER_SIZE)) {
    return false;
  }

  /* Calculate UBRR value for Double Speed Mode (U2X0 = 1). */
  uint16_t ubrr_val = (uint16_t)(((F_CPU / 8UL) / baud_rate) - 1UL);

  UBRR0H = (uint8_t)(ubrr_val >> 8);
  UBRR0L = (uint8_t)(ubrr_val & 0xFF);

  /* Enable Double Speed Mode. */
  UCSR0A = (1 << U2X0);

  /* Enable Receiver, Transmitter, and Receive Complete Interrupt. */
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

  /* Set frame format: 8 data bits, 1 stop bit, no parity (8N1). */
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

  return true;
}

/**
 * @brief Check if unread bytes are waiting in the receive ring buffer.
 *
 * @return bool true if at least one byte is available to read.
 */
bool uart0_available(void) { return !ring_buffer_is_empty(&s_rx_ring0); }

/**
 * @brief Read a single byte from the receive ring buffer.
 *
 * @return uint8_t Next unread byte, or 0 if buffer is empty.
 */
uint8_t uart0_read(void) {
  uint8_t byte = 0;

  (void)ring_buffer_pop(&s_rx_ring0, &byte);

  return byte;
}

/**
 * @brief Non-blocking transmit byte over USART0 via TX ring buffer.
 *
 * Enqueues the byte in software buffer and enables UDRIE0 interrupt.
 *
 * @param byte Data byte to transmit.
 * @return bool true if enqueued successfully, false if buffer is full.
 */
bool uart0_write(uint8_t byte) {
  /* Wait if TX buffer is full. Ensure UDRIE0 is enabled so the ISR drains it (prevents deadlock). */
  while (ring_buffer_is_full(&s_tx_ring0)) {

    /* Save global interrupt flag. */
    uint8_t sreg = SREG;

    /* Disable global interrupts. */
    cli();

    /* Enable UDRIE0 interrupt so background ISR drains the buffer. */
    UCSR0B |= (1 << UDRIE0);

    /* Restore global interrupt flag. */
    SREG = sreg;
  }

  /* Save global interrupt flag. */
  uint8_t sreg = SREG;

  /* Disable global interrupts. */
  cli();

  /* Push byte into ring buffer. */
  bool ok = ring_buffer_push(&s_tx_ring0, byte);

  /* Enable UDRIE0 interrupt. */
  UCSR0B |= (1 << UDRIE0);

  /* Restore global interrupt flag. */
  SREG = sreg;

  return ok;
}

/**
 * @brief Transmit a contiguous buffer of bytes over USART0.
 *
 * @param buffer Pointer to data buffer.
 * @param length Number of bytes to transmit.
 * @return bool true if all bytes were enqueued successfully.
 */
bool uart0_write_buffer(const uint8_t *buffer, uint8_t length) {
  if (buffer == (void *)0 || length == 0) {
    return false;
  }

  for (uint8_t i = 0; i < length; i++) {
    if (!uart0_write(buffer[i])) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Flush and clear the receive ring buffer.
 */
void uart0_flush_rx(void) { (void)ring_buffer_clear(&s_rx_ring0); }

