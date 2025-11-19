#  Toolchain
CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-gcc
SIZE    = arm-none-eabi-size
OBJCOPY = arm-none-eabi-objcopy

PROJECT = USART

#  MCU Options (STM32H743 = Cortex-M7)
CPU = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

#  Directories
SRC_DIR   = src
SYS_DIR   = src/system
DRIVER_DIR= drivers
INC_DIR   = inc
BUILD_DIR = build

#  Sources & Objects
SRCS      	= $(wildcard $(SRC_DIR)/*.c)
SYS_SRCS  	= $(wildcard $(SYS_DIR)/*.c) $(wildcard $(SYS_DIR)/*.s)
DRIVER_SRCS = $(wildcard $(DRIVER_DIR)/src/*.c)

OBJS = \
		$(patsubst $(SYS_DIR)/%.s,$(BUILD_DIR)/%.o,$(filter %.s,$(SYS_SRCS))) \
		$(patsubst $(SYS_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SYS_SRCS))) \
		$(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SRCS))) \
		$(patsubst $(DRIVER_DIR)/src/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(DRIVER_SRCS)))


DEPS = $(OBJS:.o=.d)

ELF  = $(BUILD_DIR)/$(PROJECT).elf
BIN  = $(BUILD_DIR)/$(PROJECT).bin
HEX  = $(BUILD_DIR)/$(PROJECT).hex
MAP  = $(BUILD_DIR)/$(PROJECT).map

#  Flags
INCLUDES = \
	-I$(INC_DIR) \
	-I$(INC_DIR)/CMSIS/Core/Include \
	-I$(DRIVER_DIR)/inc

CFLAGS = $(CPU) -std=gnu11 -g -Wall -Wextra -Os -ffunction-sections -fdata-sections -MMD -MP $(INCLUDES) -DSTM32H743xx 
ASMFLAGS = $(CPU) -g3 -x assembler-with-cpp
LDFLAGS = $(CPU) -Wall -Os -TSTM32H743XX_FLASH.ld -Wl,--gc-sections -Wl,-Map=$(MAP) -Wl,--start-group -specs=nano.specs -lnosys -lm -Wl,--end-group # -lc -u _printf_float

#  Default target
all: $(BUILD_DIR) $(ELF) $(BIN) $(HEX)
	@echo "Build complete: $(PROJECT)"

#  Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

#  Build rules
$(BUILD_DIR)/%.o: $(SYS_DIR)/%.s
	@echo "Assembling $<"
	$(CC) $(ASMFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SYS_DIR)/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@
	
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(DRIVER_DIR)/src/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@


#  Linking
$(ELF): $(OBJS)
	@echo "Linking $@"
	$(LD) $(OBJS) -o $@ $(LDFLAGS)
	+@make -s print_size

#  Output formats
$(BIN): $(ELF)
	@echo "Creating binary $@"
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O verilog $< $@

#  Flashing
flash: $(BIN)
	@echo "Flashing target..."
	JLinkExe -device STM32H743VI -if SWD -speed 4000 -autoconnect 1 -CommanderScript jflash.jlink

#  Clean
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

#  Include dependency files
-include $(DEPS)


print_size:
	@echo "Memory usage summary:"
	@$(SIZE) -B $(ELF)
	@echo ""
	@$(SIZE) -B -A $(ELF) | awk '\
		/\.isr_vector/  { flash += $$2 } \
		/\.text/        { flash += $$2 } \
		/\.rodata/      { flash += $$2 } \
		/\.ARM.extab/   { flash += $$2 } \
		/\.ARM/         { flash += $$2 } \
		/\.init_array/  { flash += $$2 } \
		/\.fini_array/  { flash += $$2 } \
		/\.data/        { flash += $$2; ram += $$2 } \
		/\.bss/         { ram += $$2 } \
		/_user_heap_stack/ { ram += $$2 } \
		END { \
			flash_total = 2097152; \
			ram_total   = 1048576; \
			ram_perc   = ram / ram_total * 100; \
			flash_perc = flash / flash_total * 100; \
			ram_bar   = int(ram_perc / 10); \
			flash_bar = int(flash_perc / 10); \
			printf "RAM:   [%s%s] %5.1f%% (used %d of %d bytes)\n", \
				repeat("#", ram_bar), repeat(" ", 10 - ram_bar), \
				ram_perc, ram, ram_total; \
			printf "Flash: [%s%s] %5.1f%% (used %d of %d bytes)\n", \
				repeat("#", flash_bar), repeat(" ", 10 - flash_bar), \
				flash_perc, flash, flash_total; \
		} \
		function repeat(s, n, r) { \
			r = ""; for (i = 0; i < n; i++) r = r s; return r; \
		}'

.PHONY: all flash clean print_size