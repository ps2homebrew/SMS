EE_BIN = tyra_demo.elf
BIN_IMAGE = tyra_demo.bin
EE_OBJS = crt0.o main.o gs.o vram_malloc.o prim.o ee_malloc.o \
vif.o utils.o vu_program.o box0.o matrix.o sine_tabs.o \
random.o 
EE_LIBS = 
all: $(EE_BIN) $(BIN_IMAGE)

clean:
	rm -f *.elf *.o *.a *.bin *~

$(BIN_IMAGE) : $(EE_BIN)
	ee-objcopy -O binary $(EE_BIN) $(BIN_IMAGE)

include $(PS2LIB)/Makefile.pref
include Makefile.eeglobal
