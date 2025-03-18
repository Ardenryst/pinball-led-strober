include Makefile.in

MCU:=attiny85
F_CPU:=8000000

TARGET          = firmware


CC      = $(AVR_TOOLS_PATH)/avr-gcc
CXX     = $(AVR_TOOLS_PATH)/avr-g++
OBJCOPY = $(AVR_TOOLS_PATH)/avr-objcopy
OBJDUMP = $(AVR_TOOLS_PATH)/avr-objdump
AR      = $(AVR_TOOLS_PATH)/avr-gcc-ar
SIZE    = $(AVR_TOOLS_PATH)/avr-size

AVR_DUDE= $(AVR_DUDE_PATH)/avrdude -C $(AVR_DUDE_PATH)/../etc/avrdude.conf 

OBJ_DIR = ./build/obj
APP_DIR := ./build

SOURCES_C := $(wildcard ./*.c)

OBJECTS += \
	$(SOURCES_C:%.c=$(OBJ_DIR)/%.o)

CFLAGS = -c -Os -flto
CFLAGS += -D F_CPU=$(F_CPU)

all: clean build $(APP_DIR)/$(TARGET).hex size

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@echo ">> Building obj [$<]..."
	$(CC) $(CFLAGS) -mmcu=$(MCU) $< -o $@ -I ./

$(APP_DIR)/$(TARGET).elf: $(OBJECTS)
	@mkdir -p $(@D)
	@echo ">> Building elf..."
	$(CC) $(LDFLAGS) -mmcu=$(MCU) $(OBJECTS) -o $(APP_DIR)/$(TARGET).elf

$(APP_DIR)/$(TARGET).hex: $(APP_DIR)/$(TARGET).elf
	@echo ">> converting to hex..."
	$(OBJCOPY) -O ihex -j .data -j .text $(APP_DIR)/$(TARGET).elf $(APP_DIR)/$(TARGET).hex

.PHONY: all build dump clean size program

$(OBJ_DIR)/%.i: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -mmcu=$(MCU) -E $< -o $@ $(addprefix -I , $(HEADER_DIRS))


dump: $(SOURCES:%.c=$(OBJ_DIR)/%.i)

size:
	$(OBJDUMP) -Pmem-usage $(APP_DIR)/$(TARGET).elf
	#$(SIZE) --mcu=$(MCU) -C $(TARGET).elf

program:
	$(AVR_DUDE) -p$(MCU) -P$(PORT) $(BAUD) -c$(PROGRAMMER) -v -Uflash:w:$(APP_DIR)/$(TARGET).hex:i

burn_fuses:
	$(AVR_DUDE) -p$(MCU) -P$(PORT) $(BAUD) -c$(PROGRAMMER) -v -e -Uefuse:w:0xFF:m -Uhfuse:w:0b11010111:m -Ulfuse:w:0xE2:m

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/* 
