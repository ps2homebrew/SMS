EE_LIB = libpbdemo.a
EE_OBJS = PbMatrix.o PbMath.o PbDma.o PbSpr.o PbVif.o PbScreen.o PbVram.o  \
          PbPrim.o PbMisc.o PbTexture.o PbFile.o PbVu1.o

all: $(EE_LIB)

clean:
	rm -f $(EE_LIB) $(EE_OBJS)

include $(PS2LIB)/Makefile.pref
include Makefile.eeglobal
