# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$

EE_BIN = fstest.elf
EE_OBJS = fstest.o
EE_LIBS = -lc -ldebug -lkernel -lsyscall -lc

all: $(EE_BIN)

test: reset make_ps2http copy_ps2http all start

start: 
	ps2client execee host:fstest.elf

reset: clean
	ps2client reset

make_ps2http:
	make -C ../tenftp clean
	make -C ../tenftp

copy_ps2http:
	cp ../tenftp/bin/tenftp.irx ./tenftp.irx

clean:
	rm -f *.elf *.o *.a

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

