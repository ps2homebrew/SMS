EE_PREFIX = ee-
EE_CC = $(EE_PREFIX)gcc
EE_OBJCOPY = $(EE_PREFIX)objcopy

EE_INCS = -I. -I../include
EE_CFLAGS = -D_EE -O2 -G0 -Wall $(EE_INCS)

SBIOS_ADDR = 0x80001000
EE_LDFLAGS = -nostartfiles -Wl,-Ttext -Wl,$(SBIOS_ADDR) -Wl,--defsym -Wl,_start=$(SBIOS_ADDR)

EE_OBJS = prologue.o sbios.o misc.o sifdma.o core.o sifcmd.o

all: sbios.bin

sbios.bin: sbios.elf
	$(EE_OBJCOPY) -I elf32-littlemips -O binary $< $@

sbios.elf: $(EE_OBJS)
	$(EE_CC) $(EE_LDFLAGS) -o $@ $(EE_OBJS)

clean:
	rm -f sbios.bin sbios.elf $(EE_OBJS)

%.o: %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o: %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
