EE_BIN = adresd_part5.elf
BIN_IMAGE = adresd_part5.bin
EE_OBJS = crt0.o main.o oddments.o odd.o Vu1Distort.o ../PbDemoLib/libpbdemo.a
all: $(EE_BIN) $(BIN_IMAGE)

EE_LDFLAGS = -L../PbDemoLib
EE_LIBS = -lpbdemo -lgcc
EE_INCS += -I../PbDemoLib

clean:
	rm -f *.elf *.o *.a *.bin

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include ../PbDemoLib/Makefile.eeglobal

