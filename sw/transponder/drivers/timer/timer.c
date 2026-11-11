/**
 * @file timer.c
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
 * @brief High-resolution 1ms system timebase generator using 16-bit Timer1.
 */

#include "timer.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

/**
 * @brief Localized CTC Compare Match Value:
 *        16MHz CPU clock / 64 prescaler / 1000Hz = 250 ticks -> OCR1A = 249
 */
static const uint16_t TIMER1_COMPARE_MATCH_1MS = 249;

/**
 * @brief Volatile 32-bit counter for elapsed milliseconds since boot.
 */
static volatile uint32_t s_millis = 0;

/**
 * @brief Timer1 Compare Match A Interrupt Service Routine.
 *
 * Increments the 32-bit milliseconds counter every 1ms.
 */
ISR(TIMER1_COMPA_vect) { s_millis++; }

/**
 * @brief Initialize Timer1 in CTC mode (Clear Timer on Compare Match).
 *
 * @return bool true on successful timer initialization.
 */
bool timer_init(void) {
  /* Stop Timer1 during setup. */
  TCCR1A = 0;
  TCCR1B = 0;

  /* Reset timer counter. */
  TCNT1 = 0;

  /* Set compare value for 1ms interval. */
  OCR1A = TIMER1_COMPARE_MATCH_1MS;

  /* CTC mode (WGM12=1) with prescaler 64 (CS11=1, CS10=1). */
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);

  /* Enable Timer1 Compare Match A Interrupt. */
  TIMSK1 |= (1 << OCIE1A);

  /* Reset milliseconds counter. */
  s_millis = 0;

  return true;
}

/**
 * @brief Get elapsed milliseconds since system boot.
 *
 * @return uint32_t Milliseconds counter.
 */
uint32_t timer_millis(void) {
  uint32_t ms = 0;

  /* Atomic block with memory barrier prevents 32-bit tearing and compiler reordering. */
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ms = s_millis; }

  return ms;
}

/**
 * @brief Check if a specified interval has elapsed since the last check.
 *
 * @param last_time Pointer to the variable holding the last timestamp.
 * @param interval_ms Target interval in milliseconds.
 * @return bool true if interval has elapsed (and updates *last_time), false otherwise.
 */
bool timer_interval_elapsed(uint32_t *last_time, uint32_t interval_ms) {
  if (last_time == (void *)0) {
    return false;
  }

  uint32_t current_time = timer_millis();

  if ((current_time - *last_time) >= interval_ms) {
    *last_time = current_time;
    return true;
  }

  return false;
}

