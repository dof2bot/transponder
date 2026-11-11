# Makefile
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

include sources.mk
include subdir.mk
include objects.mk
include cflags.mk
include odflags.mk
include ocflags.mk
include csflags.mk
include tools.mk

ifneq ($(MAKECMDGOALS),clean)
	ifneq ($(strip $(I_DEPS)),)
		-include $(I_DEPS)
	endif
	ifneq ($(strip $(ASM_DEPS)),)
		-include $(ASM_DEPS)
	endif
	ifneq ($(strip $(M_DEPS)),)
		-include $(M_DEPS)
	endif
	ifneq ($(strip $(MI_DEPS)),)
		-include $(MI_DEPS)
	endif
	ifneq ($(strip $(S_DEPS)),)
		-include $(S_DEPS)
	endif
	ifneq ($(strip $(S_UPPER_DEPS)),)
		-include $(S_UPPER_DEPS)
	endif
	ifneq ($(strip $(C_DEPS)),)
		-include $(C_DEPS)
	endif
endif

LSS += \
	transponder.lss

FLASH_IMAGE += \
	transponder.hex

SIZEDUMMY += \
	sizedummy

CLEAN_LIST += \
	$(FLASH_IMAGE) \
	$(ELFS) \
	$(I_DEPS) \
	$(OBJS) \
	$(ASM_DEPS) \
	$(M_DEPS) \
	$(MI_DEPS) \
	$(S_DEPS) \
	$(SIZEDUMMY) \
	$(S_UPPER_DEPS) \
	$(LSS) \
	$(C_DEPS) \
	*.i \
	*.s \
	transponder.map \
	transponder.elf

PORT ?= $(shell test -e /dev/ttyACM0 && echo /dev/ttyACM0 || echo /dev/ttyUSB0)
BAUD ?= 115200

all: transponder.elf secondary-outputs

transponder.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	$(CC) $(CFLAGS) -o "transponder.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

transponder.lss: transponder.elf
	@echo 'Invoking: AVR Create Extended Listing'
	$(OD) $(ODFLAGS) transponder.elf > "transponder.lss"
	@echo 'Finished building: $@'
	@echo ' '

transponder.hex: transponder.elf
	@echo 'Create Flash image (ihex format)'
	$(OC) $(OCFLAGS) -O ihex transponder.elf "transponder.hex"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: transponder.elf
	@echo 'Invoking: Print Size'
	$(CS) $(CSFLAGS) transponder.elf
	@echo 'Finished building: $@'
	@echo ' '

flash: transponder.hex
	@echo 'Flashing transponder.hex to ATmega2560 on $(PORT) at $(BAUD) baud...'
	avrdude -v -p m2560 -c wiring -P $(PORT) -b $(BAUD) -D -U flash:w:transponder.hex:i

clean:
	@for item in $(CLEAN_LIST);do echo "$(RM) $$item";$(RM) $$item;done
	@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(SIZEDUMMY)

.PHONY: all clean secondary-outputs flash
