EE_BIN = particle_test.elf
BIN_IMAGE = particle_test.bin
EE_OBJS = crt0.o main.o math.o

EE_CFLAGS += -fshort-double

EE_INCS = -I../PbDemoLib
EE_LDFLAGS = -L../PbDemoLib
EE_LIBS = -lpbdemo -lgcc

.SUFFIXES: .s .o .bin

.bin.s:
	rawtoasm $< $@ $@	

all: $(EE_BIN) $(BIN_IMAGE)

clean:
	rm -f *.elf *.o *.a $(BIN_IMAGE) $(EE_BIN)

run: all
	ps2client -h 10.0.0.10 execee host:..\harness.elf host:particle_test.bin
	
longtest: all
	ps2client -h 10.0.0.10 execee host:..\harness.elf -t250 host:particle_test.bin
	

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include ../PbDemoLib/Makefile.eeglobal
