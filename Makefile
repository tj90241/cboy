#
# Makefile for n64chain ROMs.
#
# n64chain: A (free) open-source N64 development toolchain.
# Copyright 2014-16 Tyler J. Stachecki <stachecki.tyler@gmail.com>
#
# This file is subject to the terms and conditions defined in
# 'LICENSE', which is part of this source code package.
#

ifdef SystemRoot
FIXPATH = $(subst /,\,$1)
RM = del /Q
else
FIXPATH = $1
RM = rm -f
endif

ROM_NAME = $(notdir $(CURDIR))

AS = $(call FIXPATH,$(CURDIR)/../tools/bin/mips64-elf-as)
AR = $(call FIXPATH,$(CURDIR)/../tools/bin/mips64-elf-gcc-ar)
CC = $(call FIXPATH,$(CURDIR)/../tools/bin/mips64-elf-gcc)
MAKE = $(call FIXPATH,$(CURDIR)/../tools/bin/make)
OBJCOPY = $(call FIXPATH,$(CURDIR)/../tools/bin/mips64-elf-objcopy)

CHECKSUM = $(call FIXPATH,$(CURDIR)/../tools/bin/checksum)
RSPASM = $(call FIXPATH,$(CURDIR)/../tools/bin/rspasm)

CFLAGS = -Wall -std=c99 -Wno-main -fno-strict-aliasing \
	-I../libn64/include -I../libn64 -I.

OPTFLAGS = -O3 -march=vr4300 -mabi=eabi -mgp32 -mlong32 \
	-flto -ffat-lto-objects -ffunction-sections -fdata-sections \
	-G4 -mno-extern-sdata -mgpopt -fselective-scheduling \
  -fsel-sched-pipelining -fsel-sched-pipelining-outer-loops

ASMFILES = $(call FIXPATH,\
)

CFILES = $(call FIXPATH,\
	src/audio.c \
	src/bootrom.c \
	src/cart.c \
	src/cartdesc.c \
	src/cartrom.c \
	src/cpu.c \
	src/input.c \
	src/main.c \
	src/mbc_boot.c \
	src/mbc.c \
	src/mbc_cam.c \
	src/mbc_huc1.c \
	src/mbc_huc3.c \
	src/mbc_mbc1.c \
	src/mbc_mbc2.c \
	src/mbc_mbc3.c \
	src/mbc_mbc5.c \
	src/mbc_mbc7.c \
	src/mbc_none.c \
	src/memory.c \
	src/serial.c \
	src/video.c \
)

UCODES = $(call FIXPATH,\
)

OBJFILES = \
	$(ASMFILES:.S=.o) \
	$(CFILES:.c=.o)

UCODEBINS = $(UCODES:.rsp=.bin)
DEPFILES = $(OBJFILES:.o=.d)

#
# Primary targets.
#
all: $(ROM_NAME).z64

$(ROM_NAME).z64: $(ROM_NAME).elf
	@echo $(call FIXPATH,"Building: $(ROM_NAME)/$@")
	@$(OBJCOPY) -O binary $< $@
	@$(CHECKSUM) $(call FIXPATH,../libn64/header.bin) $@

$(ROM_NAME).elf: libn64 $(OBJFILES)
	@echo $(call FIXPATH,"Building: $(ROM_NAME)/$@")
	@$(CC) $(CFLAGS) $(OPTFLAGS) -Wl,-Map=$(ROM_NAME).map -nostdlib \
		-T$(call FIXPATH,../libn64/rom.ld) -o $@ $(OBJFILES) \
		-L$(call FIXPATH,../libn64) -ln64

#
# Generic compilation/assembly targets.
#
$(call FIXPATH,src/graphics.o): $(call FIXPATH,src/graphics.S) $(call FIXPATH,src/graphics.bin)
%.o: %.S
	@echo $(call FIXPATH,"Assembling: $(ROM_NAME)/$<")
	@$(CC) $(CFLAGS) $(OPTFLAGS) -MMD -c $< -o $@

%.o: %.c
	@echo $(call FIXPATH,"Compiling: $(ROM_NAME)/$<")
	@$(CC) $(CFLAGS) $(OPTFLAGS) -MMD -c $< -o $@

%.bin: %.rsp
	@echo $(call FIXPATH,"Assembling: $(ROM_NAME)/$<")
	@$(RSPASM) -o $@ $<

.PHONY: libn64
libn64:
	@$(MAKE) -sC $(call FIXPATH,../libn64)

#
# Clean project target.
#
.PHONY: clean
clean:
	@echo "Cleaning $(ROM_NAME)..."
	@$(RM) $(ROM_NAME).map $(ROM_NAME).elf $(ROM_NAME).z64 \
		$(DEPFILES) $(OBJFILES) $(UCODEBINS)

#
# Use computed dependencies.
#
-include $(DEPFILES)

