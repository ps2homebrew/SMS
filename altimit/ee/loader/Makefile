EE_BIN = ./loader.elf
EE_OBJS = loader.o fakehost.o poweroff.o

EE_INCS := $(EE_INCS) -I$(PS2DEV)/sbv-1.0-lite/include -I.

EE_CFLAGS  := -fno-exceptions -march=5900 -ffreestanding -fno-builtin -G0 \
		-fshort-double -mlong64 -mhard-float -mno-abicalls -O2 -EL -Wall \
		-c $(EE_INCS) $(EE_CFLAGS)
EE_ASFLAGS := -EL -G0 $(EE_ASFLAGS)

BIN2S = $(PS2SDK)/bin/bin2s

LOADADDR  = 0x90000
STACKADDR = 0xb0000
ifeq ($(DEBUG),1)
LOADADDR  = 0x1700000
STACKADDR = 0x1720000
endif

LDPARAMS := -Wl,--defsym -Wl,_stack_size=0x8000 -Wl,--defsym -Wl,_stack=$(STACKADDR)
EE_LDFLAGS += -Wl,-Ttext -Wl,$(LOADADDR) -s $(LDPARAMS)

EE_LDFLAGS := $(EE_LDFLAGS) -L$(PS2DEV)/sbv-1.0-lite/lib

EE_LIBS = -lkernel -lpoweroff -lmc -lhdd -lfileXio -lsbv_patches -lsyscall -lgcc -lc

all: $(EE_BIN)

fakehost.s:
	$(BIN2S) $(PS2SDK)/iop/irx/fakehost.irx fakehost.s fakehost_irx

poweroff.s:
	$(BIN2S) $(PS2SDK)/iop/irx/poweroff.irx poweroff.s poweroff_irx

clean:
	rm -f *.elf *.o *.a *.s

include ../../Makefile.pref
include ../../Makefile.eeglobal
