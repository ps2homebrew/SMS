IOP_SRC_DIR  = ./
IOP_OBJS_DIR = ./

IOP_BIN  = SMSAudrv.irx
IOP_OBJS = SMSAudrv.o SMSAudrv_US.o SMSAudrv_LInUS.o imports.o

IOP_LDFLAGS = -s
IOP_CFLAGS  += -I./ -Wall -fno-builtin -D_IOP

all: $(IOP_BIN)

clean:
	rm -f *.elf *.o *.a *.irx

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
