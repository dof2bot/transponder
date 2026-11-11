/**
 * @file led.h
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
 * @brief Board status LED driver for Arduino Mega 2560 (Digital Pin 13 / PB7).
 *
 * Hardware Schematic & Pin Mapping:
 *
 *     ATmega2560                          Arduino Mega 2560
 *   +------------+                        +---------------+
 *   |            |       Current Limit    |               |
 *   |  PB7 (Pin) |---> [ 1k Resistor ] -->| [ Builtin LED]|---> GND
 *   |            |                        |   (Pin 13)    |
 *   +------------+                        +---------------+
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize PB7 as digital output and turn LED off.
 */
void led_init(void);

/**
 * @brief Turn on status LED (drive PB7 HIGH).
 */
void led_on(void);

/**
 * @brief Turn off status LED (drive PB7 LOW).
 */
void led_off(void);

/**
 * @brief Toggle status LED state (invert PB7 output).
 */
void led_toggle(void);

#ifdef __cplusplus
}
#endif

