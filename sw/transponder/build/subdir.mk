# subdir.mk
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

include tools.mk
include cflags.mk

C_SRCS += \
	../main.c \
	../core/ring_buffer/ring_buffer.c \
	../core/packet_filter/packet_filter.c \
	../core/error_handler/error_handler.c \
	../core/gateway/gateway.c \
	../drivers/uart/uart0/uart0.c \
	../drivers/uart/uart1/uart1.c \
	../drivers/led/led.c \
	../drivers/timer/timer.c

OBJS += \
	./main.o \
	./ring_buffer.o \
	./packet_filter.o \
	./error_handler.o \
	./gateway.o \
	./uart0.o \
	./uart1.o \
	./led.o \
	./timer.o

C_DEPS += \
	./main.d \
	./ring_buffer.d \
	./packet_filter.d \
	./error_handler.d \
	./gateway.d \
	./uart0.d \
	./uart1.d \
	./led.d \
	./timer.d

%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/ring_buffer/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/packet_filter/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/error_handler/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/gateway/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../drivers/uart/uart0/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../drivers/uart/uart1/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../drivers/led/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../drivers/timer/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
