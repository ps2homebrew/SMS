EE_INCS := $(EE_INCS) -I$(LIBHDD)/ee/include -I$(LIBHDD)/common/include \
	   -I$(PS2DRV)/common/include -I$(PS2DRV)/ee/libfileXio \
	   -I$(PS2LIB)/ee/include -I$(PS2LIB)/common/include -I.

D_EE -mips3 -mcpu=r5900 -ffreestanding -fno-builtin \
		-fshort-double -mlong64 -mhard-float -mno-abicalls -O2 -EL -Wall

EE_CFLAGS  :=	-mips3 -mcpu=r5900 -ffreestanding -fno-builtin -G0 \
		-fshort-double -mlong64 -mhard-float -mno-abicalls -O2 -EL -Wall \
		-c $(EE_INCS) $(EE_CFLAGS)
EE_ASFLAGS := -EL -G0 $(EE_ASFLAGS)
ifeq ($(DEBUG),1)
LOADADDR  = 0xd00000
STACKADDR = 0xde0000
LDPARAMS := -Wl,--defsym -Wl,_stack_size=0x8000 -Wl,--defsym -Wl,_stack=$(STACKADDR)
EE_LDFLAGS += -Wl,-Ttext -Wl,$(LOADADDR) $(LDPARAMS)
endif

EE_LDFLAGS := $(EE_LDFLAGS) -L$(LIBHDD)/ee/lib -L$(PS2DRV)/ee/libfileXio \
		-L$(PS2LIB)/ee/lib -T$(PS2LIB)/ee/startup/linkfile -nostartfiles
EE_BIN = ../bin/HDDMENU.ELF
EE_OBJS = hddmenu.o hw.o hardware.o

EE_LIBS = -lmc -lpad -lhdd -lfileXio

all: $(EE_BIN)

clean:
	rm -f *.elf *.o *.a

include $(PS2LIB)/Makefile.pref
include $(PS2LIB)/Makefile.eeglobal
