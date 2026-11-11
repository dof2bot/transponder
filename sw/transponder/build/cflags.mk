# cflags.mk
# Copyright (C) 2026 Vladimir Roncevic <elektron.ronca@gmail.com>
#
# transponder is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# transponder is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program_name. If not, see <http://www.gnu.org/licenses/>.

INCLUDES := \
	-I.. \
	-I../core/interfaces \
	-I../core/error_handler \
	-I../core/ring_buffer \
	-I../core/protocol \
	-I../core/packet_filter \
	-I../core/gateway \
	-I../drivers/uart/uart0 \
	-I../drivers/uart/uart1 \
	-I../drivers/led \
	-I../drivers/timer

CFLAGS := \
	-Wl,-Map,transponder.map \
	-mmcu=atmega2560

CCFLAGS := \
	-Wall \
	-g2 \
	-O2 \
	-fpack-struct \
	-fshort-enums \
	-ffunction-sections \
	-fdata-sections \
	-std=gnu99 \
	-funsigned-char \
	-funsigned-bitfields \
	-mmcu=atmega2560 \
	-DF_CPU=16000000UL \
	$(INCLUDES) \
	-MMD \
	-MP \
	-MF
