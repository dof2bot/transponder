/**
 * @file packet_filter.h
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
 * @brief Stream filter separating MyCobot frames from internal Feetech servo chatter.
 *
 * State Machine Architecture:
 *
 *                       +---------------------------------------+
 *                       |                                       |
 *                       v                                       |
 *                   +-------+                                   |
 *       +---------> | IDLE  | <---------------------------------+
 *       |           +-------+                                   |
 *       |             |   |                                     |
 *       |    0xFE     |   | 0xFF                                |
 *       |             v   v                                     |
 *       |  +------------+   +------------+                      |
 *       |  | MYCOBOT_H2 |   |  SERVO_H2  |                      |
 *       |  +------------+   +------------+                      |
 *       |        |                |                             |
 *       |   0xFE |           0xFF |                             |
 *       |        v                v                             |
 *       |  +------------+   +------------+                      |
 *       |  | MYCOBOT_LEN|   |  SERVO_ID  |                      |
 *       |  +------------+   +------------+                      |
 *       |        |                |                             |
 *       |        | (read len)     | (read id)                   |
 *       |        v                v                             |
 *       |  +------------+   +------------+                      |
 *       |  |  PAYLOAD   |   | SERVO_LEN  |                      |
 *       |  +------------+   +------------+                      |
 *       |        |                |                             |
 *       |        | (collect len)  | (read len)                  |
 *       |        v                v                             |
 *       |  [Verify 0xFA]    +------------+                      |
 *       |  (Emit Frame)     |DISCARD_DATA|                      |
 *       |                   +------------+                      |
 *       |                         | (discard len bytes)         |
 *       +-------------------------+-----------------------------+
 */

#pragma once

#include "mycobot_protocol.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Filter callback invoked when a complete valid MyCobot frame is assembled.
 *
 * @param frame Pointer to contiguous verified frame bytes (0xFE 0xFE <len> ... 0xFA).
 * @param length Total length of frame in bytes (3 + len).
 */
typedef void (*packet_filter_callback_t)(const uint8_t *frame, uint8_t length);

/**
 * @brief Parser state enumeration.
 */
typedef enum {
  FILTER_STATE_IDLE = 0,
  FILTER_STATE_MYCOBOT_HEADER_2,
  FILTER_STATE_MYCOBOT_LEN,
  FILTER_STATE_MYCOBOT_PAYLOAD,
  FILTER_STATE_SERVO_HEADER_2,
  FILTER_STATE_SERVO_ID,
  FILTER_STATE_SERVO_LEN,
  FILTER_STATE_SERVO_DISCARD
} filter_state_t;

/**
 * @brief Packet filter context structure.
 */
typedef struct {
  filter_state_t state;
  uint8_t frame_buf[MYCOBOT_MAX_FRAME_LEN];
  uint8_t frame_idx;
  uint8_t remaining_bytes;
  packet_filter_callback_t on_frame;
} packet_filter_t;

/**
 * @brief Initialize packet filter context.
 *
 * @param filter Pointer to packet_filter_t instance.
 * @param on_frame Callback function invoked on complete verified frames.
 * @return bool true on success.
 */
bool packet_filter_init(packet_filter_t *filter, packet_filter_callback_t on_frame);

/**
 * @brief Feed a single byte into the filter state machine.
 *
 * @param filter Pointer to packet_filter_t instance.
 * @param byte Incoming raw byte from Arm USART.
 */
void packet_filter_feed(packet_filter_t *filter, uint8_t byte);

/**
 * @brief Reset filter state machine to idle.
 *
 * @param filter Pointer to packet_filter_t instance.
 */
void packet_filter_reset(packet_filter_t *filter);

/**
 * @brief Check if filter state machine is in idle state waiting for a frame.
 *
 * @param filter Pointer to packet_filter_t instance.
 * @return bool true if in FILTER_STATE_IDLE or filter is NULL, false if parsing frame.
 */
bool packet_filter_is_idle(const packet_filter_t *filter);

#ifdef __cplusplus
}
#endif
