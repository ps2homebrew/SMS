#     _  ___  ___   ___  ___     ____    ___ ___ 
# |\  | |___ |   | |        |  /  ___|  |    ___|
# | \_| |___ |___| |___  ___| /  |   ___|   |___  Neocd/Ps2 Project.
#----------------------------------------------------------------------
# Copyright 2004 - Olivier "evilo" Biot <evilo13@gmail.com>
# Released under GNU license
# Review neocd/ps2 README & LICENSE files for further details.
#---------------------------------------------------------------------- 
# Cygwin Env
# GCC 3.2.2
# PS2SDK 1.2
#---------------------------------------------------------------------- 
# Compilation Flags
# -DCPU68K_USE_C68K	 use stef 68k core (wip, not complete yet)
# -DCPU68K_USE_MUSASHI   use musashi 68k core
#
# -DUSE_MAMEZ80   	use mame z80 core (+)
# -DUSE_MULTIZ80	multi-z80 core seems to be broken
#---------------------------------------------------------------------- 

EE_BIN = neocd.elf

EE_PROBJS = mc68000/mc68000.a c68k/c68k.a mz80/mz80.a \
 	    cdrom/libcdvdfs.a $(PS2SDK)/sbv/lib/libpatches.a
 	    
EE_OBJS =  neocd.o pd4990a.o \
	   mz80/mz80interf.o cpu/cpu68k.o \
	   misc/misc.o \
	   input/input.o \
	   memory/memory.o \
	   save/mc.o save/ngcdIcn.o \
	   menu/menu.o \
	   sound/sound.o sound/2610intf.o sound/ay8910.o sound/fm.o sound/ymdeltat.o \
	   sound/timer.o sound/isjpcmIrx.o sound/sjpcm_rpc.o \
	   video/video.o video/draw_fix.o \
	   cdrom/cdrom.o cdrom/cdvdIrx.o cdrom/cdvd_rpc.o cdaudio/cdaudio.o \
	   gs/gfxpipe.o gs/hw.o gs/gs.o

EE_CFLAGS += -funroll-loops -finline-functions -freorder-blocks -funsigned-char -mhard-float -EL -ffast-math -DINLINE="inline" -DLSB_FIRST -DCPU68K_USE_MUSASHI -DUSE_MAMEZ80
EE_LIBS   += $(EE_PROBJS) -lc -lm -lcdvd -lmc -lpad -lgcc
EE_INCS   += -I. -I$(PS2SDK)/sbv/include/

BMP2C = bmp2c
CP    = cp
BIN_DIR =../bin
BMP_DIR = ./video/resources


all: makez80_foo make_bmp $(EE_BIN) 
	$(EE_STRIP) $(EE_BIN)
	$(CP) $(EE_BIN) $(BIN_DIR)/$(EE_BIN)

makez80_foo:
	gcc -o z80/makez80 -Iz80 -I. z80/makez80.c
	z80/makez80.exe z80/z80.c -c
	touch makez80_foo

make_bmp:
	$(BMP_DIR)/$(BMP2C) $(BMP_DIR)/loading256x256.bmp loading > $(BMP_DIR)/loading.h
	$(BMP_DIR)/$(BMP2C) $(BMP_DIR)/splash256x256.bmp  splash  > $(BMP_DIR)/splash.h
	
clean:
	rm -f -R *.elf *.o
	
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

