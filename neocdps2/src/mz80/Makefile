# _____ ___ ____ ___ ____
# ____| | ____| | | |____|
# | ___| |____ ___| ____| | \ PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004.
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_LIB = mz80.a

EE_CFLAGS = -finline-functions -funroll-loops -DLSB_FIRST -DINLINE="static inline" -DFAST_MATH
EE_OBJS   =  z80.o
EE_LIBS += -lgcc -lkernel 

all : $(EE_LIB)



clean:
	rm -f *.o mz80.a $

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

