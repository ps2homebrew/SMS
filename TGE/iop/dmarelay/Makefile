IOP_BIN  = dmarelay.irx
IOP_OBJS = main.o ata.o smap.o imports.o

IOP_CFLAGS  += -Wall -fno-builtin
IOP_LDFLAGS += -s

all: $(IOP_BIN)

include $(PS2DRV)/Defs.make
include $(PS2DRV)/iop/Rules.make
