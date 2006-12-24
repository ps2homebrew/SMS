IOP_BIN     = SMSMB.irx
IOP_OBJS    = SMSMB.o
IOP_LDFLAGS = -Wl,-s

all: $(IOP_BIN)

clean:
	rm -f *.elf *.o *.a *.irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal
