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
#include "SMS_Locale.h"
#include "SMS_Config.h"

#include <string.h>
#include <fileio.h>
#include <malloc.h>

static char s_SMSLng[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:SMS/SMS.lng";

char g_EmptyStr  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "";
char g_SlashStr  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "/";
char g_BSlashStr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "\\";
char g_ColonStr  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ":";
char g_DesktopStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "DESKTOP";
char g_StatuslStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "STATUSL";
char g_DevMenuStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "DEVMENU";
char g_FilMenuStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "FILMENU";
char g_SMSMenuStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "SMS Menu";
char g_pDefIP    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "192.168.0.10\x00\x00\x00";
char g_pDefMask  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "255.255.255.0\x00\x00";
char g_pDefGW    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "192.168.0.1\x00\x00\x00\x00";
char g_pIPConf   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:/SYS-CONF/IPCONFIG.DAT";
char g_DotStr    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ".";
char g_pAVIStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "AVI";
char g_pExtM3UStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "#EXTM3U";
char g_pExtInfStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "#EXTINF";
char g_pM3UStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "M3U";
char g_pPercDStr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%d";
char g_pMP3Str   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "MP3";
char g_pCmdPrcStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "CMDPROC";
char g_pSubStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "sub";
char g_pSrtStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "srt";
char g_pBXDATASYS[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:/B?DATA-SYSTEM/";

static unsigned char s_pAvailableMedia [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Available media:  ";
static unsigned char s_pNone           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "none";
static unsigned char s_pInitSMS        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Initializing SMS...";
static unsigned char s_pSavingConf     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Saving configuration...";
static unsigned char s_pLoading        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Loading %s...";
static unsigned char s_pInitNet        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Initializing network...";
static unsigned char s_pLocUSBD        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Locating USBD.IRX...";
static unsigned char s_pWaitingFMedia  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Waiting for media (press \"start\" for menu)...";
static unsigned char s_pReadingDisk    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Reading disk...";
static unsigned char s_pIllegalDisk    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Illegal disk (press \"cross\" to continue)...";
static unsigned char s_pReadingMedia   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Reading media...";
static unsigned char s_pDisplaySettings[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display settings...";
static unsigned char s_pDispSettings1  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display settings";
static unsigned char s_pDeviceSettings [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Device settings...";
static unsigned char s_pDeviceSettings1[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Device settings";
static unsigned char s_pBrowserSettings[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Browser settings...";
static unsigned char s_pBrowsSettings1 [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Browser settings";
static unsigned char s_pPlayerSettings [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Player settings...";
static unsigned char s_pPlayerSettings1[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Player settings";
static unsigned char s_pHelp           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Help...";
static unsigned char s_pSaveSettings   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Save settings";
static unsigned char s_pShutdownConsole[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Shutdown console";
static unsigned char s_pExit2BB        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Exit SMS";
static unsigned char s_pTVSystem       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "TV system";
static unsigned char s_pPAL            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "PAL";
static unsigned char s_pNTSC           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "NTSC";
static unsigned char s_pAuto           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Auto";
static unsigned char s_pCharset        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Character set";
static unsigned char s_pWinLatin1      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "WinLatin1";
static unsigned char s_pWinLatin2      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "WinLatin2";
static unsigned char s_pWinCyrillic    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "WinCyrillic";
static unsigned char s_pWinGreek       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "WinGreek";
static unsigned char s_pAdjustLeft     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Adjust image left";
static unsigned char s_pAdjustRight    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Adjust image right";
static unsigned char s_pAdjustUp       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Adjust image up";
static unsigned char s_pAdjustDown     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Adjust image down";
static unsigned char s_pAutoNet        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Autostart network";
static unsigned char s_pAutoUSB        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Autostart USB";
static unsigned char s_pAutoHDD        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Autostart HDD";
static unsigned char s_pStartNet       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Start network support";
static unsigned char s_pStartHDD       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Start HDD support";
static unsigned char s_pStartUSB       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Start USB support";
static unsigned char s_pEditIPConfig   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Edit IPCONFIG.DAT...";
static unsigned char s_pEditIPConfig1  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Edit IPCONFIG.DAT";
static unsigned char s_pPS2IP1         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "IP Address (octet 1)";
static unsigned char s_pPS2IP2         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "IP Address (octet 2)";
static unsigned char s_pPS2IP3         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "IP Address (octet 3)";
static unsigned char s_pPS2IP4         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "IP Address (octet 4)";
static unsigned char s_pNetMask1       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Net Mask (octet 1)";
static unsigned char s_pNetMask2       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Net Mask (octet 2)";
static unsigned char s_pNetMask3       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Net Mask (octet 3)";
static unsigned char s_pNetMask4       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Net Mask (octet 4)";
static unsigned char s_pGateway1       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Gateway Address (octet 1)";
static unsigned char s_pGateway2       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Gateway Address (octet 2)";
static unsigned char s_pGateway3       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Gateway Address (octet 3)";
static unsigned char s_pGateway4       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Gateway Address (octet 4)";
static unsigned char s_pSaveIPC        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Save/Update IPCONFIG.DAT";
static unsigned char s_pUseBgImage     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Use background image";
static unsigned char s_pSortFSObjects  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Sort filesystem objects";
static unsigned char s_pFilterMFiles   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Filter media files";
static unsigned char s_pDisplayHDLPart [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display HDL partitions";
static unsigned char s_pHideSysPart    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Hide system partitions";
static unsigned char s_pActBorderClr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Active border color";
static unsigned char s_pInactBorderClr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Inactive border color";
static unsigned char s_pFontClr        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Text color";
static unsigned char s_pSLTextColor    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Status line text color";
static unsigned char s_pDefaultVolume  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Default volume";
static unsigned char s_pSubAlignment   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle alignment";
static unsigned char s_pSubOffset      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle offset";
static unsigned char s_pSubAutoload    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Autoload subtitles";
static unsigned char s_pSubOpaque      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Opaque subtitles";
static unsigned char s_pSubColor       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle color";
static unsigned char s_pSubBColor      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle bold color";
static unsigned char s_pSubIColor      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle italic color";
static unsigned char s_pSubUColor      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle underline color";
static unsigned char s_pAutoPowerOff   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Auto power-off";
static unsigned char s_pScrollBarLen   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Scroll bar length";
static unsigned char s_pScrollBarPos   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Scroll bar position";
static unsigned char s_pDisplaySBTime  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display scroll bar time";
static unsigned char s_pScrollbarClr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Scroll bar color";
static unsigned char s_pVolumeBarClr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Volume bar color";
static unsigned char s_pAudioAnimDisp  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Audio animation display";
static unsigned char s_pTop            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "top";
static unsigned char s_pBottom         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "bottom";
static unsigned char s_pOff            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "off";
static unsigned char s_pCenter         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "center";
static unsigned char s_pLeft           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "left";
static unsigned char s_pMin            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%d min";
static unsigned char s_pPts            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%d pts";
static unsigned char s_pSubFontHSize   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle font width";
static unsigned char s_pSubFontVSize   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle font height";
static unsigned char s_pQuickHelp      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Quick help";
static unsigned char s_pSpace          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = " ";
static unsigned char s_pHelp01         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "At startup:";
static unsigned char s_pHelp02         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+R1 - NTSC mode";
static unsigned char s_pHelp03         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+R2 - PAL mode";
static unsigned char s_pHelp04         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Browser:";
static unsigned char s_pHelp05         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "triangle - parent directory";
static unsigned char s_pHelp06         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "left/right - device menu";
static unsigned char s_pHelp07         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "up/down - navigate directory";
static unsigned char s_pHelp08         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "cross - action";
static unsigned char s_pHelp09         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+circle - power off";
static unsigned char s_pHelp10         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+R1 - adjust image right";
static unsigned char s_pHelp11         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+L1 - adjust image left";
static unsigned char s_pHelp12         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+R2 - adjust image down";
static unsigned char s_pHelp13         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+L2 - adjust image up";
static unsigned char s_pHelp14         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+square - save settings";
static unsigned char s_pHelp15         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+triangle - boot browser";
static unsigned char s_pHelp16         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "L1+L2+R1+R2 - display about";
static unsigned char s_pHelp17         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Player:";
static unsigned char s_pHelp18         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "up/down - adjust volume";
static unsigned char s_pHelp19         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "right/left - FFWD/REW mode";
static unsigned char s_pHelp20         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "cross - exit FFWD/REW mode";
static unsigned char s_pHelp21         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "triangle - stop";
static unsigned char s_pHelp22         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select - pause/timeline";
static unsigned char s_pHelp23         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "start - resume/menu";
static unsigned char s_pHelp24         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "cross - OSD timer";
static unsigned char s_pHelp25         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "rectangle - pan-scan mode";
static unsigned char s_pHelp26         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "L1 - pan left";
static unsigned char s_pHelp27         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "R1 - pan right";
static unsigned char s_pHelp28         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "circle - V/A (S/V) sync mode";
static unsigned char s_pHelp29         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "L2 - derease sync value";
static unsigned char s_pHelp30         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "R2 - increase sync value";
static unsigned char s_pHelp31         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "SMS menu:";
static unsigned char s_pHelp32         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "cross/circle - action/next level";
static unsigned char s_pHelp33         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "triangle - level up/exit menu";
static unsigned char s_pError          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Error (press \"cross\" to continue)...";
static unsigned char s_pSavingIPConfig [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Saving IPCONFIG.DAT...";
static unsigned char s_pSelBarClr      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Selection bar color";
static unsigned char s_pAdvanced       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Advanced settings...";
static unsigned char s_pAdvanced1      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Advanced settings";
static unsigned char s_pDispHeight     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display height";
static unsigned char s_pApplySettings  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Apply settings";
static unsigned char s_pSampleStr      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Sample";
                char g_pDefStr         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "English";
static unsigned char s_pUDFStr         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "user defined";
static unsigned char s_pLangStr        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Language";
static unsigned char s_pLoadIdx        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Loading indices...";
static unsigned char s_pLoadSub        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Loading subtitles...";
static unsigned char s_pSubError       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle: %s error (%d), press \"cross\" to continue...";
static unsigned char s_pFormat         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "format";
static unsigned char s_pSequence       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "sequence";
static unsigned char s_pBufferingFile  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Buffering %s file...";
static unsigned char s_pDetectingFmt   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Detecting file format...";
static unsigned char s_pPause          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Pause";
static unsigned char s_pStopping       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Stopping";
static unsigned char s_pDefault        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "default";
static unsigned char s_pVA             [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "V/A:";
static unsigned char s_pSV             [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "S/V:";
static unsigned char s_pPlay           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Play";
static unsigned char s_pFFwd           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "FFwd";
static unsigned char s_pRew            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Rew";
static unsigned char s_pCurs           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Curs";
static unsigned char s_pUnsupportedFile[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Unsupported file format (press \"cross\" to continue)...";
static unsigned char s_pRem            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Rem";
static unsigned char s_pPlayerMenu     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Player menu";
static unsigned char s_pDisplay        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display";
static unsigned char s_pLetterbox      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "letterbox";
static unsigned char s_pPanScan1       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "pan-scan 1";
static unsigned char s_pPanScan2       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "pan-scan 2";
static unsigned char s_pPanScan3       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "pan-scan 3";
static unsigned char s_pFullscreen     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "fullscreen";
static unsigned char s_pDisplaySubs    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display subtitles";
static unsigned char s_pSelectSubs     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Select subtitles";
static unsigned char s_pBootBrowser    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "boot browser";
static unsigned char s_pExec0          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:/BOOT/BOOT.ELF";
static unsigned char s_pExec1          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:/BEDATA-SYSTEM/BOOT.ELF";
static unsigned char s_pExitTo         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Exit to";
static unsigned char s_pSoundFX        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Sound FX";

static SMString s_SMStringDef[] __attribute__(   (  section( ".data" )  )   ) = {
 { sizeof ( s_pAvailableMedia  ) - 1, s_pAvailableMedia  },
 { sizeof ( s_pNone            ) - 1, s_pNone            },
 { sizeof ( s_pInitSMS         ) - 1, s_pInitSMS         },
 { sizeof ( s_pSavingConf      ) - 1, s_pSavingConf      },
 { sizeof ( s_pLoading         ) - 1, s_pLoading         },
 { sizeof ( s_pInitNet         ) - 1, s_pInitNet         },
 { sizeof ( s_pLocUSBD         ) - 1, s_pLocUSBD         },
 { sizeof ( s_pWaitingFMedia   ) - 1, s_pWaitingFMedia   },
 { sizeof ( s_pReadingDisk     ) - 1, s_pReadingDisk     },
 { sizeof ( s_pIllegalDisk     ) - 1, s_pIllegalDisk     },
 { sizeof ( s_pReadingMedia    ) - 1, s_pReadingMedia    },
 { sizeof ( s_pDisplaySettings ) - 1, s_pDisplaySettings },
 { sizeof ( s_pDeviceSettings  ) - 1, s_pDeviceSettings  },
 { sizeof ( s_pBrowserSettings ) - 1, s_pBrowserSettings },
 { sizeof ( s_pPlayerSettings  ) - 1, s_pPlayerSettings  },
 { sizeof ( s_pHelp            ) - 1, s_pHelp            },
 { sizeof ( s_pSaveSettings    ) - 1, s_pSaveSettings    },
 { sizeof ( s_pShutdownConsole ) - 1, s_pShutdownConsole },
 { sizeof ( s_pExit2BB         ) - 1, s_pExit2BB         },
 { sizeof ( s_pTVSystem        ) - 1, s_pTVSystem        },
 { sizeof ( s_pPAL             ) - 1, s_pPAL             },
 { sizeof ( s_pNTSC            ) - 1, s_pNTSC            },
 { sizeof ( s_pAuto            ) - 1, s_pAuto            },
 { sizeof ( s_pCharset         ) - 1, s_pCharset         },
 { sizeof ( s_pWinLatin1       ) - 1, s_pWinLatin1       },
 { sizeof ( s_pWinLatin2       ) - 1, s_pWinLatin2       },
 { sizeof ( s_pWinCyrillic     ) - 1, s_pWinCyrillic     },
 { sizeof ( s_pWinGreek        ) - 1, s_pWinGreek        },
 { sizeof ( s_pDispSettings1   ) - 1, s_pDispSettings1   },
 { sizeof ( g_SMSMenuStr       ) - 1, g_SMSMenuStr       },
 { sizeof ( s_pAdjustLeft      ) - 1, s_pAdjustLeft      },
 { sizeof ( s_pAdjustRight     ) - 1, s_pAdjustRight     },
 { sizeof ( s_pAdjustUp        ) - 1, s_pAdjustUp        },
 { sizeof ( s_pAdjustDown      ) - 1, s_pAdjustDown      },
 { sizeof ( s_pAutoNet         ) - 1, s_pAutoNet         },
 { sizeof ( s_pAutoUSB         ) - 1, s_pAutoUSB         },
 { sizeof ( s_pAutoHDD         ) - 1, s_pAutoHDD         },
 { sizeof ( s_pDeviceSettings1 ) - 1, s_pDeviceSettings1 },
 { sizeof ( s_pStartNet        ) - 1, s_pStartNet        },
 { sizeof ( s_pStartHDD        ) - 1, s_pStartHDD        },
 { sizeof ( s_pStartUSB        ) - 1, s_pStartUSB        },
 { sizeof ( s_pEditIPConfig    ) - 1, s_pEditIPConfig    },
 { sizeof ( s_pEditIPConfig1   ) - 1, s_pEditIPConfig1   },
 { sizeof ( s_pPS2IP1          ) - 1, s_pPS2IP1          },
 { sizeof ( s_pPS2IP2          ) - 1, s_pPS2IP2          },
 { sizeof ( s_pPS2IP3          ) - 1, s_pPS2IP3          },
 { sizeof ( s_pPS2IP4          ) - 1, s_pPS2IP4          },
 { sizeof ( s_pNetMask1        ) - 1, s_pNetMask1        },
 { sizeof ( s_pNetMask2        ) - 1, s_pNetMask2        },
 { sizeof ( s_pNetMask3        ) - 1, s_pNetMask3        },
 { sizeof ( s_pNetMask4        ) - 1, s_pNetMask4        },
 { sizeof ( s_pGateway1        ) - 1, s_pGateway1        },
 { sizeof ( s_pGateway2        ) - 1, s_pGateway2        },
 { sizeof ( s_pGateway3        ) - 1, s_pGateway3        },
 { sizeof ( s_pGateway4        ) - 1, s_pGateway4        },
 { sizeof ( s_pSaveIPC         ) - 1, s_pSaveIPC         },
 { sizeof ( s_pBrowsSettings1  ) - 1, s_pBrowsSettings1  },
 { sizeof ( s_pUseBgImage      ) - 1, s_pUseBgImage      },
 { sizeof ( s_pSortFSObjects   ) - 1, s_pSortFSObjects   },
 { sizeof ( s_pFilterMFiles    ) - 1, s_pFilterMFiles    },
 { sizeof ( s_pDisplayHDLPart  ) - 1, s_pDisplayHDLPart  },
 { sizeof ( s_pHideSysPart     ) - 1, s_pHideSysPart     },
 { sizeof ( s_pActBorderClr    ) - 1, s_pActBorderClr    },
 { sizeof ( s_pInactBorderClr  ) - 1, s_pInactBorderClr  },
 { sizeof ( s_pFontClr         ) - 1, s_pFontClr         },
 { sizeof ( s_pSLTextColor     ) - 1, s_pSLTextColor     },
 { sizeof ( s_pPlayerSettings1 ) - 1, s_pPlayerSettings1 },
 { sizeof ( s_pDefaultVolume   ) - 1, s_pDefaultVolume   },
 { sizeof ( s_pSubAlignment    ) - 1, s_pSubAlignment    },
 { sizeof ( s_pSubOffset       ) - 1, s_pSubOffset       },
 { sizeof ( s_pSubAutoload     ) - 1, s_pSubAutoload     },
 { sizeof ( s_pSubOpaque       ) - 1, s_pSubOpaque       },
 { sizeof ( s_pSubColor        ) - 1, s_pSubColor        },
 { sizeof ( s_pSubBColor       ) - 1, s_pSubBColor       },
 { sizeof ( s_pSubIColor       ) - 1, s_pSubIColor       },
 { sizeof ( s_pSubUColor       ) - 1, s_pSubUColor       },
 { sizeof ( s_pAutoPowerOff    ) - 1, s_pAutoPowerOff    },
 { sizeof ( s_pScrollBarLen    ) - 1, s_pScrollBarLen    },
 { sizeof ( s_pScrollBarPos    ) - 1, s_pScrollBarPos    },
 { sizeof ( s_pDisplaySBTime   ) - 1, s_pDisplaySBTime   },
 { sizeof ( s_pScrollbarClr    ) - 1, s_pScrollbarClr    },
 { sizeof ( s_pVolumeBarClr    ) - 1, s_pVolumeBarClr    },
 { sizeof ( s_pAudioAnimDisp   ) - 1, s_pAudioAnimDisp   },
 { sizeof ( s_pTop             ) - 1, s_pTop             },
 { sizeof ( s_pBottom          ) - 1, s_pBottom          },
 { sizeof ( s_pOff             ) - 1, s_pOff             },
 { sizeof ( s_pCenter          ) - 1, s_pCenter          },
 { sizeof ( s_pLeft            ) - 1, s_pLeft            },
 { sizeof ( s_pMin             ) - 1, s_pMin             },
 { sizeof ( s_pPts             ) - 1, s_pPts             },
 { sizeof ( s_pSubFontHSize    ) - 1, s_pSubFontHSize    },
 { sizeof ( s_pSubFontVSize    ) - 1, s_pSubFontVSize    },
 { sizeof ( s_pQuickHelp       ) - 1, s_pQuickHelp       },
 { sizeof ( s_pSpace           ) - 1, s_pSpace           },
 { sizeof ( s_pHelp01          ) - 1, s_pHelp01          },
 { sizeof ( s_pHelp02          ) - 1, s_pHelp02          },
 { sizeof ( s_pHelp03          ) - 1, s_pHelp03          },
 { sizeof ( s_pHelp04          ) - 1, s_pHelp04          },
 { sizeof ( s_pHelp05          ) - 1, s_pHelp05          },
 { sizeof ( s_pHelp06          ) - 1, s_pHelp06          },
 { sizeof ( s_pHelp07          ) - 1, s_pHelp07          },
 { sizeof ( s_pHelp08          ) - 1, s_pHelp08          },
 { sizeof ( s_pHelp09          ) - 1, s_pHelp09          },
 { sizeof ( s_pHelp10          ) - 1, s_pHelp10          },
 { sizeof ( s_pHelp11          ) - 1, s_pHelp11          },
 { sizeof ( s_pHelp12          ) - 1, s_pHelp12          },
 { sizeof ( s_pHelp13          ) - 1, s_pHelp13          },
 { sizeof ( s_pHelp14          ) - 1, s_pHelp14          },
 { sizeof ( s_pHelp15          ) - 1, s_pHelp15          },
 { sizeof ( s_pHelp16          ) - 1, s_pHelp16          },
 { sizeof ( s_pHelp17          ) - 1, s_pHelp17          },
 { sizeof ( s_pHelp18          ) - 1, s_pHelp18          },
 { sizeof ( s_pHelp19          ) - 1, s_pHelp19          },
 { sizeof ( s_pHelp20          ) - 1, s_pHelp20          },
 { sizeof ( s_pHelp21          ) - 1, s_pHelp21          },
 { sizeof ( s_pHelp22          ) - 1, s_pHelp22          },
 { sizeof ( s_pHelp23          ) - 1, s_pHelp23          },
 { sizeof ( s_pHelp24          ) - 1, s_pHelp24          },
 { sizeof ( s_pHelp25          ) - 1, s_pHelp25          },
 { sizeof ( s_pHelp26          ) - 1, s_pHelp26          },
 { sizeof ( s_pHelp27          ) - 1, s_pHelp27          },
 { sizeof ( s_pHelp28          ) - 1, s_pHelp28          },
 { sizeof ( s_pHelp29          ) - 1, s_pHelp29          },
 { sizeof ( s_pHelp30          ) - 1, s_pHelp30          },
 { sizeof ( s_pHelp31          ) - 1, s_pHelp31          },
 { sizeof ( s_pHelp32          ) - 1, s_pHelp32          },
 { sizeof ( s_pHelp33          ) - 1, s_pHelp33          },
 { sizeof ( s_pError           ) - 1, s_pError           },
 { sizeof ( s_pSavingIPConfig  ) - 1, s_pSavingIPConfig  },
 { sizeof ( s_pSelBarClr       ) - 1, s_pSelBarClr       },
 { sizeof ( s_pAdvanced        ) - 1, s_pAdvanced        },
 { sizeof ( s_pDispHeight      ) - 1, s_pDispHeight      },
 { sizeof ( s_pApplySettings   ) - 1, s_pApplySettings   },
 { sizeof ( s_pAdvanced1       ) - 1, s_pAdvanced1       },
 { sizeof ( s_pSampleStr       ) - 1, s_pSampleStr       },
 { sizeof ( g_pDefStr          ) - 1, g_pDefStr          },
 { sizeof ( s_pUDFStr          ) - 1, s_pUDFStr          },
 { sizeof ( s_pLangStr         ) - 1, s_pLangStr         },
 { sizeof ( s_pLoadIdx         ) - 1, s_pLoadIdx         },
 { sizeof ( s_pLoadSub         ) - 1, s_pLoadSub         },
 { sizeof ( s_pSubError        ) - 1, s_pSubError        },
 { sizeof ( s_pFormat          ) - 1, s_pFormat          },
 { sizeof ( s_pSequence        ) - 1, s_pSequence        },
 { sizeof ( s_pBufferingFile   ) - 1, s_pBufferingFile   },
 { sizeof ( s_pDetectingFmt    ) - 1, s_pDetectingFmt    },
 { sizeof ( s_pPause           ) - 1, s_pPause           },
 { sizeof ( s_pStopping        ) - 1, s_pStopping        },
 { sizeof ( s_pDefault         ) - 1, s_pDefault         },
 { sizeof ( s_pVA              ) - 1, s_pVA              },
 { sizeof ( s_pSV              ) - 1, s_pSV              },
 { sizeof ( s_pPlay            ) - 1, s_pPlay            },
 { sizeof ( s_pFFwd            ) - 1, s_pFFwd            },
 { sizeof ( s_pRew             ) - 1, s_pRew             },
 { sizeof ( s_pCurs            ) - 1, s_pCurs            },
 { sizeof ( s_pUnsupportedFile ) - 1, s_pUnsupportedFile },
 { sizeof ( s_pRem             ) - 1, s_pRem             },
 { sizeof ( s_pPlayerMenu      ) - 1, s_pPlayerMenu      },
 { sizeof ( s_pDisplay         ) - 1, s_pDisplay         },
 { sizeof ( s_pLetterbox       ) - 1, s_pLetterbox       },
 { sizeof ( s_pPanScan1        ) - 1, s_pPanScan1        },
 { sizeof ( s_pPanScan2        ) - 1, s_pPanScan2        },
 { sizeof ( s_pPanScan3        ) - 1, s_pPanScan3        },
 { sizeof ( s_pFullscreen      ) - 1, s_pFullscreen      },
 { sizeof ( s_pDisplaySubs     ) - 1, s_pDisplaySubs     },
 { sizeof ( s_pSelectSubs      ) - 1, s_pSelectSubs      },
 { sizeof ( s_pBootBrowser     ) - 1, s_pBootBrowser     },
 { sizeof ( s_pExec0           ) - 1, s_pExec0           },
 { sizeof ( s_pExec1           ) - 1, s_pExec1           },
 { sizeof ( s_pExitTo          ) - 1, s_pExitTo          },
 { sizeof ( s_pSoundFX         ) - 1, s_pSoundFX         }
};

static SMString s_SMStringUDF[ sizeof ( s_SMStringDef ) / sizeof ( s_SMStringDef[ 0 ] ) ] __attribute__(   (  section( ".bss" )  )   );
       SMString g_SMString   [ sizeof ( s_SMStringDef ) / sizeof ( s_SMStringDef[ 0 ] ) ] __attribute__(   (  section( ".bss" )  )   );

void SMS_LocaleInit ( void ) {

 int lFD = fioOpen ( s_SMSLng, O_RDONLY );

 memcpy (  g_SMString, s_SMStringDef, sizeof ( g_SMString )  );

 if ( lFD >= 0 ) {

  long lSize = fioLseek ( lFD, 0, SEEK_END );

  if ( lSize > 0 ) {

   unsigned int   lIdx;
   unsigned char* lpEnd;
   unsigned char* lpPtr;
   unsigned char* lpBuff = lpPtr = ( unsigned char* )malloc ( lSize + 1 );

   lpEnd = lpBuff + lSize;
   lIdx  = 0;

   fioLseek ( lFD, 0, SEEK_SET );
   fioRead ( lFD, lpBuff, lSize );

   while ( 1 ) {

    while ( lpPtr != lpEnd && *lpPtr != '\r' && *lpPtr != '\n' ) ++lpPtr;

    *lpPtr = '\x00';

    s_SMStringUDF[ lIdx ].m_pStr = lpBuff;
    s_SMStringUDF[ lIdx ].m_Len  = lpPtr - lpBuff;

    if ( lpPtr++ == lpEnd || !s_SMStringUDF[ lIdx++ ].m_Len ) break;

    if (  lIdx == sizeof ( s_SMStringUDF ) / sizeof ( s_SMStringUDF[ 0 ] )  ) break;
    if ( *lpPtr  == '\n'  ) ++lpPtr;

    lpBuff = lpPtr;

   }  /* end while */

   if (  lIdx != sizeof ( s_SMStringUDF ) / sizeof ( s_SMStringUDF[ 0 ] )  )

    g_Config.m_BrowserFlags &= ~SMS_BF_UDFL;

   else g_Config.m_BrowserFlags |= SMS_BF_UDFL;

  }  /* end if */

  fioClose ( lFD );

 }  /* end else */

 STR_EXEC1.m_pStr[ 6 ] = g_pBXDATASYS[ 6 ];

}  /* end SMS_LocaleInit */

void SMS_LocaleSet ( void ) {

 SMString* lpStr;

 if (   strcmp ( g_Config.m_Language, g_pDefStr ) && ( g_Config.m_BrowserFlags & SMS_BF_UDFL )  )

  lpStr = s_SMStringUDF;

 else lpStr = s_SMStringDef;

 memcpy (  g_SMString, lpStr, sizeof ( g_SMString )  );

}  /* end SMS_LocaleSet */
