/**
 * @file timer.h
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
 *
 * Configures AVR hardware Timer1 in Clear Timer on Compare Match (CTC) mode
 * to produce a periodic 1.0 millisecond interrupt tick for non-blocking task
 * scheduling, inter-byte packet timeouts, and watchdog reset monitoring.
 *
 * CTC Frequency & Period Derivation:
 *   f_CPU = 16,000,000 Hz (16 MHz Crystal Oscillator)
 *   Prescaler N = 64 (CS11=1, CS10=1)
 *   Target Tick Frequency f_CTC = 1,000 Hz (T = 1.0 ms)
 *
 *   Formula:
 *     OCR1A = [ f_CPU / (N * f_CTC) ] - 1
 *     OCR1A = [ 16,000,000 / (64 * 1,000) ] - 1
 *     OCR1A = [ 16,000,000 / 64,000 ] - 1 = 250 - 1 = 249
 *
 * Timer1 CTC Counter Waveform:
 *  TCNT1
 *    ^
 *    |              OCR1A = 249 (Hardware Reset to 0)
 *    |             /|       /|        /|
 *    |            / |      / |       / |
 *    |           /  |     /  |      /  |
 *    |          /   |    /   |     /   |
 *    0 --------+----+---+----+----+----+---> Time (t)
 *                   ^        ^         ^
 *                   |        |         |
 *                  1ms      1ms       1ms  (ISR: s_millis++)
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Timer1 in CTC mode with 1ms compare match interrupts.
 *
 * Configures Timer1 registers:
 * - TCCR1A = 0 (Normal port operation, OC1A/OC1B disconnected)
 * - TCNT1  = 0 (Reset counter)
 * - OCR1A  = 249 (Compare match value for 1ms at 16MHz/64)
 * - TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10) (CTC mode, prescaler 64)
 * - TIMSK1 |= (1 << OCIE1A) (Enable Output Compare A Match interrupt)
 *
 * @return bool true on successful timer initialization.
 */
bool timer_init(void);

/**
 * @brief Get elapsed milliseconds since system boot.
 *
 * Performs thread-safe atomic reading of the volatile 32-bit millisecond
 * counter using an ATOMIC_BLOCK to prevent multi-byte register tearing.
 *
 * @return uint32_t Elapsed milliseconds since boot.
 */
uint32_t timer_millis(void);

/**
 * @brief Non-blocking elapsed interval evaluator for cooperative scheduling.
 *
 * Computes elapsed time using unsigned modular subtraction to ensure seamless
 * rollover handling when the 32-bit counter overflows (~49.7 days).
 *
 * @param last_time Pointer to timestamp variable holding the last execution time.
 *                  Updated to current time when interval has elapsed.
 * @param interval_ms Target interval period in milliseconds.
 * @return bool true if interval has elapsed (and updates *last_time), false otherwise.
 */
bool timer_interval_elapsed(uint32_t *last_time, uint32_t interval_ms);

#ifdef __cplusplus
}
#endif

