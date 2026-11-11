/**
 * @file error_handler.h
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
 * @brief System error handling, health monitoring, and safety reaction service.
 *
 * Tracks operating health modes (NORMAL, DEGRADED, PANIC), aggregates error counts,
 * latches latest diagnostic codes, and dispatches safety reactions.
 *
 * System Mode State Machine:
 *
 *     +-----------------------+
 *     |  SYSTEM_MODE_NORMAL   |<-------------------+
 *     +-----------+-----------+                    |
 *                 |                                | error_handler_clear()
 *                 | Buffer Overflow / Dropped Byte |
 *                 v                                |
 *     +-----------------------+                    |
 *     | SYSTEM_MODE_DEGRADED  |--------------------+
 *     +-----------+-----------+
 *                 |
 *                 | Init Fault / Fatal Hardware Error
 *                 v
 *     +-----------------------+
 *     |   SYSTEM_MODE_PANIC   |----> LED Diagnostic Blink & Safe Halt
 *     +-----------------------+
 *
 * Safety Policy Matrix:
 *  +-----------------------------+-----------------------+--------------------------------------+
 *  | Error Code                  | Transitioned Mode     | Safety Reaction Dispatched           |
 *  +-----------------------------+-----------------------+--------------------------------------+
 *  | ERR_CODE_INIT_LED           | SYSTEM_MODE_DEGRADED  | REACTION_ENTER_DEGRADED              |
 *  | ERR_CODE_INIT_UART0         | SYSTEM_MODE_PANIC     | REACTION_PANIC_HALT                  |
 *  | ERR_CODE_INIT_UART1         | SYSTEM_MODE_PANIC     | REACTION_PANIC_HALT                  |
 *  | ERR_CODE_INIT_FILTER        | SYSTEM_MODE_PANIC     | REACTION_PANIC_HALT                  |
 *  | ERR_CODE_RX0/TX0_OVERFLOW   | SYSTEM_MODE_DEGRADED  | REACTION_LOG_ONLY                    |
 *  | ERR_CODE_RX1/TX1_OVERFLOW   | SYSTEM_MODE_DEGRADED  | REACTION_LOG_ONLY                    |
 *  | ERR_CODE_HARDWARE_FAULT     | SYSTEM_MODE_PANIC     | REACTION_SYSTEM_RESET                |
 *  +-----------------------------+-----------------------+--------------------------------------+
 */

#pragma once

#include "error_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error handler state tracking structure.
 */
typedef struct {
  system_mode_t current_mode; /**< Active health mode (NORMAL, DEGRADED, PANIC). */
  error_code_t last_error;    /**< Most recently recorded error code. */
  uint16_t error_count;       /**< Cumulative error counter. */
} error_handler_t;

/**
 * @brief Initialize error handler component in normal operating mode.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return bool true on success, false if eh is NULL.
 */
bool error_handler_init(error_handler_t *eh);

/**
 * @brief Report a system error, update state, and return recommended safety reaction.
 *
 * @param eh Pointer to error_handler_t instance.
 * @param code Error diagnostic code.
 * @return safety_reaction_t Recommended safety action.
 */
safety_reaction_t error_handler_report(error_handler_t *eh, error_code_t code);

/**
 * @brief Trigger a panic condition with diagnostic optical pattern on LED.
 *
 * Enters an observable diagnostic pulse loop signaling the error code on LED.
 *
 * @param eh Pointer to error_handler_t instance.
 * @param code Fatal error code causing the panic.
 */
void error_handler_panic(error_handler_t *eh, error_code_t code);

/**
 * @brief Get current system operational mode.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return system_mode_t Current mode (NORMAL, DEGRADED, PANIC).
 */
system_mode_t error_handler_get_mode(const error_handler_t *eh);

/**
 * @brief Get last recorded error code.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return error_code_t Last error code.
 */
error_code_t error_handler_get_last_error(const error_handler_t *eh);

/**
 * @brief Get total recorded error count.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return uint16_t Total number of reported errors.
 */
uint16_t error_handler_get_error_count(const error_handler_t *eh);

/**
 * @brief Clear last recorded error and restore normal operating mode.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return bool true on success.
 */
bool error_handler_clear(error_handler_t *eh);

/**
 * @brief Convert system mode enum to human-readable ASCII string (stored in Flash).
 *
 * @param mode System mode enum.
 * @return const char* PROGMEM string pointer.
 */
const char *error_handler_mode_to_str(system_mode_t mode);

/**
 * @brief Convert error code enum to human-readable ASCII string (stored in Flash).
 *
 * @param code Error code enum.
 * @return const char* PROGMEM string pointer.
 */
const char *error_handler_code_to_str(error_code_t code);

#ifdef __cplusplus
}
#endif
