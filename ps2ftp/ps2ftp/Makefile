EE_BIN = ps2ftp.elf
EE_OBJS = ps2ftp.o FSMan.o aio/aioMc.o iomanx.o ps2dev9.o ps2smap.o ps2ip.o ps2ips.o
EE_INCS += -I$(PS2DEV)/ps2ip/include -I$(PS2DRV)/common/include -I$(PS2DRV)/ee/libfileXio
EE_LDFLAGS += -L$(PS2DEV)/ps2ip/ee/lib -Lz:/projects/libhdd/ee/lib -L$(PS2DRV)/ee/libfileXio 
EE_LIBS += -lps2ip -lkernel -lgcc -lmc 

all-: $(EE_BIN)
	

all: $(EE_BIN)
	ee-strip $(EE_BIN)
	#@bash sjcrunch $(EE_BIN) $(EE_BIN)

iomanx.s:
	bin2s $(PS2DEV)/ps2drv/iop/iomanX/iomanx.irx iomanx.s iomanx_irx
	
ps2dev9.s: 
	bin2s modules/ps2dev9.irx ps2dev9.s ps2dev9_irx
	
ps2smap.s: 
	bin2s modules/ps2smap.irx ps2smap.s ps2smap_irx
	
ps2ip.s: 
	bin2s $(PS2DEV)/ps2ip/iop/bin/ps2ip.irx ps2ip.s ps2ip_irx
	
ps2ips.s:
	bin2s $(PS2DEV)/ps2ip/iop/ps2ips/ps2ips.irx ps2ips.s ps2ips_irx
	
clean:
	rm -f *.elf *.o *.a *.s
	rm -f aio/*.o

rebuild: clean	all
	
include $(PS2LIB)/Makefile.pref
include $(PS2LIB)/Makefile.eeglobal

