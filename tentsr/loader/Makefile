# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$

EE_BIN = ../bin/tentsr.elf
EE_INCS += -I$(PS2SDKSRC)/ee/sbv/include
EE_OBJS =  loader.o
EE_LIBS = -lc -ldebug -lkernel  -lpatches
EE_LDFLAGS += -L$(PS2SDKSRC)/ee/sbv/lib


all: $(EE_BIN)

clean: 
	rm -f *.elf *.o *.a


include $(PS2SDK)/samples/Makefile.pref
include ../Makefile.eeglobal_nolinkcmd

$(EE_BIN) : $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o
	$(EE_CC) -nostartfiles -Tlinkfile $(EE_LDFLAGS) \
		-o $(EE_BIN) $(PS2SDK)/ee/startup/crt0.o $(EE_OBJS) $(EE_LIBS)

