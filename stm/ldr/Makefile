EXE_NAME=lp_ldr
PRODUCT =lp
VER_MAJ = 1
VER_MIN = 0
VER_PATCH = 0
MAKE_BINARY=yes

TCHAIN = arm-none-eabi-
MCPU += -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
CDIALECT = gnu99
OPT_LVL = 2
DBG_OPTS = -gdwarf-2 -ggdb -g

CFLAGS   += -fvisibility=hidden -funsafe-math-optimizations -fdata-sections -ffunction-sections -fno-move-loop-invariants
CFLAGS   += -fmessage-length=0 -fno-exceptions -fno-common -fno-builtin -ffreestanding
CFLAGS   += -fsingle-precision-constant
CFLAGS   += $(C_FULL_FLAGS)
CFLAGS   += -Werror

CXXFLAGS += -fvisibility=hidden -funsafe-math-optimizations -fdata-sections -ffunction-sections -fno-move-loop-invariants
CXXFLAGS += -fmessage-length=0 -fno-exceptions -fno-common -fno-builtin -ffreestanding
CXXFLAGS += -fvisibility-inlines-hidden -fuse-cxa-atexit -felide-constructors -fno-rtti
CXXFLAGS += -fsingle-precision-constant
CXXFLAGS += $(CXX_FULL_FLAGS)
CXXFLAGS += -Werror

LDFLAGS  += -specs=nano.specs
LDFLAGS  += -Wl,--gc-sections
LDFLAGS  += -Wl,--print-memory-usage

EXT_LIBS +=c nosys

PPDEFS += USE_STDPERIPH_DRIVER STM32F40_41xxx STM32F407xx HSE_VALUE=12000000 FW_HEADER_REDUCED FW_TYPE=FW_LDR CFG_SYSTEM_SAVES_NON_NATIVE_DATA=1 DEV=\"LP_STM_LDR\"

INCDIR += .
INCDIR += ../common
INCDIR += ../common/adc
INCDIR += ../common/config_system
INCDIR += ../common/CMSIS
INCDIR += ../common/crc
INCDIR += ../common/fw_header
INCDIR += ../common/i2c
INCDIR += ../common/STM32_USB_Device_Library/Core
INCDIR += ../common/STM32_USB_OTG_Driver
INCDIR += ../common/STM32F4xx_StdPeriph_Driver/inc
INCDIR += ../common/usb

SOURCES += $(call rwildcard, ../common/adc, *.c *.S *.s)
SOURCES += $(call rwildcard, ../common/CMSIS, *.c *.S *.s)
SOURCES += $(call rwildcard, ../common/config_system, *.c *.S *.s)
SOURCES += $(call rwildcard, ../common/crc, *.c *.S *.s)
SOURCES += $(call rwildcard, ../common/i2c, *.c *.S *.s)
SOURCES += $(call rwildcard, ../common/fw_header, *.c *.S *.s)
SOURCES += $(call rwildcard, ../common/STM32_USB_Device_Library/Core, *.c *.S *.s)
SOURCES += $(call rwildcard, ../common/STM32_USB_OTG_Driver, *.c *.S *.s)
SOURCES += $(call rwildcard, ../common/STM32F4xx_StdPeriph_Driver, *.c *.S *.s)
SOURCES += $(call rwildcard, ../common/usb, *.c *.S *.s)
SOURCES += $(wildcard ../common/*.c)
SOURCES += $(wildcard *.c)
LDSCRIPT += ../ldscript/ldr.ld

BINARY_SIGNED = $(BUILDDIR)/$(EXE_NAME)_signed.bin
BINARY_MERGED = $(BUILDDIR)/$(PRODUCT)_ldr_preldr_signed.bin
SIGN = $(BUILDDIR)/sign
ARTEFACTS += $(BINARY_SIGNED)

PRELDR_SIGNED = ../preldr/build/$(PRODUCT)_preldr_signed.bin
EXT_MAKE_TARGETS = $(PRELDR_SIGNED)

include ../common/core.mk

$(SIGN): ../sign/sign.c
	@gcc $< -o $@

$(BINARY_SIGNED): $(BINARY) $(SIGN)
	@$(SIGN) $(BINARY) $@ 49152 \
		prod=$(PRODUCT) \
		prod_name=$(EXE_NAME) \
		ver_maj=$(VER_MAJ) \
		ver_min=$(VER_MIN) \
		ver_pat=$(VER_PATCH) \
		build_ts=$$(date -u +'%Y%m%d_%H%M%S')

$(BINARY_MERGED): $(EXT_MAKE_TARGETS) $(BINARY_SIGNED)
	@echo " [Merging binaries] ..." {$@}
	@cp -f $< $@
	@dd if=$(BINARY_SIGNED) of=$@ bs=1 oflag=append seek=16384 status=none

.PHONY: composite
composite: $(BINARY_MERGED)

.PHONY: clean
clean: clean_ext_targets

.PHONY: clean_ext_targets
clean_ext_targets:
	$(foreach var,$(EXT_MAKE_TARGETS),$(MAKE) -C $(subst build/,,$(dir $(var))) clean;)

.PHONY: $(EXT_MAKE_TARGETS)
$(EXT_MAKE_TARGETS):
	@$(MAKE) -C $(subst build/,,$(dir $@)) --no-print-directory
	
#####################
### FLASH & DEBUG ###
#####################

flash: $(BINARY_SIGNED)
	openocd -d0 -f ../target/stm32f4xx.cfg -c "program $< 0x08004000 verify reset exit" 

flash_all: $(BINARY_MERGED)
	openocd -d0 -f ../target/stm32f4xx.cfg -c "program $< 0x08000000 verify reset exit" 

program: $(BINARY_SIGNED)
	@usb_dfu_flasher $< LP_STM boot

debug:
	@echo "file $(EXECUTABLE)" > .gdbinit
	@echo "set auto-load safe-path /" >> .gdbinit
	@echo "set confirm off" >> .gdbinit
	@echo "target remote | openocd -c \"gdb_port pipe\" -f ../target/stm32f4xx.cfg" >> .gdbinit
	@arm-none-eabi-gdb -q -x .gdbinit