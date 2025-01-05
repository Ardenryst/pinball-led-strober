include Makefile.in

MCU:=attiny85
F_CPU:=8000000

TARGET          = firmware


CC      = $(AVR_TOOLS_PATH)/avr-gcc
CXX     = $(AVR_TOOLS_PATH)/avr-g++
# AS      = $(AVR_TOOLS_PATH)/$(AS_NAME)
OBJCOPY = $(AVR_TOOLS_PATH)/avr-objcopy
OBJDUMP = $(AVR_TOOLS_PATH)/avr-objdump
AR      = $(AVR_TOOLS_PATH)/avr-gcc-ar
SIZE    = $(AVR_TOOLS_PATH)/avr-size
# NM      = $(AVR_TOOLS_PATH)/avr-nm

AVR_DUDE= $(AVR_DUDE_PATH)/avrdude -C $(AVR_DUDE_PATH)/../etc/avrdude.conf 

# CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror
OBJ_DIR  = ./build/obj
APP_DIR  := ./build

SOURCES_C  :=   $(wildcard ./*.c)

# SOURCES_CPP  := $(wildcard ./*.cpp)

OBJECTS  += \
	$(SOURCES_C:%.c=$(OBJ_DIR)/%.o)
# 	$(SOURCES_CPP:%.cpp=$(OBJ_DIR)/%.o)

#NOTE für vollen FP support in printf -Wl,-u,vfprintf -lprintf_flt -lm
#TODO -pedantic-errors -Wall -Wextra -Werror wieder einschalten und die ganzen Warungen zum Verstummen bringen
CFLAGS = -c -Os -flto
#TODO -flto funktioniert nicht, compiler crasht mal wieder. Dreckssoftware!
CFLAGS += -D F_CPU=$(F_CPU) -D BOARD_VER=$(BOARD_VER) -D BOARD_REV=$(BOARD_REV)

#TODO evaluate flags
# CXXFLAGS = -std=gnu++11 -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics

# LDFLAGS = -flto

all: clean build $(APP_DIR)/$(TARGET).hex size

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@echo ">> Building obj [$<]..."
	$(CC) $(CFLAGS) -mmcu=$(MCU) $< -o $@ -I ./

# $(OBJ_DIR)/%.o: %.cpp
# 	@mkdir -p $(@D)
# 	@echo ">> Building obj [$<]..."
# 	$(CXX) $(CFLAGS) $(CXXFLAGS) -mmcu=$(MCU) $< -o $@

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
	$(AVR_DUDE) -p$(MCU) -P$(PORT) $(BAUD) -c$(PROGRAMMER) -V -v -Uflash:w:$(APP_DIR)/$(TARGET).hex:i

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/* 
