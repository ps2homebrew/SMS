EE_BIN = emoon_part1.elf
BIN_IMAGE = emoon_part1.bin
EE_OBJS = crt0.o main.o data/PbData.o Vu1Envmap.o ../PbDemoLib/libpbdemo.a
all: $(EE_BIN) $(BIN_IMAGE)

EE_LDFLAGS = -L../PbDemoLib
EE_LIBS = -lpbdemo -lgcc
EE_INCS += -I../PbDemoLib

clean:
	rm -f *.elf *.o *.a *.bin

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include Makefile.eeglobal

