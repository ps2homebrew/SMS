#     _  ___  ___   ___  ___     ____    ___ ___ 
# |\  | |___ |   | |        |  /  ___|  |    ___|
# | \_| |___ |___| |___  ___| /  |   ___|   |___  Neocd/Ps2 Project.
#----------------------------------------------------------------------
# Copyright 2004 - Olivier "evilo" Biot <evilo13@gmail.com>
# Released under GNU license
# Review neocd/ps2 COPYING & LICENSE files for further details.
#---------------------------------------------------------------------- 
# Cygwin Env
# GCC 3.2.2
# PS2SDK 1.2
#---------------------------------------------------------------------- 
# Compilation Flags
# -DCPU68K_USE_MUSASHI   use musashi 68k core (working core)
# -DCPU68K_USE_C68K	 use stef 68k core (wip, not working)
#
# -DCPUZ80_MAMEZ80_CORE  use mame z80 core (working core)
# -DCPUZ80_CZ80_CORE	 use stef z80 core (wip, not working)
#
#---------------------------------------------------------------------- 

EE_BIN = neocd.elf
	    
EE_OBJS =  neocd.o pd4990a.o \
	   cpu_68k/cpu68k.o \
	   cpu_z80/z80intrf.o cpu_z80/cz80/cz80.o cpu_z80/mamez80/z80.o \
	   misc/misc.o misc/timer.o \
	   input/input.o \
	   memory/memory.o \
	   save/mc.o save/ngcdIcn.o \
	   gui/menu.o gui/ps2print.o \
	   sound/sound.o sound/2610intf.o sound/ay8910.o \
	   sound/fm.o sound/ymdeltat.o \
	   sound/timer.o static/isjpcmIrx.o sound/sjpcm_rpc.o \
	   video/video.o video/draw_fix.o \
	   cdrom/cdrom.o static/cdvdIrx.o cdrom/cdvd_rpc.o cdaudio/cdaudio.o \
	   gs/gfxpipe.o gs/hw.o gs/gs.o

EE_CFLAGS  += -funroll-loops -finline-functions -freorder-blocks -funsigned-char \
	     -fno-exceptions -mhard-float -EL -ffast-math -DINLINE="inline" -DLSB_FIRST \
	     -DCPU68K_USE_MUSASHI -DCPUZ80_MAMEZ80_CORE

EE_INCS    += -I. -I$(PS2SDK)/sbv/include/
	     
EE_LDFLAGS += -L$(PS2SDK)/sbv/lib -L./cpu_68k -L./cdrom
EE_LIBS    += -lpatches -lcpu_68k -lcdvdfs -lc -lm -lcdvd -lmc -lpad -lgcc


CP_CMD    = cp
BIN_DIR =../bin


all: $(EE_BIN) 
	$(EE_STRIP) $(EE_BIN)
	$(CP_CMD) $(EE_BIN) $(BIN_DIR)/$(EE_BIN)

clean:
	rm -f -R $(EE_BIN) $(EE_OBJS)
	
reset :
	ps2client -h 192.168.1.110 reset

run :
	ps2client -h 192.168.1.110 execee host:$(EE_BIN)
	

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

