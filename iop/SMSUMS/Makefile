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

IOP_CFLAGS  += -Wall -I$(PS2SDK)/iop/include -mno-check-zero-division
IOP_LDFLAGS += -s

all: $(IOP_OBJS_DIR) $(IOP_BIN_DIR) $(IOP_BIN)

clean:
	rm -f -r $(IOP_OBJS_DIR) $(IOP_BIN_DIR)

rebuild: clean all

include $(PS2SDK)/Defs.make
include $(PS2SDK)/iop/Rules.make
include $(PS2SDK)/iop/Rules.release
