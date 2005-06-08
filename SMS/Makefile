EE_BIN       = Test.elf
EE_OBJS      = main.o GS.o DMA.o Timer.o IPU.o SMS_Bitio.o SMS_DSP_MMI.o FileContext.o SMS_VLC.o  SMS_H263.o SMS_MPEG.o SMS_DSP.o SMS_MPEG4.o SMS_MP3.o CDDA.o SMS.o SMS_AVI.o SMS_Codec.o SMS_Integer.o SIF.o SMS_AVIPlayer.o SPU.o
EE_LIBS      = -lc -lm -laudsrv
EE_LDFLAGS  += $(EE_LIB_DIRS)

all: $(EE_BIN)
	ee-strip $(EE_BIN)

clean:
	del *.elf *.o *.a

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

EE_CFLAGS := -D_EE -O2 -G8192 -mgpopt -Wall
