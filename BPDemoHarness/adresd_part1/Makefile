EE_BIN = adresd_part1.elf
BIN_IMAGE = adresd_part1.bin
EE_OBJS = crt0.o main.o SparmGenDot.o Vu1Micro_SDot.o adresd_bg_tex.o

EE_INCS = -I../PbDemoLib
EE_LDFLAGS = -L../PbDemoLib
EE_LIBS = -lpbdemo -lgcc -lkernel

all: clean $(EE_BIN) $(BIN_IMAGE)

clean:
	rm -f adresd_part1.elf *.o *.a *.bin
	
run:
	e:\ps2\inlink\inlink.exe host:..\harness.elf host:jar_part1.bin	

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include ../PbDemoLib/Makefile.eeglobal
