# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004.
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = neocd.elf
EE_PROBJS = mc68000/mc68000.a 
EE_OBJS = z80/z80.o z80/z80intrf.o neocd.o pd4990a.o misc/misc.o input/input.o memory/memory.o sound/sound.o sound/streams.o sound/2610intf.o sound/ay8910.o sound/fm.o  sound/timer.o sound/ymdeltat.o video/video.o video/draw_fix.o cdrom/cdrom.o cdaudio/cdaudio.o gs/gs.o gs/g2.o gs/ps2_asm.o gs/dma_asm.o gs/gs_asm.o gs/bitmap.o gs/f_arial.o 
EE_CFLAGS += -DLSB_FIRST 
#EE_CFLAGS += -DLSB_FIRST -DDEBUG
BMP2C = bmp2c
BMP_DIR = ./video/resources

EE_LIBS += $(EE_PROBJS) -lgcc -lc -lm -lcdvd -lmc -lpad -I. -Iz80 -Imc68000 


all: makez80_foo $(EE_BIN)

makez80_foo:
	gcc -o z80/makez80 -Iz80 -I. z80/makez80.c
	z80/makez80.exe z80/z80.c -c
	touch makez80_foo

bmp:
	$(BMP_DIR)/$(BMP2C) $(BMP_DIR)/loading.bmp loading > $(BMP_DIR)/loading.h
	$(BMP_DIR)/$(BMP2C) $(BMP_DIR)/splash.bmp splash > $(BMP_DIR)/splash.h
	$(BMP_DIR)/$(BMP2C) $(BMP_DIR)/insert.bmp insert > $(BMP_DIR)/insert.h

clean:
	rm -f -R *.elf *.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
