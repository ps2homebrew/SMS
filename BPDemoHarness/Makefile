EE_BIN = harness.elf
EE_OBJS = crt0.o main.o gs.o stream_ee/streamload_rpc.o dernc.o
# uncomment to add newlib support
#EE_LIBS = $(NEWLIB)/lib/libc.a -lpad
EE_LIBS = -lpad

all: $(EE_BIN)

clean:
	rm -f *.elf *.o *.a

include $(PS2LIB)/Makefile.pref
include Makefile.eeglobal
