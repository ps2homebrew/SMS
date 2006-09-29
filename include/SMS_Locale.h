/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_Locale_H
# define __SMS_Locale_H

typedef struct SMString {

 unsigned int   m_Len;
 unsigned char* m_pStr;

} SMString __attribute__(   (  aligned( 4 )  )   );

# define SMS_STRING( ID ) g_SMString[ ID ]

# define STR_AVAILABLE_MEDIA      SMS_STRING(   0 )
# define STR_NONE                 SMS_STRING(   1 )
# define STR_INITIALIZING_SMS     SMS_STRING(   2 )
# define STR_SAVING_CONFIGURATION SMS_STRING(   3 )
# define STR_LOADING              SMS_STRING(   4 )
# define STR_INITIALIZING_NETWORK SMS_STRING(   5 )
# define STR_LOCATING_USBD        SMS_STRING(   6 )
# define STR_WAITING_FOR_MEDIA    SMS_STRING(   7 )
# define STR_READING_DISK         SMS_STRING(   8 )
# define STR_ILLEGAL_DISK         SMS_STRING(   9 )
# define STR_READING_MEDIA        SMS_STRING(  10 )
# define STR_DISPLAY_SETTINGS     SMS_STRING(  11 )
# define STR_DEVICE_SETTINGS      SMS_STRING(  12 )
# define STR_BROWSER_SETTINGS     SMS_STRING(  13 )
# define STR_PLAYER_SETTINGS      SMS_STRING(  14 )
# define STR_HELP                 SMS_STRING(  15 )
# define STR_SAVE_SETTINGS        SMS_STRING(  16 )
# define STR_SHUTDOWN_CONSOLE     SMS_STRING(  17 )
# define STR_EXIT_TO_BOOT_BROWSER SMS_STRING(  18 )
# define STR_TV_SYSTEM            SMS_STRING(  19 )
# define STR_PAL                  SMS_STRING(  20 )
# define STR_NTSC                 SMS_STRING(  21 )
# define STR_AUTO                 SMS_STRING(  22 )
# define STR_CHARACTER_SET        SMS_STRING(  23 )
# define STR_WINLATIN1            SMS_STRING(  24 )
# define STR_WINLATIN2            SMS_STRING(  25 )
# define STR_WINCYRILLIC          SMS_STRING(  26 )
# define STR_WINGREEK             SMS_STRING(  27 )
# define STR_DISPLAY_SETTINGS1    SMS_STRING(  28 )
# define STR_SMS_MENU             SMS_STRING(  29 )
# define STR_ADJUST_IMAGE_LEFT    SMS_STRING(  30 )
# define STR_ADJUST_IMAGE_RIGHT   SMS_STRING(  31 )
# define STR_ADJUST_IMAGE_UP      SMS_STRING(  32 )
# define STR_ADJUST_IMAGE_DOWN    SMS_STRING(  33 )
# define STR_AUTOSTART_NETWORK    SMS_STRING(  34 )
# define STR_AUTOSTART_USB        SMS_STRING(  35 )
# define STR_AUTOSTART_HDD        SMS_STRING(  36 )
# define STR_DEVICE_SETTINGS1     SMS_STRING(  37 )
# define STR_START_NETWORK_NOW    SMS_STRING(  38 )
# define STR_START_HDD_NOW        SMS_STRING(  39 )
# define STR_START_USB_NOW        SMS_STRING(  40 )
# define STR_EDIT_IPCONFIG        SMS_STRING(  41 )
# define STR_EDIT_IPCONFIG1       SMS_STRING(  42 )
# define STR_PS2_IP1              SMS_STRING(  43 )
# define STR_PS2_IP2              SMS_STRING(  44 )
# define STR_PS2_IP3              SMS_STRING(  45 )
# define STR_PS2_IP4              SMS_STRING(  46 )
# define STR_NETMASK1             SMS_STRING(  47 )
# define STR_NETMASK2             SMS_STRING(  48 )
# define STR_NETMASK3             SMS_STRING(  49 )
# define STR_NETMASK4             SMS_STRING(  50 )
# define STR_GATEWAY1             SMS_STRING(  51 )
# define STR_GATEWAY2             SMS_STRING(  52 )
# define STR_GATEWAY3             SMS_STRING(  53 )
# define STR_GATEWAY4             SMS_STRING(  54 )
# define STR_SAVE_IPCONFIG_DAT    SMS_STRING(  55 )
# define STR_BROWSER_SETTINGS1    SMS_STRING(  56 )
# define STR_USE_BACKGROUND_IMAGE SMS_STRING(  57 )
# define STR_SORT_FS_OBJECTS      SMS_STRING(  58 )
# define STR_FILTER_MEDIA_FILES   SMS_STRING(  59 )
# define STR_DISPLAY_HDL_PART     SMS_STRING(  60 )
# define STR_HIDE_SYSTEM_PART     SMS_STRING(  61 )
# define STR_ACTIVE_BORDER_CLR    SMS_STRING(  62 )
# define STR_INACTIVE_BORDER_CLR  SMS_STRING(  63 )
# define STR_TEXT_CLR             SMS_STRING(  64 )
# define STR_STATUS_LINE_TEXT_CLR SMS_STRING(  65 )
# define STR_PLAYER_SETTINGS1     SMS_STRING(  66 )
# define STR_DEFAULT_VOLUME       SMS_STRING(  67 )
# define STR_SUBTITLE_ALIGNMENT   SMS_STRING(  68 )
# define STR_SUBTITLE_OFFSET      SMS_STRING(  69 )
# define STR_AUTOLOAD_SUBTITLES   SMS_STRING(  70 )
# define STR_OPAQUE_SUBTITLES     SMS_STRING(  71 )
# define STR_SUBTITLE_COLOR       SMS_STRING(  72 )
# define STR_SUBTITLE_BOLD_COLOR  SMS_STRING(  73 )
# define STR_SUBTITLE_ITL_COLOR   SMS_STRING(  74 )
# define STR_SUBTITLE_UND_COLOR   SMS_STRING(  75 )
# define STR_AUTO_POWER_OFF       SMS_STRING(  76 )
# define STR_SCROLLBAR_LENGTH     SMS_STRING(  77 )
# define STR_SCROLLBAR_POSITION   SMS_STRING(  78 )
# define STR_DISPLAY_SB_TIME      SMS_STRING(  79 )
# define STR_SCROLLBAR_COLOR      SMS_STRING(  80 )
# define STR_VOLUME_BAR_COLOR     SMS_STRING(  81 )
# define STR_AUDIO_ANIM_DISPLAY   SMS_STRING(  82 )
# define STR_TOP                  SMS_STRING(  83 )
# define STR_BOTTOM               SMS_STRING(  84 )
# define STR_OFF                  SMS_STRING(  85 )
# define STR_CENTER               SMS_STRING(  86 )
# define STR_LEFT                 SMS_STRING(  87 )
# define STR_MIN_FORMAT           SMS_STRING(  88 )
# define STR_PTS_FORMAT           SMS_STRING(  89 )
# define STR_SUB_FONT_HSIZE       SMS_STRING(  90 )
# define STR_SUB_FONT_VSIZE       SMS_STRING(  91 )
# define STR_QUICK_HELP           SMS_STRING(  92 )
# define STR_SPACE                SMS_STRING(  93 )
# define STR_HELP_01              SMS_STRING(  94 )
# define STR_HELP_02              SMS_STRING(  95 )
# define STR_HELP_03              SMS_STRING(  96 )
# define STR_HELP_04              SMS_STRING(  97 )
# define STR_HELP_05              SMS_STRING(  98 )
# define STR_HELP_06              SMS_STRING(  99 )
# define STR_HELP_07              SMS_STRING( 100 )
# define STR_HELP_08              SMS_STRING( 101 )
# define STR_HELP_09              SMS_STRING( 102 )
# define STR_HELP_10              SMS_STRING( 103 )
# define STR_HELP_11              SMS_STRING( 104 )
# define STR_HELP_12              SMS_STRING( 105 )
# define STR_HELP_13              SMS_STRING( 106 )
# define STR_HELP_14              SMS_STRING( 107 )
# define STR_HELP_15              SMS_STRING( 108 )
# define STR_HELP_16              SMS_STRING( 109 )
# define STR_HELP_17              SMS_STRING( 110 )
# define STR_HELP_18              SMS_STRING( 111 )
# define STR_HELP_19              SMS_STRING( 112 )
# define STR_HELP_20              SMS_STRING( 113 )
# define STR_HELP_21              SMS_STRING( 114 )
# define STR_HELP_22              SMS_STRING( 115 )
# define STR_HELP_23              SMS_STRING( 116 )
# define STR_HELP_24              SMS_STRING( 117 )
# define STR_HELP_25              SMS_STRING( 118 )
# define STR_HELP_26              SMS_STRING( 119 )
# define STR_HELP_27              SMS_STRING( 120 )
# define STR_HELP_28              SMS_STRING( 121 )
# define STR_HELP_29              SMS_STRING( 122 )
# define STR_HELP_30              SMS_STRING( 123 )
# define STR_HELP_31              SMS_STRING( 124 )
# define STR_HELP_32              SMS_STRING( 125 )
# define STR_HELP_33              SMS_STRING( 126 )
# define STR_ERROR                SMS_STRING( 127 )
# define STR_SAVING_IPCONFIG      SMS_STRING( 128 )
# define STR_SEL_BAR_CLR          SMS_STRING( 129 )
# define STR_ADVANCED_SETTINGS    SMS_STRING( 130 )
# define STR_DISPLAY_HEIGHT       SMS_STRING( 131 )
# define STR_APPLY_SETTINGS       SMS_STRING( 132 )
# define STR_ADVANCED_SETTINGS1   SMS_STRING( 133 )
# define STR_SAMPLE               SMS_STRING( 134 )
# define STR_DEFAULT              SMS_STRING( 135 )
# define STR_USER_DEFINED         SMS_STRING( 136 )
# define STR_LANGUAGE             SMS_STRING( 137 )
# define STR_LOADING_INDICES      SMS_STRING( 138 )
# define STR_LOADING_SUBTITLES    SMS_STRING( 139 )
# define STR_SUB_ERROR            SMS_STRING( 140 )
# define STR_FORMAT               SMS_STRING( 141 )
# define STR_SEQUENCE             SMS_STRING( 142 )
# define STR_BUFFERING_FILE       SMS_STRING( 143 )
# define STR_DETECTING_FFORMAT    SMS_STRING( 144 )
# define STR_PAUSE                SMS_STRING( 145 )
# define STR_STOPPING             SMS_STRING( 146 )
# define STR_DEFAULT1             SMS_STRING( 147 )
# define STR_VA                   SMS_STRING( 148 )
# define STR_SV                   SMS_STRING( 149 )
# define STR_PLAY                 SMS_STRING( 150 )
# define STR_FFWD                 SMS_STRING( 151 )
# define STR_REW                  SMS_STRING( 152 )
# define STR_CURS                 SMS_STRING( 153 )
# define STR_UNSUPPORTED_FILE     SMS_STRING( 154 )
# define STR_REM                  SMS_STRING( 155 )
# define STR_PLAYER_MENU          SMS_STRING( 156 )
# define STR_DISPLAY              SMS_STRING( 157 )
# define STR_LETTERBOX            SMS_STRING( 158 )
# define STR_PANSCAN1             SMS_STRING( 159 )
# define STR_PANSCAN2             SMS_STRING( 160 )
# define STR_PANSCAN3             SMS_STRING( 161 )
# define STR_FULLSCREEN           SMS_STRING( 162 )
# define STR_DISPLAY_SUBTITLES    SMS_STRING( 163 )
# define STR_SELECT_SUBTITLES     SMS_STRING( 164 )
# define STR_BOOT_BROWSER         SMS_STRING( 165 )
# define STR_EXEC0                SMS_STRING( 166 )
# define STR_EXEC1                SMS_STRING( 167 )
# define STR_EXIT_TO              SMS_STRING( 168 )
# define STR_SOUND_FX             SMS_STRING( 169 )
# define STR_SPDIF_DD             SMS_STRING( 170 )
# define STR_MP3_SETTINGS         SMS_STRING( 171 )
# define STR_RANDOMIZE_PLAYLIST   SMS_STRING( 172 )
# define STR_REPEAT_MODE          SMS_STRING( 173 )
# define STR_AUDIO_SPECTRUM_DSP   SMS_STRING( 174 )
# define STR_COPY_TO_HDD          SMS_STRING( 175 )
# define STR_COPYING              SMS_STRING( 176 )
# define STR_KBS                  SMS_STRING( 177 )
# define STR_SELECT_ACTION        SMS_STRING( 178 )
# define STR_COLOR_RESOLUTION     SMS_STRING( 179 )
# define STR_32_BIT               SMS_STRING( 180 )
# define STR_16_BIT               SMS_STRING( 181 )
# define STR_DTV_480P             SMS_STRING( 182 )
# define STR_CONTROLLER_SLOT2     SMS_STRING( 183 )
# define STR_GAMEPAD              SMS_STRING( 184 )
# define STR_REMOTE_CONTROL       SMS_STRING( 185 )
# define STR_MP3_HP               SMS_STRING( 186 )
# define STR_RIGHT                SMS_STRING( 187 )
# define STR_VESA_60_HZ           SMS_STRING( 188 )
# define STR_VESA_75_HZ           SMS_STRING( 189 )

extern SMString g_SMString  [] __attribute__(   (  section( ".bss" )  )   );
extern char     g_EmptyStr  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_SlashStr  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_BSlashStr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_ColonStr  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_DesktopStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_StatuslStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_DevMenuStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_FilMenuStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_SMSMenuStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pDefIP    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pDefMask  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pDefGW    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pIPConf   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_DotStr    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pDefStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pAVIStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pExtM3UStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pExtInfStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pM3UStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pPercDStr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pMP3Str   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pCmdPrcStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pSubStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pSrtStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char     g_pBXDATASYS[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_LocaleInit ( void );
void SMS_LocaleSet  ( void );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_Locale_H */
