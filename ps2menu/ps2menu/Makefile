EE_INCS := $(EE_INCS) -I$(LIBHDD)/ee/include -I$(LIBHDD)/common/include \
	   -I$(PS2DRV)/common/include -I$(PS2DRV)/ee/libfileXio \
	   -I$(PS2LIB)/ee/include -I$(PS2LIB)/common/include \
	   -I$(PS2DEV)/sbv-1.0-lite/include -I.

D_EE -mips3 -mcpu=r5900 -ffreestanding -fno-builtin \
		-fshort-double -mlong64 -mhard-float -mno-abicalls -O2 -EL -Wall

EE_CFLAGS  :=	-mips3 -mcpu=r5900 -ffreestanding -fno-builtin -G0 \
		-fshort-double -mlong64 -mhard-float -mno-abicalls -O2 -EL -Wall \
		-c $(EE_INCS) $(EE_CFLAGS)
EE_ASFLAGS := -EL -G0 $(EE_ASFLAGS)

EE_LDFLAGS := $(EE_LDFLAGS) -s -L$(LIBHDD)/ee/lib -L$(PS2DRV)/ee/libfileXio \
		-L$(PS2LIB)/ee/lib -L$(PS2DEV)/sbv-1.0-lite/lib \
		-T$(PS2LIB)/ee/startup/linkfile -nostartfiles
EE_BIN = ../bin/ps2menu.elf
EE_OBJS = ps2menu.o hw.o hardware.o iomanx.o filexio.o ps2atad.o ps2dev9.o \
		ps2hdd.o ps2fs.o loader.o poweroff.o iuntar.o ps2menupcx.o

EE_LIBS = -lmc -lpad -lhdd -lfileXio -lsbv_patches

all: $(EE_BIN)

iomanx.s:
	bin2s $(PS2DRV)/iop/iomanX/iomanX.irx iomanx.s iomanx_irx

filexio.s:
	bin2s $(PS2DRV)/iop/fileXio/fileXio.irx filexio.s filexio_irx

ps2atad.s:
	bin2s $(PS2DRV)/iop/ps2atad/ps2atad.irx ps2atad.s ps2atad_irx

ps2dev9.s:
	bin2s $(PS2DRV)/iop/ps2dev9/ps2dev9.irx ps2dev9.s ps2dev9_irx

ps2fs.s:
	bin2s $(LIBHDD)/iop/modules/ps2fs.irx ps2fs.s ps2fs_irx

ps2hdd.s:
	bin2s $(LIBHDD)/iop/modules/ps2hdd.irx ps2hdd.s ps2hdd_irx

poweroff.s:
	bin2s $(LIBHDD)/iop/poweroff/poweroff.irx poweroff.s poweroff_irx

iuntar.s:
	bin2s $(PS2DRV)/iop/iuntar/iuntar.irx iuntar.s iuntar_irx

loader.s:
	bin2s ./loader/loader.elf loader.s loader_elf

ps2menupcx.s:
	bin2s images/ps2menu.PCX ps2menupcx.s ps2menu_pcx

#mainlogo.s:
#	bin2s images/mainlogo.pcx mainlogo.s mainlogo_pcx

clean:
	rm -f *.elf *.o *.a iomanx.s filexio.s ps2atad.s ps2dev9.s ps2fs.s ps2hdd.s poweroff.s iuntar.s loader.s ps2menupcx.s

include $(PS2LIB)/Makefile.pref
include $(PS2LIB)/Makefile.eeglobal
