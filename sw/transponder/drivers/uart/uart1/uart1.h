/**
 * @file uart1.h
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
 * Hardware Data Flow & Ring Buffer Architecture:
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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USART1 buffer sizing limits.
 */
typedef enum {
  UART1_RX_BUFFER_SIZE = 64, /**< Receive ring buffer capacity in bytes. */
  UART1_TX_BUFFER_SIZE = 64  /**< Transmit ring buffer capacity in bytes. */
} uart1_buffer_limit_t;

/** Default baud rate for Robot Arm ATOM serial communication (1,000,000 bps). */
static const uint32_t UART1_DEFAULT_BAUD_RATE = 1000000UL;

/**
 * @brief Initialize USART1 hardware and internal ring buffers.
 *
 * Configures 1,000,000 8N1 communication using Double Speed Mode (U2X1 = 1, UBRR1 = 1).
 *
 * @param baud_rate Transmission speed in bps (defaults to 1,000,000).
 * @return bool true on success, false on error.
 */
bool uart1_init(uint32_t baud_rate);

/**
 * @brief Check if unread bytes are waiting in the receive ring buffer.
 *
 * @return bool true if at least one byte is available to read.
 */
bool uart1_available(void);

/**
 * @brief Read a single byte from the receive ring buffer.
 *
 * @return uint8_t Next unread byte, or 0 if buffer is empty.
 */
uint8_t uart1_read(void);

/**
 * @brief Non-blocking transmit byte over USART1 via TX ring buffer.
 *
 * Enqueues the byte in software buffer and enables UDRIE1 interrupt.
 *
 * @param byte Data byte to transmit.
 * @return bool true if enqueued successfully, false if buffer is full.
 */
bool uart1_write(uint8_t byte);

/**
 * @brief Transmit a contiguous buffer of bytes over USART1.
 *
 * @param buffer Pointer to data buffer.
 * @param length Number of bytes to transmit.
 * @return bool true if all bytes were enqueued successfully.
 */
bool uart1_write_buffer(const uint8_t *buffer, uint8_t length);

/**
 * @brief Flush and clear the receive ring buffer.
 */
void uart1_flush_rx(void);

#ifdef __cplusplus
}
#endif

