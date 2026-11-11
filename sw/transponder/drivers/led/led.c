/**
 * @file led.c
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

#include "led.h"
#include <avr/io.h>

/**
 * @brief Status LED pin assignment on ATmega2560 (Port B, Bit 7).
 */
typedef enum {
  LED_PIN_BIT = PB7
} led_pin_t;

void led_init(void) {
  /* Set PB7 as output. */
  DDRB |= (1 << LED_PIN_BIT);
  /* Initial state: LED OFF. */
  PORTB &= ~(1 << LED_PIN_BIT);
}

void led_on(void) {
  PORTB |= (1 << LED_PIN_BIT);
}

void led_off(void) {
  PORTB &= ~(1 << LED_PIN_BIT);
}

void led_toggle(void) {
  /* Writing a logic 1 to PINBn toggles the PORTBn bit in AVR. */
  PINB |= (1 << LED_PIN_BIT);
}

