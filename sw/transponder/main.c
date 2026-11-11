/**
 * @file main.c
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
 * @brief Main firmware entrypoint for ATmega2560 Transponder Gateway.
 *
 * High-Level System Architecture & Execution Flow:
 *
 *   +-------------------------------------------------------------+
 *   |                         main()                              |
 *   |                                                             |
 *   |  1. Early Safety Boot:                                      |
 *   |     |- MCUSR = 0; wdt_disable()   -> Prevents WDT reset loop|
 *   |     `- error_handler_init()       -> Diagnostics ready      |
 *   |                                                             |
 *   |  2. gateway_init()                                          |
 *   |     |- led_init()                 -> PB7 output configured  |
 *   |     |- timer_init()               -> Timer1 1ms CTC tick    |
 *   |     |- uart0_init(115200)         -> Ring Buffers 1 & 2     |
 *   |     |- uart1_init(1000000)        -> Ring Buffers 3 & 4     |
 *   |     `- packet_filter_init(...)    -> State Machine ready    |
 *   |                                                             |
 *   |  3. Hardware Watchdog & Interrupts:                         |
 *   |     |- wdt_enable(WDTO_2S)        -> 2.0s hardware failsafe |
 *   |     `- sei()                      -> Global interrupts ON   |
 *   |                                                             |
 *   |  4. Real-Time Superloop:                                    |
 *   |     while (1) {                                             |
 *   |         wdt_reset();              -> Feed Watchdog          |
 *   |         gateway_process();        -> Non-blocking streaming |
 *   |     }                                                       |
 *   +-------------------------------------------------------------+
 *                                |
 *      +-------------------------+-------------------------+
 *      |                                                   |
 *      v                                                   v
 *  [ Interrupt Services (ISRs) ]              [ Non-Blocking Processing ]
 *  - TIMER1_COMPA_vect (1ms System Tick)      - Downlink: PC -> Arm
 *  - USART0_RX_vect   (Pushes PC byte)        - Uplink:   Arm -> Filter -> PC
 *  - USART0_UDRE_vect (Pops byte to PC)       - Timeout:  25ms Silence Reset
 *  - USART1_RX_vect   (Pushes Arm byte)       - Activity LED feedback
 *  - USART1_UDRE_vect (Pops byte to Arm)      - Hardware Watchdog Refresh
 */

#include "error_handler.h"
#include "gateway.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>

/** Global error handler instance. */
static error_handler_t s_error_handler;

int main(void) {
  /* Clear reset flags and disable watchdog during early boot to prevent reset loops. */
  MCUSR = 0;
  wdt_disable();

  /* Initialize error handler. */
  (void)error_handler_init(&s_error_handler);

  /* Initialize all gateway peripherals and internal data structures. */
  if (!gateway_init(&s_error_handler)) {
    /* If initialization fails, enter visual optical diagnostic panic loop on LED. */
    error_handler_panic(&s_error_handler, error_handler_get_last_error(&s_error_handler));
  }

  /* Hardware Watchdog temporarily disabled to isolate communication diagnostics. */
  /* wdt_enable(WDTO_2S); */

  /* Enable global interrupts to start interrupt-driven ring buffer transfers. */
  sei();

  /* Main non-blocking real-time event loop. */
  while (1) {
    wdt_reset();
    gateway_process();
  }

  return 0;
}
