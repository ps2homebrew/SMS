IOP_BIN  = ../SJPCMSTR.IRX
IOP_OBJS = sjpcm_irx.o hw.o imports.o

IOP_CFLAGS  += -Wall -fno-builtin
IOP_LDFLAGS += -s

all: $(IOP_BIN)

include $(PS2DRV)/Defs.make
include $(PS2DRV)/iop/Rules.make
