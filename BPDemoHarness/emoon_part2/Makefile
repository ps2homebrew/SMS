EE_BIN = emoon_part2t.elf
BIN_IMAGE = emoon_part2.bin
EE_OBJS = crt0.o main.o Vu1Cube.o Data.o

EE_INCS = -I../PbDemoLib
EE_LDFLAGS = -L../PbDemoLib
EE_LIBS = -lpbdemo

all: $(EE_BIN) $(BIN_IMAGE)

clean:
	rm -f *.elf *.o *.a *.bin

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include ../PbDemoLib/Makefile.eeglobal
