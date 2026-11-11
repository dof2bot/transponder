/**
 * @file error_types.h
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
 * @brief System error diagnostic codes, operating modes, and safety reactions.
 *
 * System Error Architecture:
 *
 *   +-------------------------------------------------------------+
 *   |                     error_types.h                           |
 *   |                                                             |
 *   |  system_mode_t       error_code_t       safety_reaction_t   |
 *   |  - NORMAL            - NONE             - NONE              |
 *   |  - DEGRADED          - INIT_LED         - LOG_ONLY          |
 *   |  - PANIC             - INIT_UART0       - ENTER_DEGRADED    |
 *   |                      - INIT_UART1       - PANIC_HALT        |
 *   |                      - INIT_FILTER      - SYSTEM_RESET      |
 *   |                      - RX/TX_OVERFLOW                       |
 *   |                      - HARDWARE_FAULT                       |
 *   +-------------------------------------------------------------+
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System operating modes indicating transponder health.
 */
typedef enum {
  SYSTEM_MODE_NORMAL = 0, /**< All peripherals nominal; bidirectional bridge active. */
  SYSTEM_MODE_DEGRADED,   /**< Non-fatal fault latched (e.g. ring buffer dropped byte). */
  SYSTEM_MODE_PANIC       /**< Fatal initialization or hardware fault; halt or reset. */
} system_mode_t;

/**
 * @brief Diagnostic error codes across transponder firmware.
 */
typedef enum {
  ERR_CODE_NONE = 0,
  /* Initialization errors */
  ERR_CODE_INIT_LED,
  ERR_CODE_INIT_UART0,
  ERR_CODE_INIT_UART1,
  ERR_CODE_INIT_FILTER,
  ERR_CODE_INIT_TIMER,
  /* Runtime stream errors */
  ERR_CODE_RX0_OVERFLOW,
  ERR_CODE_TX0_OVERFLOW,
  ERR_CODE_RX1_OVERFLOW,
  ERR_CODE_TX1_OVERFLOW,
  /* Fatal hardware errors */
  ERR_CODE_HARDWARE_FAULT
} error_code_t;

/**
 * @brief Safety reaction policy dispatched on error occurrence.
 */
typedef enum {
  REACTION_NONE = 0,
  REACTION_LOG_ONLY,
  REACTION_ENTER_DEGRADED,
  REACTION_PANIC_HALT,
  REACTION_SYSTEM_RESET
} safety_reaction_t;

#ifdef __cplusplus
}
#endif
