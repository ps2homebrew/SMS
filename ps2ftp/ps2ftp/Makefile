EE_BIN = ps2ftp.elf
EE_OBJS = ps2ftp.o FSMan.o irxload.o aio/aioMc.o aio/aioCdvd.o aio/aioHdd.o aio/cdvd_rpc.o
EE_INCS += -I$(PS2DEV)/ps2ip/include -I$(PS2DRV)/common/include -I$(PS2DRV)/ee/libfileXio
EE_LDFLAGS += -L$(PS2DEV)/ps2ip/ee/lib -Lz:/projects/libhdd/ee/lib -L$(PS2DRV)/ee/libfileXio 
EE_LIBS += -lgcc -lkernel -lmc -lhdd -lps2ip -lfileXio

all:	$(EE_BIN)

clean:
	rm -f *.elf *.o *.a

rebuild: clean	all
	
include $(PS2LIB)/Makefile.pref
include $(PS2LIB)/Makefile.eeglobal

