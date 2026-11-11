/**
 * @file packet_filter.c
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

#include "packet_filter.h"

/**
 * @brief Initialize packet filter context with verified callback handler.
 *
 * @param filter Pointer to packet_filter_t instance.
 * @param on_frame Callback function invoked on complete verified frames.
 * @return bool true on success, false if filter is NULL.
 */
bool packet_filter_init(packet_filter_t *filter, packet_filter_callback_t on_frame) {
  if (filter == (void *)0) {
    return false;
  }

  filter->state = FILTER_STATE_IDLE;
  filter->frame_idx = 0;
  filter->remaining_bytes = 0;
  filter->on_frame = on_frame;

  return true;
}

/**
 * @brief Reset filter state machine to idle state.
 *
 * @param filter Pointer to packet_filter_t instance.
 */
void packet_filter_reset(packet_filter_t *filter) {
  if (filter == (void *)0) {
    return;
  }

  filter->state = FILTER_STATE_IDLE;
  filter->frame_idx = 0;
  filter->remaining_bytes = 0;
}

/**
 * @brief Check if filter state machine is in idle state waiting for a frame.
 *
 * @param filter Pointer to packet_filter_t instance.
 * @return bool true if in FILTER_STATE_IDLE or filter is NULL, false if parsing frame.
 */
bool packet_filter_is_idle(const packet_filter_t *filter) {
  if (filter == (void *)0) {
    return true;
  }

  return (filter->state == FILTER_STATE_IDLE);
}

/**
 * @brief Feed a single byte into the filter state machine.
 *
 * Discards Feetech servo packets (0xFF 0xFF <id> <len> ...) and collects
 * valid Elephant Robotics frames (0xFE 0xFE <len> <payload> 0xFA).
 *
 * @param filter Pointer to packet_filter_t instance.
 * @param byte Incoming raw byte from Arm USART1.
 */
void packet_filter_feed(packet_filter_t *filter, uint8_t byte) {
  if (filter == (void *)0) {
    return;
  }

  switch (filter->state) {
  case FILTER_STATE_IDLE:
    if (byte == MYCOBOT_HEADER_BYTE) {
      filter->state = FILTER_STATE_MYCOBOT_HEADER_2;
    } else if (byte == SERVO_HEADER_BYTE) {
      filter->state = FILTER_STATE_SERVO_HEADER_2;
    }
    break;

  case FILTER_STATE_MYCOBOT_HEADER_2:
    if (byte == MYCOBOT_HEADER_BYTE) {
      filter->frame_buf[0] = MYCOBOT_HEADER_BYTE;
      filter->frame_buf[1] = MYCOBOT_HEADER_BYTE;
      filter->frame_idx = 2;
      filter->state = FILTER_STATE_MYCOBOT_LEN;
    } else if (byte == SERVO_HEADER_BYTE) {
      filter->state = FILTER_STATE_SERVO_HEADER_2;
    } else {
      filter->state = FILTER_STATE_IDLE;
    }
    break;

  case FILTER_STATE_MYCOBOT_LEN:
    if (byte == MYCOBOT_HEADER_BYTE) {
      /* Extra 0xFE received; keep frame_buf initialized and remain ready for len. */
      filter->frame_buf[0] = MYCOBOT_HEADER_BYTE;
      filter->frame_buf[1] = MYCOBOT_HEADER_BYTE;
      filter->frame_idx = 2;
    } else if (byte < 1 || byte > (MYCOBOT_MAX_FRAME_LEN - 3)) {
      filter->state = FILTER_STATE_IDLE;
    } else {
      filter->frame_buf[filter->frame_idx++] = byte;
      filter->remaining_bytes = byte;
      filter->state = FILTER_STATE_MYCOBOT_PAYLOAD;
    }
    break;

  case FILTER_STATE_MYCOBOT_PAYLOAD:
    if (filter->frame_idx < MYCOBOT_MAX_FRAME_LEN) {
      filter->frame_buf[filter->frame_idx++] = byte;
    }
    filter->remaining_bytes--;

    if (filter->remaining_bytes == 0) {
      /* Verify packet footer marker (0xFA). */
      if (byte == MYCOBOT_FOOTER_BYTE) {
        if (filter->on_frame != (void *)0) {
          filter->on_frame(filter->frame_buf, filter->frame_idx);
        }
      }
      filter->state = FILTER_STATE_IDLE;
      filter->frame_idx = 0;
    }
    break;

  case FILTER_STATE_SERVO_HEADER_2:
    if (byte == SERVO_HEADER_BYTE) {
      filter->state = FILTER_STATE_SERVO_ID;
    } else if (byte == MYCOBOT_HEADER_BYTE) {
      filter->state = FILTER_STATE_MYCOBOT_HEADER_2;
    } else {
      filter->state = FILTER_STATE_IDLE;
    }
    break;

  case FILTER_STATE_SERVO_ID:
    /* Received Servo ID; next byte is packet length. */
    filter->state = FILTER_STATE_SERVO_LEN;
    break;

  case FILTER_STATE_SERVO_LEN:
    if (byte < 1 || byte > SERVO_MAX_PACKET_LEN) {
      filter->state = FILTER_STATE_IDLE;
    } else {
      filter->remaining_bytes = byte;
      filter->state = FILTER_STATE_SERVO_DISCARD;
    }
    break;

  case FILTER_STATE_SERVO_DISCARD:
    filter->remaining_bytes--;
    if (filter->remaining_bytes == 0) {
      filter->state = FILTER_STATE_IDLE;
    }
    break;

  default:
    filter->state = FILTER_STATE_IDLE;
    break;
  }
}
