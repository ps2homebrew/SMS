EE_BIN = emoon_part1.elf
BIN_IMAGE = emoon_part1.bin
EE_OBJS = crt0.o main.o gs.o vram_malloc.o prim.o  \
          PbGfx.o PbVu1.o PbMatrix.o PbGlobal.o \
	  math.o PbDma.o PbFile.o PbPart1.o PbMeshData.o PbGs.o PbTexture.o PbSpr.o data/PbData.o PbVu1Micro.o
all: $(EE_BIN) $(BIN_IMAGE)

clean:
	rm -f *.elf *.o *.a *.bin

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include Makefile.eeglobal
