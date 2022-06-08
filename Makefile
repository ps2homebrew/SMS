.SILENT:

EE_OBJ_DIR = obj/
EE_INC_DIR = include/
EE_BIN_DIR = bin/
EE_SRC_DIR = src/

EE_BIN = $(EE_BIN_DIR)SMS.elf

EE_INCS    = -I$(EE_INC_DIR) -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I$(PS2SDK)/ports/include -I$(PS2SDK)/sbv/include
EE_LDFLAGS = -L$(PS2SDK)/sbv/lib -L$(PS2SDK)/ee/lib -L$(PS2SDK)/ports/lib -L$(EE_SRC_DIR)/lzma2
EE_LIBS    = -lpatches -lc -lkernel -lm

EE_OBJS  = main.o SMS_OS.o SMS_GS_0.o SMS_GS_1.o SMS_GS_2.o SMS_Timer.o           \
           SMS_MP123Core.o SMS_FileContext.o  SMS_H263.o                          \
           SMS_DSP.o SMS_DSP_MMI.o SMS_MPEG.o SMS_MPEG4.o SMS_VLC.o SMS_IPU.o     \
           SMS_AAC.o SMS_Utils.o SMS_MP123.o SMS_AC3.o SMS_SPU.o                  \
           SMS_Player.o  SMS_AC3_imdct.o SMS_MSMPEG4.o SMS_Codec.o                \
           SMS_VideoBuffer.o SMS_PlayerControl.o SMS_CDVD.o SMS_CDDA.o            \
           SMS_EE.o SMS_IOP.o SMS_PAD.o SMS_RC.o SMS_RC_0.o SMS_MC.o              \
           SMS_RingBuffer.o SMS_Container.o SMS_ContainerAVI.o SMS_ContainerMP3.o \
           SMS_ContainerM3U.o SMS_List.o SMS_Config.o About.o SMS_Data.o          \
           SMS_GSFont.o About_Data.o SMS_GUIcons.o SMS_ConfigIcon.o SMS_GUI.o     \
           SMS_GUIDevMenu.o  SMS_DirTree.o SMS_GUIFileMenu.o SMS_Locale.o         \
           SMS_FileDir.o SMS_GUIMenu.o SMS_DTS.o SMS_SubtitleContext.o            \
           SMS_GUICmdProc.o SMS_GUIDesktop.o SMS_PlayerMenu.o SMS_Sounds.o        \
           SMS_DSP_QPel.o SMS_GUIMiniBrowser.o SMS_DSP_FFT.o SMS_Spectrum.o       \
           SMS_DMA_0.o SMS_IPU_0.o SMS_IPU_1.o SMS_GUIFileCtxMenu.o               \
           SMS_InverseCodePages.o SMS_ContainerMPEG_PS.o SMS_MPEG12.o libmpeg.o   \
           libmpeg_core.o SMS_DXSB.o SMS_ContainerOGG.o SMS_OGG.o SMS_CopyTree.o  \
           SMS_GUIMenuSMS.o SMS_GUISMBrowser.o SMS_WMA.o mbstring.o SMS_PCM.o     \
           SMS_ContainerASF.o SMS_GUInfoPanel.o SMS_ContainerMOV.o                \
           SMS_ContainerAAC.o SMS_ContainerFLAC.o SMS_FLAC.o SMS_ContainerAC3.o   \
           SMS_History.o SMS_PgInd.o SMS_VSync.o SMS_GUIClock.o SMS_DateTime.o    \
           SMS_PlayerBallSim.o SMS_SIF.o SMS_ContainerJPG.o SMS_FileMapping.o     \
           SMS_JPEGData.o SMS_JPEG.o SMS_Rescale.o SMS_MPEGInit.o                 \
           lzma2.o xz_crc32.o xz_dec_lzma2.o xz_dec_stream.o

EE_OBJS := $(EE_OBJS:%=$(EE_OBJ_DIR)%)

all: $(EE_OBJ_DIR) $(EE_BIN_DIR) $(EE_BIN)
	@$(EE_STRIP) --remove-section=.comment $(EE_BIN)

$(EE_OBJ_DIR):
	@$(MKDIR) -p $(EE_OBJ_DIR)

$(EE_BIN_DIR):
	@$(MKDIR) -p $(EE_BIN_DIR)

$(EE_OBJ_DIR)%.o : $(EE_SRC_DIR)%.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJ_DIR)SMS_MPEG4.o : $(EE_SRC_DIR)SMS_MPEG4.c $(EE_INC_DIR)SMS_MPEG.h
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJ_DIR)SMS_MSMPEG4.o : $(EE_SRC_DIR)SMS_MSMPEG4.c $(EE_INC_DIR)SMS_MPEG.h
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJ_DIR)SMS_MPEG.o : $(EE_SRC_DIR)SMS_MPEG.c $(EE_INC_DIR)SMS_MPEG.h
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJ_DIR)SMS_H263.o : $(EE_SRC_DIR)SMS_H263.c $(EE_INC_DIR)SMS_MPEG.h
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJ_DIR)SMS_GS_1.o : $(EE_SRC_DIR)SMS_GS_1.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJ_DIR)SMS_GS_2.o : $(EE_SRC_DIR)SMS_GS_2.c
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
	@rm -f -r $(EE_BIN_DIR) $(EE_OBJ_DIR)

include $(PS2SDK)/Defs.make

EE_CFLAGS := -Dmemset=mips_memset -Dmemcpy=mips_memcpy -D_EE -O2 -G8192 -mgpopt -Wall -mno-check-zero-division
