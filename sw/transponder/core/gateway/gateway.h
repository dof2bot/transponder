/**
 * @file gateway.h
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
 *       HOST PC (115,200 baud)                          ROBOT ARM ATOM (1,000,000 baud)
 *       +--------------------+                          +------------------------------+
 *       | idecobot / Python  |                          | M5Stack ATOM ESP32 / Servos  |
 *       +--------------------+                          +------------------------------+
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

#pragma once

#include "error_handler.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all gateway subsystems (UART0, UART1, Packet Filter, LED).
 *
 * Reports diagnostic error codes to error handler if any initialization step fails.
 *
 * @param eh Pointer to error_handler_t instance (may be NULL).
 * @return bool true on success, false if any subsystem initialization failed.
 */
bool gateway_init(error_handler_t *eh);

/**
 * @brief Process pending stream transfers in both directions (non-blocking).
 *
 * Call repeatedly inside the main event loop.
 */
void gateway_process(void);

#ifdef __cplusplus
}
#endif
