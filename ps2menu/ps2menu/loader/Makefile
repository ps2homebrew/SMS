EE_INCS := $(EE_INCS) -I$(LIBHDD)/ee/include -I$(LIBHDD)/common/include \
	   -I$(PS2DRV)/common/include -I$(PS2DRV)/ee/libfileXio \
	   -I$(PS2LIB)/ee/include -I$(PS2LIB)/common/include \
	   -I$(PS2DEV)/sbv-1.0-lite/include -I.

EE_CFLAGS  := -mips3 -mcpu=r5900 -ffreestanding -fno-builtin -G0 \
		-fshort-double -mlong64 -mhard-float -mno-abicalls -O2 -EL -Wall \
		-c $(EE_INCS) $(EE_CFLAGS)
EE_ASFLAGS := -EL -G0 $(EE_ASFLAGS)
LOADADDR  = 0x90000
STACKADDR = 0xb0000
ifeq ($(DEBUG),1)
LOADADDR  = 0x1700000
STACKADDR = 0x1720000
endif
LDPARAMS := -Wl,--defsym -Wl,_stack_size=0x8000 -Wl,--defsym -Wl,_stack=$(STACKADDR)
EE_LDFLAGS += -Wl,-Ttext -Wl,$(LOADADDR) -s $(LDPARAMS)

EE_LDFLAGS := $(EE_LDFLAGS) -L$(LIBHDD)/ee/lib -L$(PS2DRV)/ee/libfileXio \
		-L$(PS2LIB)/ee/lib -L$(PS2DEV)/sbv-1.0-lite/lib \
		-T$(PS2LIB)/ee/startup/linkfile -nostartfiles
EE_BIN = loader.elf
EE_OBJS = loader.o fakehost.o poweroff.o

EE_LIBS = -lmc -lhdd -lfileXio -lsbv_patches

all: $(EE_BIN)

fakehost.s:
	bin2s $(PS2DRV)/iop/fakehost/fakehost.irx fakehost.s fakehost_irx

poweroff.s:
	bin2s $(LIBHDD)/iop/poweroff/poweroff.irx poweroff.s poweroff_irx

clean:
	rm -f *.elf *.o *.a *.s

include $(PS2LIB)/Makefile.pref
include $(PS2LIB)/Makefile.eeglobal
