# Wokwi build of the DAQ telemetry demo (STM32C031C6 / Nucleo-C031C6)
#
# Usage:
#   make              -> build/DAQ_Telemetry_Wokwi.{elf,hex,bin}
#   make clean
#
# Toolchain: arm-none-eabi-gcc must be on PATH, or pass GCC_PATH=<bin dir>
#   e.g. make GCC_PATH="C:/ST/STM32CubeIDE_1.19.0/STM32CubeIDE/plugins/<gnu-tools>/tools/bin"

TARGET = DAQ_Telemetry_Wokwi
BUILD_DIR = build

ifdef GCC_PATH
PREFIX = $(GCC_PATH)/arm-none-eabi-
else
PREFIX = arm-none-eabi-
endif
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

HAL = Drivers/STM32C0xx_HAL_Driver

C_SOURCES = \
Core/Src/main.c \
Core/Src/stm32c0xx_it.c \
Core/Src/stm32c0xx_hal_msp.c \
Core/Src/system_stm32c0xx.c \
Core/Src/syscalls.c \
Core/Src/sysmem.c \
$(HAL)/Src/stm32c0xx_hal.c \
$(HAL)/Src/stm32c0xx_hal_adc.c \
$(HAL)/Src/stm32c0xx_hal_adc_ex.c \
$(HAL)/Src/stm32c0xx_hal_cortex.c \
$(HAL)/Src/stm32c0xx_hal_dma.c \
$(HAL)/Src/stm32c0xx_hal_dma_ex.c \
$(HAL)/Src/stm32c0xx_hal_exti.c \
$(HAL)/Src/stm32c0xx_hal_flash.c \
$(HAL)/Src/stm32c0xx_hal_flash_ex.c \
$(HAL)/Src/stm32c0xx_hal_gpio.c \
$(HAL)/Src/stm32c0xx_hal_pwr.c \
$(HAL)/Src/stm32c0xx_hal_pwr_ex.c \
$(HAL)/Src/stm32c0xx_hal_rcc.c \
$(HAL)/Src/stm32c0xx_hal_rcc_ex.c \
$(HAL)/Src/stm32c0xx_hal_tim.c \
$(HAL)/Src/stm32c0xx_hal_tim_ex.c \
$(HAL)/Src/stm32c0xx_hal_uart.c \
$(HAL)/Src/stm32c0xx_hal_uart_ex.c

ASM_SOURCES = Core/Startup/startup_stm32c031xx.s

# The CMSIS core headers (core_cm0plus.h, ...) are shared with the F401 project
C_INCLUDES = \
-ICore/Inc \
-I$(HAL)/Inc \
-I$(HAL)/Inc/Legacy \
-IDrivers/CMSIS/Device/ST/STM32C0xx/Include \
-I../Drivers/CMSIS/Include

CPU = -mcpu=cortex-m0plus -mthumb
C_DEFS = -DUSE_HAL_DRIVER -DSTM32C031xx

OPT = -Os
CFLAGS = $(CPU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -g -gdwarf-2 -MMD -MP
ASFLAGS = $(CPU) $(OPT) -Wall -fdata-sections -ffunction-sections

LDSCRIPT = STM32C031C6TX_FLASH.ld
LDFLAGS = $(CPU) -T$(LDSCRIPT) --specs=nano.specs \
-Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections -lc -lm

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(CP) -O binary -S $< $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
