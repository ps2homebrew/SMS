EE_BIN = tyra_demo.elf
BIN_IMAGE = tyra_demo.bin
EE_OBJS = crt0.o main.o 
EE_INCS = -I../PbDemoLib/
EE_LDFLAGS=-L../PbDemoLib/
EE_LIBS = -lpbdemo 
all: $(EE_BIN) $(BIN_IMAGE)

clean:
	rm -f *.elf *.o *.a *.bin

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include Makefile.eeglobal
