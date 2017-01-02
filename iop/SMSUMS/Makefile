# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id: $

IOP_OBJS_DIR = obj/
IOP_BIN_DIR = bin/
IOP_SRC_DIR = src/
IOP_INC_DIR = include/

IOP_BIN  = bin/ums.irx
IOP_OBJS = obj/ums.o obj/ums_driver.o obj/ums_imports.o obj/ums_fat.o obj/ums_cache.o

IOP_CFLAGS  += -Wall -I$(PS2SDK)/iop/include -I$(PS2SDK)/common/include -mno-check-zero-division
IOP_LDFLAGS += -s

all: $(IOP_OBJS_DIR) $(IOP_BIN_DIR) $(IOP_BIN)
	xz --lzma2=nice=32 --check=crc32 $(IOP_BIN)
	bin2c $(IOP_BIN).xz UMS.dmp s_UMS
	rm $(IOP_BIN).xz

clean:
	rm -f -r $(IOP_OBJS_DIR) $(IOP_BIN_DIR)

rebuild: clean all

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
