EE_OBJ_DIR = obj/
EE_INC_DIR = include/
EE_BIN_DIR = bin/
EE_SRC_DIR = src/

EE_BIN = $(EE_BIN_DIR)SMS.elf

EE_INCS     = -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include
EE_INCS    := -I$(EE_INC_DIR) $(EE_INCS) -I$(PS2SDK)/sbv/include
EE_LIBS     = -lfileXio -lhdd -lc -lkernel -laudsrv -lpatches
EE_LDFLAGS  = -L$(PS2SDK)/sbv/lib -L$(PS2SDK)/ee/lib

EE_OBJS  = main.o GS.o DMA.o Timer.o IPU.o SMS_Bitio.o SMS_DSP_MMI.o FileContext.o SMS_VLC.o        \
           SMS_H263.o SMS_MPEG.o SMS_DSP.o SMS_MPEG4.o SMS_MP3.o CDDA.o SMS.o SMS_AVI.o SMS_Codec.o \
           SMS_Integer.o SIF.o SMS_AVIPlayer.o SPU.o SMS_AudioBuffer.o SMS_Data.o SMS_VideoBuffer.o
EE_OBJS := $(EE_OBJS:%=$(EE_OBJ_DIR)%)

all: $(EE_OBJ_DIR) $(EE_BIN_DIR) $(EE_BIN)
	@ee-strip $(EE_BIN)

$(EE_OBJ_DIR):
	@$(MKDIR) -p $(EE_OBJ_DIR)

$(EE_BIN_DIR):
	@$(MKDIR) -p $(EE_BIN_DIR)

$(EE_OBJ_DIR)%.o : $(EE_SRC_DIR)%.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJ_DIR)%.o : $(EE_SRC_DIR)%.s
	$(EE_AS) $(EE_ASFLAGS) $< -o $@

$(EE_BIN) : $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o
	$(EE_CC) -mno-crt0 -T$(PS2SDK)/ee/startup/linkfile $(EE_LDFLAGS) \
		  -o $(EE_BIN) $(PS2SDK)/ee/startup/crt0.o $(EE_OBJS) $(EE_LIBS)

rebuild: clean all

clean:
	@rm -rf $(EE_BIN_DIR)*.elf $(EE_OBJ_DIR)*.o

include $(PS2SDK)/Defs.make

EE_CFLAGS := -DVB_SYNC -D_EE -O2 -G8192 -mgpopt -Wall
