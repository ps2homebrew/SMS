#     _  ___  ___   ___  ___     ____    ___ ___ 
# |\  | |___ |   | |        |  /  ___|  |    ___|
# | \_| |___ |___| |___  ___| /  |   ___|   |___  Neocd/Ps2 Project.
#----------------------------------------------------------------------
# Copyright 2001-2004.
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_LIB = c68k.a
EE_CFLAGS =  -DLSB_FIRST -DINLINE="static inline" -DFAST_MATH 
EE_OBJS   = c68kexec.o c68k.o
EE_LIBS +=  -lgcc -lkernel 


all : $(EE_LIB)

gen68k: 
	gcc gen68k.c -o gen68k.exe
	.\gen68k.exe

clean:
	rm -f *.o c68k.a $

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

