IOP_BIN = ps2ftp.irx
IOP_OBJS = ps2ftp.o snprintf.o mcman.o
IOP_INCS += -I$(PS2DEV)/ps2ip/include
IOP_LDFLAGS += -L$(PS2DEV)/ps2ip/iop/lib
IOP_LIBS += -liop_ps2ip

all:	$(IOP_BIN)

clean:
	rm -f *.elf *.o *.a *.irx

include $(PS2LIB)/Makefile.pref
include $(PS2LIB)/Makefile.iopglobal


