EE_BIN = ../bin/altimit.elf
EE_OBJS = altimit.o altimitFS.o abstractFS.o altimitGS.o altimitGUI.o altimitPAD.o \
			loader.o buttons.o lucida.o

#
# "make LOADIRX=1" to build modules into final elf executable
#
ifeq ($(LOADIRX),1)
EE_OBJS += poweroff.o iomanx.o filexio.o ps2dev9.o ps2ip.o ps2smap.o ps2host.o ps2netfs.o \
			dns.o ps2http.o ps2atad.o ps2hdd.o ps2fs.o cdvd.o ps2mouse.o ps2kbd.o
EE_INCS += -DLOADIRX=$(LOADIRX)
endif

EE_INCS += -fno-exceptions -I../include -I$(SBVLITE)/include \
		-I$(LIBCDVD)/ee -I$(LIBCDVD)/common -I$(GSLIB)/source
EE_LDFLAGS += -s -L$(LIBCDVD)/lib -L$(SBVLITE)/lib -L$(GSLIB)/lib
EE_LIBS += -lgs -lhdd -lpoweroff -lmouse -lkbd -lc -lgcc -lmc -lpad -lfileXio -lcdvdfs \
		-ldebug -lsbv_patches -lc -lsupc++ -lsyscall

all-: $(EE_BIN)
	sjcrunch ../bin/altimit.elf ../bin/altimitlzo.elf

all: $(EE_BIN)
	ee-strip $(EE_BIN)

loader.s:
	bin2s ./loader/loader.elf loader.s loader_elf

buttons.s:
	bin2s ../bin/buttons.iif buttons.s buttons_iif

lucida.s:
	bin2s ../bin/lucida.fnt lucida.s lucida_fnt

ifeq ($(LOADIRX),1)
poweroff.s:
	bin2s $(PS2SDK)/iop/irx/poweroff.irx poweroff.s poweroff_irx

iomanx.s:
	bin2s $(PS2SDK)/iop/irx/iomanX.irx iomanx.s iomanx_irx

filexio.s:
	bin2s $(PS2SDK)/iop/irx/fileXio.irx filexio.s filexio_irx

ps2dev9.s:
	bin2s $(PS2SDK)/iop/irx/ps2dev9.irx ps2dev9.s ps2dev9_irx

ps2ip.s:
	bin2s $(PS2SDK)/iop/irx/ps2ip.irx ps2ip.s ps2ip_irx

ps2smap.s:
	bin2s $(PS2DEV)/ps2eth/smap/ps2smap.irx ps2smap.s ps2smap_irx

ps2host.s:
	bin2s $(PS2SDK)/iop/irx/ps2host.irx ps2host.s ps2host_irx

ps2netfs.s:
	bin2s $(PS2SDK)/iop/irx/ps2netfs.irx ps2netfs.s ps2netfs_irx

dns.s:
	bin2s $(PS2SDK)/iop/irx/dns.irx dns.s dns_irx

ps2http.s:
	bin2s $(PS2SDK)/iop/irx/ps2http.irx ps2http.s ps2http_irx

ps2atad.s:
	bin2s $(PS2SDK)/iop/irx/ps2atad.irx ps2atad.s ps2atad_irx

ps2hdd.s:
	bin2s $(PS2SDK)/iop/irx/ps2hdd.irx ps2hdd.s ps2hdd_irx

ps2fs.s:
	bin2s $(PS2SDK)/iop/irx/ps2fs.irx ps2fs.s ps2fs_irx

#cdvd.s:
#	bin2s ../bin/SYS-MODULES/CDVD.IRX cdvd.s cdvd_irx
cdvd.s:
	bin2s $(LIBCDVD)/lib/cdvd.irx cdvd.s cdvd_irx

ps2mouse.s:
	bin2s $(PS2SDK)/iop/irx/ps2mouse.irx ps2mouse.s ps2mouse_irx

ps2kbd.s:
	bin2s $(PS2SDK)/iop/irx/ps2kbd.irx ps2kbd.s ps2kbd_irx

endif

clean:
	rm -f *.elf *.o *.a *.s

include ../Makefile.pref
include ../Makefile.eeglobal

