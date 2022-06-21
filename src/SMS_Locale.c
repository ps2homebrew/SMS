/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2007 lior e
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_Locale.h"
#include "SMS_Config.h"
#include "SMS_MC.h"

#include <string.h>
#include <fileio.h>
#include <malloc.h>

char g_SMSLng[ 12 ] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "SMS/SMS.lng";

char g_EmptyStr  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "";
char g_SlashStr  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "/";
char g_BSlashStr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "\\";
char g_ColonStr  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ":";
char g_ColonSStr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ": ";
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
char g_pASFStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "ASF";
char g_pExtM3UStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "#EXTM3U";
char g_pExtInfStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "#EXTINF";
char g_pM3UStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "M3U";
char g_pPercDStr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%d";
char g_pMP3Str   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "MP3";
char g_pCmdPrcStr[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "CMDPROC";
char g_pSubStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "sub";
char g_pSrtStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "srt";
char g_pTxtStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "txt";
char g_pBXDATASYS[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:/B?DATA-SYSTEM/";
char g_pOGGStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "OGG";
char g_pSpaceStr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = " ";
char g_pVerStr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "_VS_";
char g_pSMSSkn   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:/SMS/Skins";
char g_pSMI      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ".smi";
char g_pMPEG12Str[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mpeg12";
char g_pExtSMI   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ".smi";
char g_pExtMBF   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ".mbf";
char g_pMC0SMS   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:SMS";
char g_pMPEG12   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mpeg1/2";
char g_pFmt0     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ", %d%s, %d%s, %d%s";
char g_pFmt1     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%.4s";
char g_pFmt2     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%.4s, %dx%d";
char g_pQPel     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ", qpel";
char g_pGMC      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = ", gmc";
char g_pWMA      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "wma";
char g_pAAC      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "aac";
char g_pMPEGPS   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "MPEG(PS)";
char g_pMOV      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "MOV";
char g_pFmt3     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%s%s%s";
char g_pFLAC     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "FLAC";
char g_pAC3      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "ac3";
char g_pDEV9X    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "dev9x:";
char g_pHDD      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "hdd:";
char g_pPFS      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "pfs:";
char g_pQuote    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "'";
char g_pPeriod   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "...";
char g_pJPEG     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "JPEG";

static char s_pAvailableMedia [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Available media:  ";
static char s_pNone           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "none";
static char s_pInitSMS        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Initializing SMS...";
static char s_pSavingConf     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Saving configuration...";
static char s_pLoading        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Loading %s...";
static char s_pInitNet        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Initializing network...";
static char s_pLocUSBD        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Locating USBD.IRX...";
static char s_pWaitingFMedia  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Waiting for media (press 'start' for menu)...";
static char s_pReadingDisk    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Reading disk...";
static char s_pIllegalDisk    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Illegal disk (press 'cross' to continue)...";
static char s_pReadingMedia   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Reading media...";
static char s_pDisplaySettings[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display settings...";
static char s_pDispSettings1  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display settings";
static char s_pDeviceSettings [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Device settings...";
static char s_pDeviceSettings1[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Device settings";
static char s_pBrowserSettings[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Browser settings...";
static char s_pBrowsSettings1 [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Browser settings";
static char s_pPlayerSettings [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Player settings...";
static char s_pPlayerSettings1[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Player settings";
static char s_pHelp           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Help...";
static char s_pSaveSettings   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Save settings";
static char s_pShutdownConsole[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Shutdown console";
static char s_pExit2BB        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Exit SMS";
static char s_pTVSystem       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "TV system";
static char s_pPAL            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "PAL";
static char s_pNTSC           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "NTSC";
static char s_pAuto           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "auto";
static char s_pCharset        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Character set";
static char s_pWinLatin1      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "WinLatin1";
static char s_pWinLatin2      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "WinLatin2";
static char s_pWinCyrillic    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "WinCyrillic";
static char s_pWinGreek       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "WinGreek";
static char s_pAdjustLeft     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Adjust image left";
static char s_pAdjustRight    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Adjust image right";
static char s_pAdjustUp       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Adjust image up";
static char s_pAdjustDown     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Adjust image down";
static char s_pAutoNet        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Autostart network";
static char s_pAutoUSB        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Autostart USB";
static char s_pAutoHDD        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Autostart HDD";
static char s_pStartNet       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Start network support";
static char s_pStartHDD       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Start HDD support";
static char s_pStartUSB       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Start USB support";
static char s_pEditIPConfig   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Edit IPCONFIG.DAT...";
static char s_pEditIPConfig1  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Edit IPCONFIG.DAT";
static char s_pPS2IP1         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "IP Address (octet 1)";
static char s_pPS2IP2         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "IP Address (octet 2)";
static char s_pPS2IP3         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "IP Address (octet 3)";
static char s_pPS2IP4         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "IP Address (octet 4)";
static char s_pNetMask1       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Net Mask (octet 1)";
static char s_pNetMask2       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Net Mask (octet 2)";
static char s_pNetMask3       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Net Mask (octet 3)";
static char s_pNetMask4       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Net Mask (octet 4)";
static char s_pGateway1       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Gateway Address (octet 1)";
static char s_pGateway2       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Gateway Address (octet 2)";
static char s_pGateway3       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Gateway Address (octet 3)";
static char s_pGateway4       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Gateway Address (octet 4)";
static char s_pSaveIPC        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Save/Update IPCONFIG.DAT";
static char s_pUseBgImage     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Use background image";
static char s_pSortFSObjects  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Sort filesystem objects";
static char s_pFilterMFiles   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Filter media files";
static char s_pDisplayHDLPart [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display HDL partitions";
static char s_pHideSysPart    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Hide system partitions";
static char s_pActBorderClr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Active border color";
static char s_pInactBorderClr [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Inactive border color";
static char s_pFontClr        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Text color";
static char s_pSLTextColor    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Status line text color";
static char s_pDefaultVolume  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Default volume";
static char s_pSubAlignment   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle alignment";
static char s_pSubOffset      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle offset";
static char s_pSubAutoload    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Autoload subtitles";
static char s_pSubOpaque      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Opaque subtitles";
static char s_pSubColor       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle color";
static char s_pSubBColor      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle bold color";
static char s_pSubIColor      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle italic color";
static char s_pSubUColor      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle underline color";
static char s_pAutoPowerOff   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Auto power-off";
static char s_pScrollBarLen   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Scroll bar length";
static char s_pScrollBarPos   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Scroll bar position";
static char s_pDisplaySBTime  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display scroll bar time";
static char s_pScrollbarClr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Scroll bar color";
static char s_pVolumeBarClr   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Volume bar color";
static char s_pAudioAnimDisp  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Audio animation display";
static char s_pTop            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "top";
static char s_pBottom         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "bottom";
static char s_pOff            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "off";
static char s_pCenter         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "center";
static char s_pLeft           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "left";
static char s_pMin            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%d min";
static char s_pPts            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%d pts";
static char s_pSubFontHSize   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle font width";
static char s_pSubFontVSize   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle font height";
static char s_pQuickHelp      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Quick help";
static char s_pSpace          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = " ";
static char s_pHelp01         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "At startup:";
static char s_pHelp02         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+R1 - NTSC mode";
static char s_pHelp03         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+R2 - PAL mode";
static char s_pHelp04         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Browser:";
static char s_pHelp05         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "triangle - parent directory";
static char s_pHelp06         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "left/right - device menu";
static char s_pHelp07         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "up/down - navigate directory";
static char s_pHelp08         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "cross - action";
static char s_pHelp09         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+circle - power off";
static char s_pHelp10         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+R1 - adjust image right";
static char s_pHelp11         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+L1 - adjust image left";
static char s_pHelp12         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+R2 - adjust image down";
static char s_pHelp13         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+L2 - adjust image up";
static char s_pHelp14         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+square - save settings";
static char s_pHelp15         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select+triangle - boot browser";
static char s_pHelp16         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "L1+L2+R1+R2 - display about";
static char s_pHelp17         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Player:";
static char s_pHelp18         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "up/down - adjust volume";
static char s_pHelp19         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "right/left - FFWD/REW mode";
static char s_pHelp20         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "cross - exit FFWD/REW mode";
static char s_pHelp21         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "triangle - stop";
static char s_pHelp22         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "select - pause/timeline";
static char s_pHelp23         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "start - resume/menu";
static char s_pHelp24         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "cross - OSD timer";
static char s_pHelp25         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "rectangle - pan-scan mode";
static char s_pHelp26         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "L1 - pan left";
static char s_pHelp27         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "R1 - pan right";
static char s_pHelp28         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "circle - V/A (S/V) sync mode";
static char s_pHelp29         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "L2 - derease sync value";
static char s_pHelp30         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "R2 - increase sync value";
static char s_pHelp31         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "SMS menu:";
static char s_pHelp32         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "cross/circle - action/next level";
static char s_pHelp33         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "triangle - level up/exit menu";
static char s_pError          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Error (press 'cross' to continue)...";
static char s_pSavingIPConfig [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Saving IPCONFIG.DAT...";
static char s_pSelBarClr      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Selection bar color";
static char s_pAdvanced       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Advanced settings...";
static char s_pAdvanced1      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Advanced settings";
static char s_pDispHeight     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display height";
static char s_pApplySettings  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Apply settings";
static char s_pSampleStr      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Sample";
       char g_pDefStr         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "English";
static char s_pUDFStr         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "user defined";
static char s_pLangStr        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Language";
static char s_pLoadIdx        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Loading indices...";
static char s_pLoadSub        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Loading subtitles...";
static char s_pSubError       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle: %s error (%d), press 'cross' to continue...";
static char s_pFormat         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "format";
static char s_pSequence       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "sequence";
static char s_pBufferingFile  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Buffering %s file...";
static char s_pDetectingFmt   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Detecting file format...";
static char s_pPause          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Pause";
static char s_pStopping       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Stopping";
static char s_pDefault        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "default";
static char s_pVA             [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "V/A:";
static char s_pSV             [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "S/V:";
static char s_pPlay           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Play";
static char s_pFFwd           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "FFwd";
static char s_pRew            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Rew";
static char s_pCurs           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Curs";
static char s_pUnsupportedFile[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Unsupported file format (press 'cross' to continue)...";
static char s_pRem            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Rem";
static char s_pPlayerMenu     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Player menu";
static char s_pDisplay        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display";
static char s_pLetterbox      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "letterbox";
static char s_pPanScan1       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "pan-scan 1";
static char s_pPanScan2       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "pan-scan 2";
static char s_pPanScan3       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "pan-scan 3";
static char s_pFullscreen     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "fullscreen";
static char s_pDisplaySubs    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display subtitles";
static char s_pSelectSubs     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Select subtitles";
static char s_pBootBrowser    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "boot browser";
       char g_pExec0          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:/BOOT/BOOT.ELF";
       char g_pExec1          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc0:/BEDATA-SYSTEM/BOOT.ELF";
static char s_pExitTo         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Exit to";
static char s_pSoundFX        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Sound FX";
static char s_pSPDIFDD        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "S/PDIF - Dolby Digital/DTS";
static char s_pMP3Settings    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "MP3 settings...";
static char s_pRandomizePL    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Randomize playlist";
static char s_pRepeat         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Repeat mode";
static char s_pAudioSpectrum  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Audio spectrum display";
static char s_pCopy2HDD       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Copy to HDD";
static char s_pCopying        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Copying";
static char s_pKBS            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "KB/s";
static char s_pSelectAction   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Select action";
static char s_pColorResolution[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Color resolution";
static char s_p32Bit          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "32 bit";
static char s_p16Bit          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "16 bit";
static char s_pDTV480P        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "DTV 480p";
static char s_pCPort2         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Controller port 2";
static char s_pGamepad        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "gamepad";
static char s_pRemoteControl  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "remote control";
static char s_pAC3RangeLevel  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "AC3 range level";
static char s_pRight          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "right";
static char s_pVESA60Hz       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "VESA (60Hz)";
static char s_pVESA75Hz       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "VESA (75Hz)";
static char s_pWidescreen     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "widescreen";
static char s_pWidePanScan1   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "wide pan-scan 1";
static char s_pWidePanScan2   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "wide pan-scan 2";
static char s_pNetProtocol    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Network protocol";
static char s_pPS2DevHost     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "ps2dev/host";
static char s_pSMB_CIFS       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "SMB/CIFS";
static char s_pCommError      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Communication error (press 'cross' to continue)...";
static char s_pProtNegError   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Protocol negotiation error (%d) (press 'cross' to continue)...";
static char s_pLoginError     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Login error (%d) (press 'cross' to continue)...";
static char s_pSubtitles      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitles";
static char s_pDisableCDVD    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Disable CD/DVD";
static char s_pDispWidth      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Display width";
static char s_pReadingFmt     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Reading %s...";
static char s_pOutOfMemory    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Error: out of memory";
static char s_pCheckingFSpace [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Checking free space...";
static char s_pNotEnoughSpace [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Error: not enough free space on HDD";
static char s_pCreatingDirTree[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Creating directory tree...";
static char s_pDelete         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Delete";
static char s_pDeleting       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Deleting...";
static char s_pCDVDSpeed      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "CD/DVD speed";
static char s_pMedium         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "medium";
static char s_pHigh           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "high";
static char s_pLow            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "low";
static char s_pDirButtons     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Directional buttons";
static char s_pDTV1080I       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "DTV 1080i";
static char s_pDTV720P        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "DTV 720p";
static char s_pOpMode         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Operating mode";
static char s_pAutoNego       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "auto-negotiation";
static char s_pAutomatic      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "automatic";
static char s_pManual         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "manual";
static char s_pDuplexMode     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Duplex mode";
static char s_pFull           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "full";
static char s_pHalf           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "half";
static char s_pStandard       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Standard";
static char s_p10BaseT        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "10Base-T";
static char s_p100BaseTX      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "100Base-TX";
static char s_pNetSettings    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Network settings...";
static char s_pNetSettings1   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Network settings";
static char s_pSyncPar1       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Synch. parameter 1 (video)";
static char s_pSyncPar2       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Synch. parameter 2 (GUI)";
static char s_pSMBServer      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "SMB server...";
static char s_pSMBClosing     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Terminating pending connection...";
static char s_pUpdateLngFile  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Update SMS language";
static char s_pUpdatePalFile  [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Update SMS palette";
static char s_pUpdateSMS      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Update SMS";
static char s_pUpdateSMB      [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Update SMB server list";
static char s_pAddBackImage   [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Add background image";
static char s_pProcessing     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Processing...";
static char s_pCreatingBackup [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Creating SMS backup...";
static char s_pPlayAll        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Play all";
static char s_pAudio          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Audio";
static char s_pVideo          [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Video";
static char s_pMP3Par         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Autodetection parameter";
static char s_pSelectFile     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Start playback from";
static char s_pCenterRight    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "center-right";
static char s_pUserXH         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Use exception handler";
static char s_pSubMBCS        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Subtitle MBCS";
static char s_pLoadingFont    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Loading font...";
static char s_pLoadingODML    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Loading ODML indices %d...";
static char s_pFileSize       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "File size: ";
static char s_pGB             [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "GB";
static char s_pMB             [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "KB";
static char s_pFmt0           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%d,%02d";
static char s_pFmt1           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "%d,%03d";
static char s_pKbS            [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Kb/s";
static char s_pHz             [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Hz";
static char s_pCh             [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "ch";
static char s_pAvailMem       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Available memory: %.1fMB/%.2fMB";
static char s_pResume         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Resume playback from %s ? ('cross'='yes', 'triangle'='no')";
static char s_pImageOffset    [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Image offset";
static char s_pResolution2Big [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Video resolution is too big (press 'cross' to continue)...";
static char s_pSyncPar3       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Synch. parameter 3 (Audio)";
static char s_pImages         [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Images";
static char s_pUCImage        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Unsupported or corrupted image";
static char s_pResetButtonAct [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Reset button action";
static char s_pPowerOff       [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "power off";
static char s_pExit           [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "exit";
static char s_pPulldown22     [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "Video auto 2:2 pulldown";
static char s_pDTV576P        [] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "DTV 576p";

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
 { sizeof ( g_pExec0           ) - 1, g_pExec0           },
 { sizeof ( g_pExec1           ) - 1, g_pExec1           },
 { sizeof ( s_pExitTo          ) - 1, s_pExitTo          },
 { sizeof ( s_pSoundFX         ) - 1, s_pSoundFX         },
 { sizeof ( s_pSPDIFDD         ) - 1, s_pSPDIFDD         },
 { sizeof ( s_pMP3Settings     ) - 1, s_pMP3Settings     },
 { sizeof ( s_pRandomizePL     ) - 1, s_pRandomizePL     },
 { sizeof ( s_pRepeat          ) - 1, s_pRepeat          },
 { sizeof ( s_pAudioSpectrum   ) - 1, s_pAudioSpectrum   },
 { sizeof ( s_pCopy2HDD        ) - 1, s_pCopy2HDD        },
 { sizeof ( s_pCopying         ) - 1, s_pCopying         },
 { sizeof ( s_pKBS             ) - 1, s_pKBS             },
 { sizeof ( s_pSelectAction    ) - 1, s_pSelectAction    },
 { sizeof ( s_pColorResolution ) - 1, s_pColorResolution },
 { sizeof ( s_p32Bit           ) - 1, s_p32Bit           },
 { sizeof ( s_p16Bit           ) - 1, s_p16Bit           },
 { sizeof ( s_pDTV480P         ) - 1, s_pDTV480P         },
 { sizeof ( s_pCPort2          ) - 1, s_pCPort2          },
 { sizeof ( s_pGamepad         ) - 1, s_pGamepad         },
 { sizeof ( s_pRemoteControl   ) - 1, s_pRemoteControl   },
 { sizeof ( s_pAC3RangeLevel   ) - 1, s_pAC3RangeLevel   },
 { sizeof ( s_pRight           ) - 1, s_pRight           },
 { sizeof ( s_pVESA60Hz        ) - 1, s_pVESA60Hz        },
 { sizeof ( s_pVESA75Hz        ) - 1, s_pVESA75Hz        },
 { sizeof ( s_pWidescreen      ) - 1, s_pWidescreen      },
 { sizeof ( s_pWidePanScan1    ) - 1, s_pWidePanScan1    },
 { sizeof ( s_pWidePanScan2    ) - 1, s_pWidePanScan2    },
 { sizeof ( s_pNetProtocol     ) - 1, s_pNetProtocol     },
 { sizeof ( s_pPS2DevHost      ) - 1, s_pPS2DevHost      },
 { sizeof ( s_pSMB_CIFS        ) - 1, s_pSMB_CIFS        },
 { sizeof ( s_pCommError       ) - 1, s_pCommError       },
 { sizeof ( s_pProtNegError    ) - 1, s_pProtNegError    },
 { sizeof ( s_pLoginError      ) - 1, s_pLoginError      },
 { sizeof ( s_pSubtitles       ) - 1, s_pSubtitles       },
 { sizeof ( s_pDisableCDVD     ) - 1, s_pDisableCDVD     },
 { sizeof ( s_pDispWidth       ) - 1, s_pDispWidth       },
 { sizeof ( s_pReadingFmt      ) - 1, s_pReadingFmt      },
 { sizeof ( s_pOutOfMemory     ) - 1, s_pOutOfMemory     },
 { sizeof ( s_pCheckingFSpace  ) - 1, s_pCheckingFSpace  },
 { sizeof ( s_pNotEnoughSpace  ) - 1, s_pNotEnoughSpace  },
 { sizeof ( s_pCreatingDirTree ) - 1, s_pCreatingDirTree },
 { sizeof ( s_pDelete          ) - 1, s_pDelete          },
 { sizeof ( s_pDeleting        ) - 1, s_pDeleting        },
 { sizeof ( s_pCDVDSpeed       ) - 1, s_pCDVDSpeed       },
 { sizeof ( s_pMedium          ) - 1, s_pMedium          },
 { sizeof ( s_pHigh            ) - 1, s_pHigh            },
 { sizeof ( s_pLow             ) - 1, s_pLow             },
 { sizeof ( s_pDirButtons      ) - 1, s_pDirButtons      },
 { sizeof ( s_pDTV1080I        ) - 1, s_pDTV1080I        },
 { sizeof ( s_pDTV720P         ) - 1, s_pDTV720P         },
 { sizeof ( s_pOpMode          ) - 1, s_pOpMode          },
 { sizeof ( s_pAutoNego        ) - 1, s_pAutoNego        },
 { sizeof ( s_pAutomatic       ) - 1, s_pAutomatic       },
 { sizeof ( s_pManual          ) - 1, s_pManual          },
 { sizeof ( s_pDuplexMode      ) - 1, s_pDuplexMode      },
 { sizeof ( s_pFull            ) - 1, s_pFull            },
 { sizeof ( s_pHalf            ) - 1, s_pHalf            },
 { sizeof ( s_pStandard        ) - 1, s_pStandard        },
 { sizeof ( s_p10BaseT         ) - 1, s_p10BaseT         },
 { sizeof ( s_p100BaseTX       ) - 1, s_p100BaseTX       },
 { sizeof ( s_pNetSettings     ) - 1, s_pNetSettings     },
 { sizeof ( s_pNetSettings1    ) - 1, s_pNetSettings1    },
 { sizeof ( s_pSyncPar1        ) - 1, s_pSyncPar1        },
 { sizeof ( s_pSyncPar2        ) - 1, s_pSyncPar2        },
 { sizeof ( s_pSMBServer       ) - 1, s_pSMBServer       },
 { sizeof ( s_pSMBClosing      ) - 1, s_pSMBClosing      },
 { sizeof ( s_pUpdateLngFile   ) - 1, s_pUpdateLngFile   },
 { sizeof ( s_pUpdatePalFile   ) - 1, s_pUpdatePalFile   },
 { sizeof ( s_pUpdateSMS       ) - 1, s_pUpdateSMS       },
 { sizeof ( s_pUpdateSMB       ) - 1, s_pUpdateSMB       },
 { sizeof ( s_pAddBackImage    ) - 1, s_pAddBackImage    },
 { sizeof ( s_pProcessing      ) - 1, s_pProcessing      },
 { sizeof ( s_pCreatingBackup  ) - 1, s_pCreatingBackup  },
 { sizeof ( s_pPlayAll         ) - 1, s_pPlayAll         },
 { sizeof ( s_pAudio           ) - 1, s_pAudio           },
 { sizeof ( s_pVideo           ) - 1, s_pVideo           },
 { sizeof ( s_pMP3Par          ) - 1, s_pMP3Par          },
 { sizeof ( s_pSelectFile      ) - 1, s_pSelectFile      },
 { sizeof ( s_pCenterRight     ) - 1, s_pCenterRight     },
 { sizeof ( s_pUserXH          ) - 1, s_pUserXH          },
 { sizeof ( s_pSubMBCS         ) - 1, s_pSubMBCS         },
 { sizeof ( s_pLoadingFont     ) - 1, s_pLoadingFont     },
 { sizeof ( s_pLoadingODML     ) - 1, s_pLoadingODML     },
 { sizeof ( s_pFileSize        ) - 1, s_pFileSize        },
 { sizeof ( s_pGB              ) - 1, s_pGB              },
 { sizeof ( s_pMB              ) - 1, s_pMB              },
 { sizeof ( s_pFmt0            ) - 1, s_pFmt0            },
 { sizeof ( s_pFmt1            ) - 1, s_pFmt1            },
 { sizeof ( s_pKbS             ) - 1, s_pKbS             },
 { sizeof ( s_pHz              ) - 1, s_pHz              },
 { sizeof ( s_pCh              ) - 1, s_pCh              },
 { sizeof ( s_pAvailMem        ) - 1, s_pAvailMem        },
 { sizeof ( s_pResume          ) - 1, s_pResume          },
 { sizeof ( s_pImageOffset     ) - 1, s_pImageOffset     },
 { sizeof ( s_pResolution2Big  ) - 1, s_pResolution2Big  },
 { sizeof ( s_pSyncPar3        ) - 1, s_pSyncPar3        },
 { sizeof ( s_pImages          ) - 1, s_pImages          },
 { sizeof ( s_pUCImage         ) - 1, s_pUCImage         },
 { sizeof ( s_pResetButtonAct  ) - 1, s_pResetButtonAct  },
 { sizeof ( s_pPowerOff        ) - 1, s_pPowerOff        },
 { sizeof ( s_pExit            ) - 1, s_pExit            },
 { sizeof ( s_pPulldown22      ) - 1, s_pPulldown22      },
 { sizeof ( s_pDTV576P         ) - 1, s_pDTV576P         }
};

static unsigned char s_XLTLatin1[ 256 ] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = {
 0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB, 0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5, 
 0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9, 0xFF, 0xD6, 0xDC, 0xF8, 0xA3, 0xD8, 0xD7, 0x83, 
 0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA, 0xBF, 0xAE, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB, 
 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xC1, 0xC2, 0xC0, 0xA9, 0xA6, 0xA6, 0x2B, 0x2B, 0xA2, 0xA5, 0x2B, 
 0x2B, 0x2D, 0x2D, 0x2B, 0x2D, 0x2B, 0xE3, 0xC3, 0x2B, 0x2B, 0x2D, 0x2D, 0xA6, 0x2D, 0x2B, 0xA4, 
 0xF0, 0xD0, 0xCA, 0xCB, 0xC8, 0x69, 0xCD, 0xCE, 0xCF, 0x2B, 0x2B, 0xA6, 0x5F, 0xA6, 0xCC, 0xAF, 
 0xD3, 0xDF, 0xD4, 0xD2, 0xF5, 0xD5, 0xB5, 0xFE, 0xDE, 0xDA, 0xDB, 0xD9, 0xFD, 0xDD, 0xAF, 0xB4, 
 0xAD, 0xB1, 0x3D, 0xBE, 0xB6, 0xA7, 0xF7, 0xB8, 0xB0, 0xA8, 0xB7, 0xB9, 0xB3, 0xB2, 0xA6, 0xA0, 
 0x3F, 0x3F, 0x27, 0x9F, 0x22, 0x2E, 0xC5, 0xCE, 0x5E, 0x25, 0x53, 0x3C, 0x4F, 0x3F, 0x5A, 0x3F, 
 0x3F, 0x27, 0x27, 0x22, 0x22, 0x3F, 0x2D, 0x2D, 0x7E, 0x54, 0x73, 0x3E, 0x6F, 0x3F, 0x7A, 0x59, 
 0xFF, 0xAD, 0xBD, 0x9C, 0xCF, 0xBE, 0xDD, 0xF5, 0xF9, 0xB8, 0xA6, 0xAE, 0xAA, 0xF0, 0xA9, 0xEE, 
 0xF8, 0xF1, 0xFD, 0xFC, 0xEF, 0xE6, 0xF4, 0xFA, 0xF7, 0xFB, 0xA7, 0xAF, 0xAC, 0xAB, 0xF3, 0xA8, 
 0xB7, 0xB5, 0xB6, 0xC7, 0x8E, 0x8F, 0x92, 0x80, 0xD4, 0x90, 0xD2, 0xD3, 0xDE, 0xD6, 0xD7, 0xD8, 
 0xD1, 0xA5, 0xE3, 0xE0, 0xE2, 0xE5, 0x99, 0x9E, 0x9D, 0xEB, 0xE9, 0xEA, 0x9A, 0xED, 0xE8, 0xE1, 
 0x85, 0xA0, 0x83, 0xC6, 0x84, 0x86, 0x91, 0x87, 0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B, 
 0xD0, 0xA4, 0x95, 0xA2, 0x93, 0xE4, 0x94, 0xF6, 0x9B, 0x97, 0xA3, 0x96, 0x81, 0xEC, 0xE7, 0x98,
};

static unsigned char s_XLTLatin2[ 256 ] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = {
 0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xF9, 0xE6, 0xE7, 0xB3, 0xEB, 0xD5, 0xF5, 0xEE, 0x8F, 0xC4, 0xC6, 
 0xC9, 0xC5, 0xE5, 0xF4, 0xF6, 0xBC, 0xBE, 0x8C, 0x9C, 0xD6, 0xDC, 0x8D, 0x9D, 0xA3, 0xD7, 0xE8, 
 0xE1, 0xED, 0xF3, 0xFA, 0xA5, 0xB9, 0x8E, 0x9E, 0xCA, 0xEA, 0xAC, 0x9F, 0xC8, 0xBA, 0xAB, 0xBB, 
 0x2D, 0x2D, 0x2D, 0x2D, 0x2B, 0xC1, 0xC2, 0xCC, 0xAA, 0xA6, 0xA6, 0xAC, 0x2D, 0xAF, 0xBF, 0xAC, 
 0x4C, 0x2B, 0x54, 0x2B, 0xA6, 0x2B, 0xC3, 0xE3, 0x4C, 0x2D, 0xA6, 0x54, 0xA6, 0x3D, 0x2B, 0xA4, 
 0xF0, 0xD0, 0xCF, 0xCB, 0xEF, 0xD2, 0xCD, 0xCE, 0xEC, 0x2D, 0x2D, 0x2D, 0x2D, 0xDE, 0xD9, 0x2D, 
 0xD3, 0xDF, 0xD4, 0xD1, 0xF1, 0xF2, 0x8A, 0x9A, 0xC0, 0xDA, 0xE0, 0xDB, 0xFD, 0xDD, 0xFE, 0xB4, 
 0xAD, 0xBD, 0xB2, 0xA1, 0xA2, 0xA7, 0xF7, 0xB8, 0xB0, 0xA8, 0xFF, 0xFB, 0xD8, 0xF8, 0xA6, 0xA0, 
 0x3F, 0x3F, 0x27, 0x3F, 0x22, 0x3F, 0xC5, 0xC5, 0x3F, 0x25, 0xE6, 0x3C, 0x97, 0x9B, 0xA6, 0x8D, 
 0x3F, 0x27, 0x27, 0x22, 0x22, 0x3F, 0x2D, 0x2D, 0x3F, 0x74, 0xE7, 0x3E, 0x98, 0x9C, 0xA7, 0xAB, 
 0xFF, 0xF3, 0xF4, 0x9D, 0xCF, 0xA4, 0x7C, 0xF5, 0xF9, 0x63, 0xB8, 0xAE, 0xAA, 0xF0, 0x52, 0xBD, 
 0xF8, 0x2B, 0xF2, 0x88, 0xEF, 0x75, 0x3F, 0x3F, 0xF7, 0xA5, 0xAD, 0xAF, 0x95, 0xF1, 0x96, 0xBE, 
 0xE8, 0xB5, 0xB6, 0xC6, 0x8E, 0x91, 0x8F, 0x80, 0xAC, 0x90, 0xA8, 0xD3, 0xB7, 0xD6, 0xD7, 0xD2, 
 0xD1, 0xE3, 0xD5, 0xE0, 0xE2, 0x8A, 0x99, 0x9E, 0xFC, 0xDE, 0xE9, 0xEB, 0x9A, 0xED, 0xDD, 0xE1, 
 0xEA, 0xA0, 0x83, 0xC7, 0x84, 0x92, 0x86, 0x87, 0x9F, 0x82, 0xA9, 0x89, 0xD8, 0xA1, 0x8C, 0xD4, 
 0xD0, 0xE4, 0xE5, 0xA2, 0x93, 0x8B, 0x94, 0xF6, 0xFD, 0x85, 0xA3, 0xFB, 0x81, 0xEC, 0xEE, 0xFA
};

static unsigned char s_XLTCyrillic[ 256 ] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = {
 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 
 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 
 0x2D, 0x2D, 0x2D, 0xA6, 0x2B, 0xA6, 0xA6, 0xAC, 0xAC, 0xA6, 0xA6, 0xAC, 0x2D, 0x2D, 0x2D, 0xAC, 
 0x4C, 0x2B, 0x54, 0x2B, 0x2D, 0x2B, 0xA6, 0xA6, 0x4C, 0xE3, 0xA6, 0x54, 0xA6, 0x3D, 0x2B, 0xA6, 
 0xA6, 0x54, 0x54, 0x4C, 0x4C, 0x2D, 0xE3, 0x2B, 0x2B, 0x2D, 0x2D, 0x2D, 0x2D, 0xA6, 0xA6, 0x2D, 
 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 
 0xA8, 0xB8, 0xAA, 0xBA, 0xAF, 0xBF, 0xA1, 0xA2, 0xB0, 0x95, 0xB7, 0x76, 0xB9, 0xA4, 0xA6, 0xA0, 
 0x3F, 0x3F, 0x27, 0x3F, 0x22, 0x3A, 0xC5, 0xD8, 0x3F, 0x25, 0x3F, 0x3C, 0x3F, 0x3F, 0x3F, 0x3F, 
 0x3F, 0x27, 0x27, 0x22, 0x22, 0x07, 0x2D, 0x2D, 0x3F, 0x54, 0x3F, 0x3E, 0x3F, 0x3F, 0x3F, 0x3F, 
 0xFF, 0xF6, 0xF7, 0x3F, 0xFD, 0x3F, 0xB3, 0x15, 0xF0, 0x63, 0xF2, 0x3C, 0xBF, 0x2D, 0x52, 0xF4, 
 0xF8, 0x2B, 0x3F, 0x3F, 0x3F, 0xE7, 0x14, 0xFA, 0xF1, 0xFC, 0xF3, 0x3E, 0x3F, 0x3F, 0x3F, 0xF5, 
 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 
 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF
};

static unsigned char s_XLTGreek[ 256 ]  __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = {
 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 
 0xD1, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 
 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF3, 0xF2, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 
 0x2D, 0x2D, 0x2D, 0xA6, 0x2B, 0x3F, 0x3F, 0x3F, 0x3F, 0xA6, 0xA6, 0xAC, 0x2D, 0x3F, 0x3F, 0xAC, 
 0x4C, 0x2B, 0x54, 0x2B, 0x2D, 0x2B, 0x3F, 0x3F, 0x4C, 0x2D, 0xA6, 0x54, 0xA6, 0x3D, 0x2B, 0x3F, 
 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x2D, 0x2D, 0x2D, 0x2D, 0x3F, 0x3F, 0x2D, 
 0xF9, 0xDC, 0xDD, 0xDE, 0xFA, 0xDF, 0xFC, 0xFD, 0xFB, 0xFE, 0xA2, 0xB8, 0xB9, 0xBA, 0xBC, 0xBE, 
 0xBF, 0xB1, 0x3F, 0x3F, 0xDA, 0xDB, 0x3F, 0x3F, 0xB0, 0x3F, 0xB7, 0x3F, 0x3F, 0xB2, 0xA6, 0xA0, 
 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 
 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 
 0xFF, 0x3F, 0xEA, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 
 0xF8, 0xF1, 0xFD, 0x3F, 0x3F, 0x3F, 0x3F, 0xFA, 0xEB, 0xEC, 0xED, 0x3F, 0xEE, 0x3F, 0xEF, 0xF0, 
 0x3F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 
 0x8F, 0x90, 0x3F, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0xF4, 0xF5, 0xE1, 0xE2, 0xE3, 0xE5, 
 0x3F, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 
 0xA7, 0xA8, 0xAA, 0xA9, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xE0, 0xE4, 0xE8, 0xE6, 0xE7, 0xE9, 0x3F
};

unsigned char* g_XLT[ 4 ] __attribute__(   (  section( ".data" )  )   ) = {
 s_XLTLatin2, s_XLTCyrillic, s_XLTLatin1, s_XLTGreek
};

static unsigned char* s_pUDFBuf;
static SMString       s_SMStringUDF[ sizeof ( s_SMStringDef ) / sizeof ( s_SMStringDef[ 0 ] ) ] __attribute__(   (  section( ".bss" )  )   );
       SMString       g_SMString   [ sizeof ( s_SMStringDef ) / sizeof ( s_SMStringDef[ 0 ] ) ] __attribute__(   (  section( ".bss" )  )   );

void SMS_LocaleInit ( void ) {

 int lFD = MC_OpenS ( g_MCSlot, 0, g_SMSLng, O_RDONLY );

 if ( s_pUDFBuf ) {
  free ( s_pUDFBuf );
  s_pUDFBuf = NULL;
 }  /* end if */

 if ( lFD >= 0 ) {

  s64  lSize = MC_SeekS ( lFD, 0, SEEK_END );

  if ( lSize > 0 ) {

   unsigned int   lIdx;
   unsigned char* lpEnd;
   unsigned char* lpPtr;
   unsigned char* lpAlloc;
   unsigned char* lpBuff = lpPtr = lpAlloc = ( unsigned char* )malloc ( lSize + 1 );

   lpEnd = lpBuff + lSize;
   lIdx  = 0;

   MC_SeekS ( lFD, 0, SEEK_SET   );
   MC_ReadS ( lFD, lpBuff, lSize );

   while ( 1 ) {

    while ( lpPtr != lpEnd && *lpPtr != '\r' && *lpPtr != '\n' ) ++lpPtr;

    *lpPtr = '\x00';

    s_SMStringUDF[ lIdx ].m_pStr = ( char * )lpBuff;
    s_SMStringUDF[ lIdx ].m_Len  = lpPtr - lpBuff;

    if (  !s_SMStringUDF[ lIdx++ ].m_Len ||
          lpPtr++ == lpEnd               ||
          lIdx    == sizeof ( s_SMStringUDF ) / sizeof ( s_SMStringUDF[ 0 ] )
    ) break;

    if ( *lpPtr  == '\n'  ) ++lpPtr;

    lpBuff = lpPtr;

   }  /* end while */

   if (  lIdx != sizeof ( s_SMStringUDF ) / sizeof ( s_SMStringUDF[ 0 ] )  ) {

    free ( lpAlloc );
    g_Config.m_BrowserFlags &= ~SMS_BF_UDFL;

   } else {

    char* lpUDF = s_SMStringUDF[ 167 ].m_pStr;

    s_pUDFBuf = lpAlloc;
    g_Config.m_BrowserFlags |= SMS_BF_UDFL;

    if ( lpUDF[ 0 ] == 'm' && lpUDF[ 1 ] == 'c' &&
         lpUDF[ 3 ] == ':' && lpUDF[ 4 ] == '/' &&
         lpUDF[ 5 ] == 'B' && !strcmp ( &lpUDF[ 7 ], &s_SMStringDef[ 167 ].m_pStr[ 7 ] )
    ) lpUDF[ 6 ] = s_SMStringDef[ 167 ].m_pStr[ 6 ];

   }  /* end else */

  }  /* end if */

  MC_CloseS ( lFD );

 }  /* end else */

}  /* end SMS_LocaleInit */

void SMS_LocaleSet ( void ) {

 SMString* lpStr;

 if (   strcmp ( g_Config.m_Language, g_pDefStr ) && ( g_Config.m_BrowserFlags & SMS_BF_UDFL )  )

  lpStr = s_SMStringUDF;

 else lpStr = s_SMStringDef;

 memcpy (  g_SMString, lpStr, sizeof ( g_SMString )  );

 if ( lpStr == s_SMStringDef ) STR_EXEC1.m_pStr[ 6 ] = g_pBXDATASYS[ 6 ];

}  /* end SMS_LocaleSet */
