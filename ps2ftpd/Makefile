# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004.
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

IOP_OBJS_DIR = obj/
IOP_BIN_DIR = bin/
IOP_SRC_DIR = src/
IOP_INC_DIR = include/

IOP_BIN = bin/ps2ftpd.irx
IOP_OBJS = obj/main.o obj/FileSystem.o obj/FtpServer.o obj/FtpClient.o obj/imports.o

IOP_CFLAGS += -Wall -fno-builtin
# -DDEBUG
IOP_LDFLAGS += -s
IOP_INCS += -I$(PS2SDKSRC)/iop/tcpip/tcpip/include -I$(PS2SDKSRC)/iop/tcpip/dns/include -I$(PS2SDKSRC)/iop/system/iopmgr/include

all: $(IOP_OBJS_DIR) $(IOP_BIN_DIR) $(IOP_BIN)

clean:
	rm -f -r $(IOP_OBJS_DIR) $(IOP_BIN_DIR)

# this is just a testcase, might not work on your setup
runhd:
	ps2client reset
	ps2client -t 1 execiop mc:ps2atad.irx
	ps2client -t 8 execiop mc:ps2hdd.irx -n 20
	ps2client -t 1 execiop mc:ps2fs.irx -n 12
	ps2client -t 1 execiop mc:ps2netfs.irx
	ps2client mount pfs0: hdd:__boot
	ps2client execiop host:bin/ps2ftpd.irx

run:
	ps2client reset
	ps2client execiop host:bin/ps2ftpd.irx

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
