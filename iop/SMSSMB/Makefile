# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2005, ps2dev - http://www.ps2dev.org
# Licenced under GNU Library General Public License version 2
# Review ps2sdk README & LICENSE files for further details.
#
# $Id: Makefile 1187 2005-06-27 17:34:43Z lukasz $

IOP_OBJS_DIR = obj/
IOP_BIN_DIR  = bin/
IOP_SRC_DIR  = src/
IOP_INC_DIR  = include/

IOP_BIN  = bin/smb.irx
IOP_OBJS = $(IOP_OBJS_DIR)SMSMB.o

IOP_CFLAGS  += -Wall -fno-builtin-printf -fno-builtin-memcpy -D_IOP -I$(PS2SDK)/iop/include
IOP_LDFLAGS += -s

all: $(IOP_OBJS_DIR) $(IOP_BIN_DIR) $(IOP_BIN)

clean:
	rm -f -r $(IOP_OBJS_DIR) $(IOP_BIN_DIR)

rebuild:
	clean all

include $(PS2SDK)/Defs.make
include $(PS2SDK)/iop/Rules.make
include $(PS2SDK)/iop/Rules.release
