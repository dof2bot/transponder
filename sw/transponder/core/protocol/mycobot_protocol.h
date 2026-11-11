/**
 * @file mycobot_protocol.h
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
 * @brief Elephant Robotics and Feetech serial protocol constants and framing definitions.
 *
 * Elephant Robotics Protocol Framing (Verified & Forwarded to Host PC):
 *
 *   +-----------+-----------+-------------+------------+-----------+-----------+
 *   | 0xFE (H1) | 0xFE (H2) | Length (len)| Cmd / Data | ... (N-1) | 0xFA (End)|
 *   +-----------+-----------+-------------+------------+-----------+-----------+
 *   |<--------- Header ---->|             |<-------- Payload ------>|<- Footer -|
 *
 * Feetech Servo Bus Framing (Filtered out & Discarded from Host PC):
 *
 *   +-----------+-----------+-------------+------------+-----------+------------+
 *   | 0xFF (H1) | 0xFF (H2) |  Servo ID   | Length (L) |Instruction|  Checksum  |
 *   +-----------+-----------+-------------+------------+-----------+------------+
 *   |<------ Discarded internal servo bus chatter (L + 3 bytes total) ---------->|
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Protocol framing marker bytes for Elephant Robotics and Feetech serial bus.
 */
typedef enum {
  MYCOBOT_HEADER_BYTE = 0xFE, /**< Primary framing header marker byte. */
  MYCOBOT_FOOTER_BYTE = 0xFA, /**< Packet framing footer marker byte. */
  SERVO_HEADER_BYTE = 0xFF    /**< Internal Feetech servo bus frame header byte. */
} protocol_marker_t;

/**
 * @brief Protocol sizing limits and packet buffer capacities.
 */
typedef enum {
  MYCOBOT_MAX_FRAME_LEN = 64, /**< Maximum allowable MyCobot frame length in bytes. */
  SERVO_MAX_PACKET_LEN = 64   /**< Maximum allowable Feetech servo packet length to discard. */
} protocol_limit_t;

#ifdef __cplusplus
}
#endif
