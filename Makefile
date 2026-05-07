PRJ_NAME   = stm32f103-bare-metal
DEVICE     = STM32F103xB

CC         = arm-none-eabi-gcc
OBJCOPY    = arm-none-eabi-objcopy
OBJDUMP    = arm-none-eabi-objdump
SIZE       = arm-none-eabi-size
PROGRAMMER = openocd

LDSCRIPT   = stm32f103c8tx.ld
OPT       ?= -Og

CFLAGS     = -Wall -mcpu=cortex-m3 -mlittle-endian -mthumb -I inc/ -D $(DEVICE) $(OPT)
ASFLAGS    = $(CFLAGS)
LDFLAGS    = -T $(LDSCRIPT) -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs

EXAMPLES  := $(notdir $(patsubst %/,%,$(wildcard examples/*/)))

.PHONY: all flash burn hex bin clean list help

ifeq ($(MAKECMDGOALS),)
NEED_EXAMPLE := 1
endif
ifneq ($(filter all flash burn hex bin,$(MAKECMDGOALS)),)
NEED_EXAMPLE := 1
endif

ifeq ($(NEED_EXAMPLE),1)
ifndef EXAMPLE
$(info )
$(info ERROR: EXAMPLE is not set.)
$(info Usage: make EXAMPLE=<name> [target])
$(info )
$(info Available examples:)
$(foreach e,$(EXAMPLES),$(info   $(e)))
$(info )
$(error Please specify EXAMPLE=<name>)
endif
ifeq ($(wildcard examples/$(EXAMPLE)),)
$(error Unknown EXAMPLE '$(EXAMPLE)'. Run 'make list' to see valid names.)
endif
endif

BUILD_DIR := build/$(EXAMPLE)
ELF       := $(BUILD_DIR)/$(EXAMPLE).elf
HEX       := $(BUILD_DIR)/$(EXAMPLE).hex
BIN       := $(BUILD_DIR)/$(EXAMPLE).bin

CSRC      := $(wildcard core/*.c) $(wildcard examples/$(EXAMPLE)/*.c)
ASRC      := $(wildcard core/*.s)
OBJ       := $(addprefix $(BUILD_DIR)/,$(CSRC:.c=.o) $(ASRC:.s=.o))

PGFLAGS    = -f openocd.cfg -c "program $(ELF) verify reset" -c shutdown

all: $(ELF)

$(ELF): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)
	$(SIZE) $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -MMD -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) -MMD -c $(ASFLAGS) $< -o $@

-include $(OBJ:.o=.d)

hex: $(HEX)
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

bin: $(BIN)
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

flash burn: $(ELF)
	$(PROGRAMMER) $(PGFLAGS)

clean:
	rm -rf build

list:
	@echo "Available examples:"
	@for e in $(EXAMPLES); do echo "  $$e"; done

help:
	@echo "Usage: make EXAMPLE=<name> [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all     Build the ELF (default)"
	@echo "  hex     Build Intel HEX"
	@echo "  bin     Build raw binary"
	@echo "  flash   Flash via OpenOCD (alias: burn)"
	@echo "  clean   Remove build/ tree"
	@echo "  list    Show available example names"
	@echo ""
	@echo "Variables:"
	@echo "  EXAMPLE   Required for build/flash targets"
	@echo "  OPT       Optimization flag (default: -Og)"
