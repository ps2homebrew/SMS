# _____ ___ ____ ___ ____
# ____| | ____| | | |____|
# | ___| |____ ___| ____| | \ PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004.
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

all : libcpu_68k.a
EE_LFLAGS = -DLSB_FIRST -finline-functions -ffast-math -DINLINE
EE_CFLAGS = -DLSB_FIRST -finline-functions -ffast-math -DINLINE
EE_GSRC   = neocd68k.c m68kcpu.c m68kops.c m68kopac.c m68kopdm.c m68kopnz.c
EE_OBJS   = m68kcpu.o m68kops.o m68kopac.o m68kopdm.o m68kopnz.o

MSRC = m68kmake.c
MEXE = m68kmake.exe

mc68000.a : $(EE_OBJS) $(EE_GSRC)
	$(EE_AR) cru ./../cpu_68k.a $(EE_OBJS)

$(EE_GSRC): $(MEXE)
	./$(MEXE)

$(MEXE): $(MSRC)
	gcc $(MSRC) -o $(MEXE)

clean:
	rm -f *.o mc68000.a $(MEXE)
	rm -f m68kops.c m68kopac.c m68kopdm.c m68kopnz.c

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal