/**
 * @file error_handler.c
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
 * State Machine Architecture & Panic Visual Diagnostic Pattern:
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
 *     |   SYSTEM_MODE_PANIC   |----> Optical LED Pulse Pattern (N blinks + pause)
 *     +-----------------------+
 */

#include "error_handler.h"
#include "led.h"
#include "progmem_compat.h"
#include <util/delay.h>

bool error_handler_init(error_handler_t *eh) {
  if (eh == (void *)0) {
    return false;
  }

  eh->current_mode = SYSTEM_MODE_NORMAL;
  eh->last_error = ERR_CODE_NONE;
  eh->error_count = 0;

  return true;
}

safety_reaction_t error_handler_report(error_handler_t *eh, error_code_t code) {
  if (eh == (void *)0) {
    return REACTION_NONE;
  }

  eh->last_error = code;
  eh->error_count++;

  switch (code) {
  case ERR_CODE_INIT_LED:
    eh->current_mode = SYSTEM_MODE_DEGRADED;
    return REACTION_ENTER_DEGRADED;

  case ERR_CODE_INIT_UART0:
  case ERR_CODE_INIT_UART1:
  case ERR_CODE_INIT_FILTER:
  case ERR_CODE_INIT_TIMER:
    eh->current_mode = SYSTEM_MODE_PANIC;
    return REACTION_PANIC_HALT;

  case ERR_CODE_RX0_OVERFLOW:
  case ERR_CODE_TX0_OVERFLOW:
  case ERR_CODE_RX1_OVERFLOW:
  case ERR_CODE_TX1_OVERFLOW:
    eh->current_mode = SYSTEM_MODE_DEGRADED;
    return REACTION_LOG_ONLY;

  case ERR_CODE_HARDWARE_FAULT:
    eh->current_mode = SYSTEM_MODE_PANIC;
    return REACTION_SYSTEM_RESET;

  case ERR_CODE_NONE:
  default:
    return REACTION_NONE;
  }
}

void error_handler_panic(error_handler_t *eh, error_code_t code) {
  if (eh != (void *)0) {
    eh->current_mode = SYSTEM_MODE_PANIC;
    eh->last_error = code;
    eh->error_count++;
  }

  /* Diagnostic optical signaling: blink LED 'code' times, then pause 1 second. */
  uint8_t blink_count = (uint8_t)code;
  if (blink_count == 0) {
    blink_count = 1;
  }

  while (1) {
    for (uint8_t i = 0; i < blink_count; i++) {
      led_on();
      _delay_ms(150);
      led_off();
      _delay_ms(150);
    }
    _delay_ms(1000);
  }
}

system_mode_t error_handler_get_mode(const error_handler_t *eh) {
  if (eh == (void *)0) {
    return SYSTEM_MODE_PANIC;
  }
  return eh->current_mode;
}

error_code_t error_handler_get_last_error(const error_handler_t *eh) {
  if (eh == (void *)0) {
    return ERR_CODE_NONE;
  }
  return eh->last_error;
}

uint16_t error_handler_get_error_count(const error_handler_t *eh) {
  if (eh == (void *)0) {
    return 0;
  }
  return eh->error_count;
}

bool error_handler_clear(error_handler_t *eh) {
  if (eh == (void *)0) {
    return false;
  }

  eh->last_error = ERR_CODE_NONE;
  eh->current_mode = SYSTEM_MODE_NORMAL;

  return true;
}

static const char S_MODE_STRINGS[][9] PROGMEM = {
    "NORMAL",
    "DEGRADED",
    "PANIC",
};

static const char S_ERROR_CODE_STRINGS[][16] PROGMEM = {
    "NONE",
    "INIT_LED",
    "INIT_UART0",
    "INIT_UART1",
    "INIT_FILTER",
    "RX0_OVERFLOW",
    "TX0_OVERFLOW",
    "RX1_OVERFLOW",
    "TX1_OVERFLOW",
    "HARDWARE_FAULT",
};

const char *error_handler_mode_to_str(system_mode_t mode) {
  if ((uint8_t)mode < (sizeof(S_MODE_STRINGS) / sizeof(S_MODE_STRINGS[0]))) {
    return S_MODE_STRINGS[mode];
  }
  return PSTR("UNKNOWN");
}

const char *error_handler_code_to_str(error_code_t code) {
  if ((uint8_t)code < (sizeof(S_ERROR_CODE_STRINGS) / sizeof(S_ERROR_CODE_STRINGS[0]))) {
    return S_ERROR_CODE_STRINGS[code];
  }
  return PSTR("UNKNOWN");
}
