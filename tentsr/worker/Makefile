# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$

EE_BIN = ../bin/tentsr_worker.elf
EE_INCS += -I$(PS2SDKSRC)/ee/sbv/include
EE_OBJS =  worker.o helpers.o
EE_LIBS = -lc -ldebug -lkernel -lpatches
EE_LDFLAGS += -L$(PS2SDKSRC)/ee/sbv/lib


all: $(EE_BIN)

clean: 
	rm -f *.elf *.o *.a


include $(PS2SDK)/samples/Makefile.pref
include ../Makefile.eeglobal_nolinkcmd

$(EE_BIN) : $(EE_OBJS)
	$(EE_CC) -nostartfiles -Tlinkfile $(EE_LDFLAGS) \
		-o $(EE_BIN) $(EE_OBJS) $(EE_LIBS)

# $(PS2SDK)/ee/startup/crt0.o   $(PS2SDK)/ee/startup/crt0.o