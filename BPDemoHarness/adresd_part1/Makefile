EE_BIN = adresd_part1.elf
BIN_IMAGE = adresd_part1.bin
EE_OBJS = crt0.o main.o SparmGenDot.o Vu1Micro_SDot.o

EE_INCS = -I../PbDemoLib
EE_LDFLAGS = -L../PbDemoLib
EE_LIBS = -lpbdemo -lgcc

all: $(EE_BIN) $(BIN_IMAGE)

clean:
	rm -f *.elf *.o *.a *.bin

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include ../PbDemoLib/Makefile.eeglobal
