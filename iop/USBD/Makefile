
IOP_OBJS_DIR = obj/
IOP_BIN_DIR  = bin/
IOP_SRC_DIR  = src/
IOP_INC_DIR  = include/

IOP_BIN  = bin/USBD.irx
IOP_OBJS = obj/hcd.o obj/hub.o obj/interface.o obj/mem.o obj/usbd.o obj/usbio.o obj/driver.o obj/imports.o obj/exports.o

IOP_CFLAGS=-Wall -I$(PS2SDK)/iop/include -I$(PS2SDK)/common/include
# -DDEBUG
IOP_LDFLAGS=-Wl,-s

all: $(IOP_OBJS_DIR) $(IOP_BIN_DIR) $(IOP_BIN)
	xz --lzma2=nice=32 --check=crc32 $(IOP_BIN)
	bin2c $(IOP_BIN).xz USBD.dmp s_USBD
	rm $(IOP_BIN).xz

clean:
	rm -f -r $(IOP_OBJS_DIR) $(IOP_BIN_DIR)

rebuild: clean all

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
