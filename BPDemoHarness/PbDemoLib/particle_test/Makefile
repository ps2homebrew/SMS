EE_BIN = particle_test.elf
BIN_IMAGE = particle_test.bin
EE_OBJS = crt0.o main.o

EE_INCS = -I../
EE_LDFLAGS = -L../
EE_LIBS = -lpbdemo

all: $(EE_BIN) $(BIN_IMAGE)

clean:
	rm -f *.elf *.o *.a *.bin

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include Makefile.eeglobal
