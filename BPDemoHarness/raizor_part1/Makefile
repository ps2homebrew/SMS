EE_BIN = jar_part1.elf
BIN_IMAGE = bin.bin
EE_OBJS = crt0.o main.o math.o texture.o \
          gfx/c_rzr.o gfx/c_emoon.o gfx/c_tyranid.o gfx/c_jar.o gfx/c_adresd.o \
          gfx/c_gfxcoding.o gfx/credits.o

EE_CFLAGS += -fshort-double

EE_INCS = -I../PbDemoLib
EE_LDFLAGS = -L../PbDemoLib
EE_LIBS = -lpbdemo -lgcc -lkernel

.SUFFIXES: .s .o .bin

.bin.s:
	rawtoasm $< $@ $@	

all: clean $(EE_BIN) $(BIN_IMAGE)

clean:
	rm -f jar_part1.elf *.o *.a $(BIN_IMAGE) $(EE_BIN)

run: all
	ps2client -h 10.0.0.10 execee host:..\harness.elf host:jar_part1.bin
	
longtest: all
	ps2client -h 10.0.0.10 execee host:..\harness.elf -t250 host:jar_part1.bin	

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include ../PbDemoLib/Makefile.eeglobal
