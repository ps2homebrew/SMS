EE_BIN = adresd_part4.elf
BIN_IMAGE = intro.bin
EE_OBJS = crt0.o main.o oddments.o odd.o Vu1Distort.o ../PbDemoLib/libpbdemo.a gfx/ps2dev.o \
          gfx/presents.o gfx/font.o font_lookup.o
all: $(EE_BIN) $(BIN_IMAGE)

EE_LDFLAGS = -L../PbDemoLib
EE_LIBS = -lpbdemo -lgcc -lkernel
EE_INCS += -I../PbDemoLib

clean:
	rm -f adresd_part4.elf *.o *.a *.bin

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include ../PbDemoLib/Makefile.eeglobal

