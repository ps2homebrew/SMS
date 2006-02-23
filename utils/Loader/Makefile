# You can change the following to suit your ps2dev environment.
EE_PREFIX = ee-
EE_CC = $(EE_PREFIX)gcc
EE_OBJCOPY = $(EE_PREFIX)objcopy

# Change this to suit your host environment.
CC = gcc
CFLAGS = -O2 -ansi -Wall

# Don't change anything below this line.
EE_INCS = -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include
EE_CFLAGS = -D_EE -O2 -G0 -Wall $(EE_INCS)

EE_LDFLAGS = -nostartfiles -Tlinkfile -L$(PS2SDK)/ee/lib -s
EE_LIBS = -lkernel

all: loader.h

loader.h: loader.bin
	bin2c $< $@ s_Loader

loader.bin: loader.elf
	$(EE_OBJCOPY) -I elf32-littlemips -O binary $< $@

loader.elf: loader.c
	$(EE_CC) $(EE_CFLAGS) $(EE_LDFLAGS) $< -o $@ $(EE_LIBS) -Xlinker -Map -Xlinker loader.map

clean:
	rm -f loader.elf loader.bin loader.h loader.map
