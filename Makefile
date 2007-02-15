EE_OBJ_DIR = obj/
EE_INC_DIR = include/
EE_BIN_DIR = bin/
EE_SRC_DIR = src/

EE_BIN = $(EE_BIN_DIR)SMS.elf

EE_INCS     = -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include
EE_INCS    := -I$(EE_INC_DIR) $(EE_INCS) -I$(PS2SDK)/sbv/include
EE_LIBS     = -lhdd -lpoweroff -lpatches -lfileXio -lc -lkernel
EE_LDFLAGS  = -L$(PS2SDK)/sbv/lib -L$(PS2SDK)/ee/lib

EE_OBJS  = main.o SMS_GS_0.o SMS_GS_1.o SMS_GS_2.o SMS_Timer.o                    \
           SMS_IPU.o SMS_Bitio.o SMS_MP123Core.o SMS_FileContext.o SMS_H263.o     \
           SMS_DSP.o SMS_DSP_MMI.o SMS_MPEG.o SMS_VLC.o SMS_MPEG4.o SMS_MPEG2.o   \
           SMS_MP123.o SMS_SPU.o SMS_CDDA.o SMS_Codec.o SMS_SIF.o                 \
           SMS_Player.o SMS_AC3.o SMS_AC3_imdct.o SMS_MSMPEG4.o SMS_VideoBuffer.o \
           SMS_PlayerControl.o SMS_PlayerBallSim.o SMS_CDVD.o SMS_EE.o SMS_IOP.o  \
           SMS_PAD.o SMS_MC.o SMS_RingBuffer.o SMS_Container.o SMS_ContainerAVI.o \
           SMS_ContainerMP3.o SMS_ContainerM3U.o SMS_List.o SMS_Config.o About.o  \
           SMS_Data.o SMS_GSFont.o SMS_GUIcons.o SMS_ConfigIcon.o SMS_GUI.o       \
           SMS_GUIDesktop.o SMS_GUIDevMenu.o SMS_GUIFileMenu.o SMS_Locale.o       \
           SMS_FileDir.o SMS_GUIMenu.o SMS_GUIMenuSMS.o                           \
           SMS_SubtitleContext.o SMS_GUICmdProc.o SMS_PlayerMenu.o                \
           SMS_GUIMiniBrowser.o SMS_Sounds.o SMS_DSP_QPel.o SMS_Spectrum.o        \
           SMS_Utils.o SMS_DSP_FFT.o SMS_DMA_0.o SMS_IPU_0.o SMS_IPU_1.o          \
           SMS_GUIFileCtxMenu.o SMS_RC.o SMS_RC_0.o SMS_InverseCodePages.o        \
           SMS_ContainerMPEG_PS.o SMS_MPEG12.o libmpeg.o libmpeg_core.o

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

$(EE_OBJ_DIR)%.o : $(EE_SRC_DIR)%.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_BIN) : $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o
	$(EE_CC) -mno-crt0 -T$(PS2SDK)/ee/startup/linkfile $(EE_LDFLAGS) \
		  -o $(EE_BIN) $(PS2SDK)/ee/startup/crt0.o $(EE_OBJS) $(EE_LIBS) -Xlinker -Map -Xlinker ./obj/SMS.map

rebuild: clean all

clean:
	@rm -rf $(EE_BIN_DIR)*.elf $(EE_OBJ_DIR)*.o

include $(PS2SDK)/Defs.make

EE_CFLAGS := -Dmemcpy=mips_memcpy -DVB_SYNC -D_EE -O2 -G8192 -mgpopt -Wall -mno-check-zero-division
