/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006-2008 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2006 bix64
# (c) 2007 lior e
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_GUIMenu.h"
#include "SMS_GS.h"
#include "SMS_PAD.h"
#include "SMS_Locale.h"
#include "SMS_GUIcons.h"
#include "SMS_Config.h"
#include "SMS_IOP.h"
#include "SMS_MC.h"
#include "SMS_FileDir.h"
#include "SMS_EE.h"
#include "SMS_Timer.h"
#include "SMS_SPU.h"
#include "SMS_Sounds.h"
#include "SMS_RC.h"
#include "SMS_CDVD.h"
#include "SMS_GUIClock.h"
#include "SMS_IOP.h"
#include "SMS_ioctl.h"

#include <kernel.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <libhdd.h>
#include <sys/ioctl.h>
#include <sifrpc.h>

typedef struct _Sample {

 DECLARE_GUI_OBJECT()

} _Sample;

static int        s_fHDD;
static GUIObject* s_pSample;

static void _display_handler  ( GUIMenu*, int );
static void _device_handler   ( GUIMenu*, int );
static void _browser_handler  ( GUIMenu*, int );
static void _player_handler   ( GUIMenu*, int );
static void _help_handler     ( GUIMenu*, int );
       void _save_handler     ( GUIMenu*, int );
       void _shutdown_handler ( GUIMenu*, int );
       void _exit_handler     ( GUIMenu*, int );

static void _tvsys_handler    ( GUIMenu*, int );
static void _charset_handler  ( GUIMenu*, int );
static void _lang_handler     ( GUIMenu*, int );
       void _adjleft_handler  ( GUIMenu*, int );
       void _adjright_handler ( GUIMenu*, int );
       void _adjup_handler    ( GUIMenu*, int );
       void _adjdown_handler  ( GUIMenu*, int );

static void _cntslot_handler  ( GUIMenu*, int );
static void _autonet_handler  ( GUIMenu*, int );
static void _autousb_handler  ( GUIMenu*, int );
static void _autohdd_handler  ( GUIMenu*, int );
static void _startnet_handler ( GUIMenu*, int );
static void _startusb_handler ( GUIMenu*, int );
static void _starthdd_handler ( GUIMenu*, int );
static void _editipc_handler  ( GUIMenu*, int );
static void _cdvd_handler     ( GUIMenu*, int );
static void _cdvd_spd_handler ( GUIMenu*, int );
static void _dirbtn_handler   ( GUIMenu*, int );

static void _ip1_handler     ( GUIMenu*, int );
static void _ip2_handler     ( GUIMenu*, int );
static void _ip3_handler     ( GUIMenu*, int );
static void _ip4_handler     ( GUIMenu*, int );
static void _nm1_handler     ( GUIMenu*, int );
static void _nm2_handler     ( GUIMenu*, int );
static void _nm3_handler     ( GUIMenu*, int );
static void _nm4_handler     ( GUIMenu*, int );
static void _gw1_handler     ( GUIMenu*, int );
static void _gw2_handler     ( GUIMenu*, int );
static void _gw3_handler     ( GUIMenu*, int );
static void _gw4_handler     ( GUIMenu*, int );
static void _saveipc_handler ( GUIMenu*, int );

static void _usebg_handler  ( GUIMenu*, int );
static void _sound_handler  ( GUIMenu*, int );
static void _sortfs_handler ( GUIMenu*, int );
static void _filter_handler ( GUIMenu*, int );
static void _dsphdl_handler ( GUIMenu*, int );
static void _usexh_handler  ( GUIMenu*, int );
static void _hidesp_handler ( GUIMenu*, int );
static void _abclr_handler  ( GUIMenu*, int );
static void _ibclr_handler  ( GUIMenu*, int );
static void _txtclr_handler ( GUIMenu*, int );
static void _sltxt_handler  ( GUIMenu*, int );
static void _sclr_handler   ( GUIMenu*, int );
static void _exit_2_handler ( GUIMenu*, int );
static void _pwrbtn_handler ( GUIMenu*, int );

static void _autols_handler  ( GUIMenu*, int );
static void _opaqs_handler   ( GUIMenu*, int );
static void _dsbt_handler    ( GUIMenu*, int );
static void _aadsp_handler   ( GUIMenu*, int );
static void _spdif_handler   ( GUIMenu*, int );
static void _pdw22_handler   ( GUIMenu*, int );
       void _subclr_handler  ( GUIMenu*, int );
       void _subbclr_handler ( GUIMenu*, int );
       void _subiclr_handler ( GUIMenu*, int );
       void _subu_handler    ( GUIMenu*, int );
static void _sbclr_handler   ( GUIMenu*, int );
static void _vbclr_handler   ( GUIMenu*, int );
static void _clres_handler   ( GUIMenu*, int );
static void _vol_handler     ( GUIMenu*, int );
static void _salign_handler  ( GUIMenu*, int );
static void _suboff_handler  ( GUIMenu*, int );
static void _poff_handler    ( GUIMenu*, int );
static void _sblen_handler   ( GUIMenu*, int );
static void _sbpos_handler   ( GUIMenu*, int );
static void _sfonth_handler  ( GUIMenu*, int );
static void _sfontv_handler  ( GUIMenu*, int );
static void _smbcs_handler   ( GUIMenu*, int );
static void _advset_handler  ( GUIMenu*, int );
static void _disph_handler   ( GUIMenu*, int );
static void _dispw_handler   ( GUIMenu*, int );
static void _dispc_handler   ( GUIMenu*, int );
static void _disps1_handler  ( GUIMenu*, int );
static void _disps2_handler  ( GUIMenu*, int );
static void _disps3_handler  ( GUIMenu*, int );
static void _apply_handler   ( GUIMenu*, int );

static void _enter_sample ( GUIMenu* );
static void _leave_sample ( GUIMenu* );

static void _mp3_handler        ( GUIMenu*, int );
static void _mp3_rand_handler   ( GUIMenu*, int );
static void _mp3_repeat_handler ( GUIMenu*, int );
static void _mp3_asd_handler    ( GUIMenu*, int );
static void _mp3_adp_handler    ( GUIMenu*, int );

static void _network_handler ( GUIMenu*, int );
static void _netprot_handler ( GUIMenu*, int );
static void _smbsrv_handler  ( GUIMenu*, int );
static void _opmode_handler  ( GUIMenu*, int );
static void _duplex_handler  ( GUIMenu*, int );
static void _standt_handler  ( GUIMenu*, int );

static GUIMenuItem s_SMSMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { 0, &STR_DISPLAY_SETTINGS,     GUICON_DISPLAY, 0, _display_handler,  0, 0 },
 { 0, &STR_DEVICE_SETTINGS,      GUICON_NETWORK, 0, _device_handler,   0, 0 },
 { 0, &STR_BROWSER_SETTINGS,     GUICON_BROWSER, 0, _browser_handler,  0, 0 },
 { 0, &STR_PLAYER_SETTINGS,      GUICON_PLAYER,  0, _player_handler,   0, 0 },
 { 0, &STR_HELP,                 GUICON_HELP,    0, _help_handler,     0, 0 },
 { 0, &STR_SAVE_SETTINGS,        GUICON_SAVE,    0, _save_handler,     0, 0 },
 { 0, &STR_SHUTDOWN_CONSOLE,     GUICON_EXIT,    0, _shutdown_handler, 0, 0 },
 { 0, &STR_EXIT_TO_BOOT_BROWSER, GUICON_FINISH,  0, _exit_handler,     0, 0 }
};

static GUIMenuItem s_DispMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { MENU_ITEM_TYPE_TEXT, &STR_TV_SYSTEM,          0, 0, _tvsys_handler,    0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_CHARACTER_SET,      0, 0, _charset_handler,  0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_LANGUAGE,           0, 0, _lang_handler,     0, 0 },
 {                   0, &STR_ADJUST_IMAGE_LEFT,  0, 0, _adjleft_handler,  0, 0 },
 {                   0, &STR_ADJUST_IMAGE_RIGHT, 0, 0, _adjright_handler, 0, 0 },
 {                   0, &STR_ADJUST_IMAGE_UP,    0, 0, _adjup_handler,    0, 0 },
 {                   0, &STR_ADJUST_IMAGE_DOWN,  0, 0, _adjdown_handler,  0, 0 },
 {                   0, &STR_ADVANCED_SETTINGS,  0, 0, _advset_handler,   0, 0 }
};

static GUIMenuItem s_AdvDispMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { MENU_ITEM_TYPE_TEXT, &STR_DISPLAY_HEIGHT,     0, 0, _disph_handler,  0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_DISPLAY_WIDTH,      0, 0, _dispw_handler,  0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_COLOR_RESOLUTION,   0, 0, _dispc_handler,  0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_SYNC_PAR_1,         0, 0, _disps1_handler, 0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_SYNC_PAR_2,         0, 0, _disps2_handler, 0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_SYNC_PAR_3,         0, 0, _disps3_handler, 0, 0 },
 {                   0, &STR_APPLY_SETTINGS,     0, 0, _apply_handler,  0, 0 }
};

static GUIMenuItem s_DevMenu[ 11 ] __attribute__(   (  section( ".data" )  )   ) = {
 {                   0, &STR_NETWORK_SETTINGS,    0, 0, _network_handler,  0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_CONTROLLER_SLOT2,    0, 0, _cntslot_handler,  0, 0 },
 {                   0, &STR_AUTOSTART_NETWORK,   0, 0, _autonet_handler,  0, 0 },
 {                   0, &STR_AUTOSTART_USB,       0, 0, _autousb_handler,  0, 0 },
 {                   0, &STR_AUTOSTART_HDD,       0, 0, _autohdd_handler,  0, 0 },
 {                   0, &STR_DISABLE_CDVD,        0, 0, _cdvd_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_CDVD_SPEED,          0, 0, _cdvd_spd_handler, 0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_DIRECTIONAL_BUTTONS, 0, 0, _dirbtn_handler,   0, 0 }
};

static GUIMenuItem s_IPCMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { MENU_ITEM_TYPE_TEXT, &STR_PS2_IP1,           0, 0, _ip1_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_PS2_IP2,           0, 0, _ip2_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_PS2_IP3,           0, 0, _ip3_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_PS2_IP4,           0, 0, _ip4_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_NETMASK1,          0, 0, _nm1_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_NETMASK2,          0, 0, _nm2_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_NETMASK3,          0, 0, _nm3_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_NETMASK4,          0, 0, _nm4_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_GATEWAY1,          0, 0, _gw1_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_GATEWAY2,          0, 0, _gw2_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_GATEWAY3,          0, 0, _gw3_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_GATEWAY4,          0, 0, _gw4_handler,     0, 0 },
 { 0,                   &STR_SAVE_IPCONFIG_DAT, 0, 0, _saveipc_handler, 0, 0 }
};

static GUIMenuItem s_BrowserMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { MENU_ITEM_TYPE_TEXT,   &STR_USE_BACKGROUND_IMAGE, 0, 0, _usebg_handler,  0, 0 },
 { 0,                     &STR_SOUND_FX,             0, 0, _sound_handler,  0, 0 },
 { 0,                     &STR_SORT_FS_OBJECTS,      0, 0, _sortfs_handler, 0, 0 },
 { 0,                     &STR_FILTER_MEDIA_FILES,   0, 0, _filter_handler, 0, 0 },
 { 0,                     &STR_DISPLAY_HDL_PART,     0, 0, _dsphdl_handler, 0, 0 },
 { 0,                     &STR_HIDE_SYSTEM_PART,     0, 0, _hidesp_handler, 0, 0 },
 { 0,                     &STR_USE_XH,               0, 0, _usexh_handler,  0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_ACTIVE_BORDER_CLR,    0, 0, _abclr_handler,  0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_INACTIVE_BORDER_CLR,  0, 0, _ibclr_handler,  0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_TEXT_CLR,             0, 0, _txtclr_handler, 0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_STATUS_LINE_TEXT_CLR, 0, 0, _sltxt_handler,  0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SEL_BAR_CLR,          0, 0, _sclr_handler,   0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_RESET_BUTTON_ACTION,  0, 0, _pwrbtn_handler, 0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_EXIT_TO,              0, 0, _exit_2_handler, 0, 0 }

};

static GUIMenuItem s_PlayerMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { 0,                     &STR_MP3_SETTINGS,        0, 0, _mp3_handler,     0, 0 },
 { 0,                     &STR_AUTOLOAD_SUBTITLES,  0, 0, _autols_handler,  0, 0 },
 { 0,                     &STR_OPAQUE_SUBTITLES,    0, 0, _opaqs_handler,   0, 0 },
 { 0,                     &STR_DISPLAY_SB_TIME,     0, 0, _dsbt_handler,    0, 0 },
 { 0,                     &STR_SPDIF_DD,            0, 0, _spdif_handler,   0, 0 },
 { 0,                     &STR_PULLDOWN22,          0, 0, _pdw22_handler,   0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SUBTITLE_COLOR,      0, 0, _subclr_handler,  0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SUBTITLE_BOLD_COLOR, 0, 0, _subbclr_handler, 0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SUBTITLE_ITL_COLOR,  0, 0, _subiclr_handler, 0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SUBTITLE_UND_COLOR,  0, 0, _subu_handler,    0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SCROLLBAR_COLOR,     0, 0, _sbclr_handler,   0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_VOLUME_BAR_COLOR,    0, 0, _vbclr_handler,   0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_COLOR_RESOLUTION,    0, 0, _clres_handler,   0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_DEFAULT_VOLUME,      0, 0, _vol_handler,     0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_SUBTITLE_ALIGNMENT,  0, 0, _salign_handler,  0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_AUTO_POWER_OFF,      0, 0, _poff_handler,    0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_SCROLLBAR_LENGTH,    0, 0, _sblen_handler,   0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_SCROLLBAR_POSITION,  0, 0, _sbpos_handler,   0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_SUBTITLE_OFFSET,     0, 0, _suboff_handler,  _enter_sample, _leave_sample },
 { MENU_ITEM_TYPE_TEXT,   &STR_SUB_FONT_HSIZE,      0, 0, _sfonth_handler,  _enter_sample, _leave_sample },
 { MENU_ITEM_TYPE_TEXT,   &STR_SUB_FONT_VSIZE,      0, 0, _sfontv_handler,  _enter_sample, _leave_sample },
 { MENU_ITEM_TYPE_TEXT,   &STR_SUBTITLE_MBCS,       0, 0, _smbcs_handler,   0, 0 }
};

static GUIMenuItem s_MP3Menu[] __attribute__(   (  section( ".data" )  )   ) = {
 { 0,                   &STR_AUDIO_ANIM_DISPLAY, 0, 0, _aadsp_handler,      0, 0 },
 { 0,                   &STR_RANDOMIZE_PLAYLIST, 0, 0, _mp3_rand_handler,   0, 0 },
 { 0,                   &STR_REPEAT_MODE,        0, 0, _mp3_repeat_handler, 0, 0 },
 { 0,                   &STR_AUDIO_SPECTRUM_DSP, 0, 0, _mp3_asd_handler,    0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_MP3_PARAMETER,      0, 0, _mp3_adp_handler,    0, 0 }
};

static GUIMenuItem s_HelpMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { 0, &STR_HELP_01, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_02, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_03, 0, 0, 0, 0, 0 },
 { 0, &STR_SPACE,   0, 0, 0, 0, 0 },
 { 0, &STR_HELP_04, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_05, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_06, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_07, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_08, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_09, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_10, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_11, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_12, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_13, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_14, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_15, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_16, 0, 0, 0, 0, 0 },
 { 0, &STR_SPACE,   0, 0, 0, 0, 0 },
 { 0, &STR_HELP_17, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_18, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_19, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_20, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_21, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_22, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_23, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_24, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_25, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_26, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_27, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_28, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_29, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_30, 0, 0, 0, 0, 0 },
 { 0, &STR_SPACE,   0, 0, 0, 0, 0 },
 { 0, &STR_HELP_31, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_32, 0, 0, 0, 0, 0 },
 { 0, &STR_HELP_33, 0, 0, 0, 0, 0 }
};

static GUIMenuItem s_NetMenu[ 6 ] __attribute__(   (  section( ".data" )  )   ) = {
 { 0,                   &STR_EDIT_IPCONFIG,    0, 0, _editipc_handler, 0, 0 },
 { MENU_ITEM_TYPE_TEXT, &STR_NETWORK_PROTOCOL, 0, 0, _netprot_handler, 0, 0 }
};

static void _Sample_Render ( GUIObject* apObj, int aCtx ) {

 if ( !apObj -> m_pGSPacket ) {

  int            lLen   = STR_SAMPLE.m_Len;
  int            lDWC   = GS_TXT_PACKET_SIZE( lLen );
  int            lWidth = GSFont_WidthEx ( STR_SAMPLE.m_pStr, lLen, g_Config.m_SubHIncr );
  u64*           lpDMA  = GSContext_NewList ( lDWC << 1 );

  g_GSCtx.m_TextColor = 0;
  GSFont_RenderEx (
   STR_SAMPLE.m_pStr, lLen, 0, g_GSCtx.m_Height - g_Config.m_PlayerSubOffset, lpDMA, g_Config.m_SubHIncr, g_Config.m_SubVIncr
  );
  GSFont_RenderEx (
   STR_SAMPLE.m_pStr, lLen, g_GSCtx.m_Width - lWidth - 96, g_GSCtx.m_Height - g_Config.m_PlayerSubOffset, lpDMA + lDWC, g_Config.m_SubHIncr, g_Config.m_SubVIncr
  );

  apObj -> m_pGSPacket = lpDMA;

 }  /* end if */

 GSContext_CallList ( aCtx, apObj -> m_pGSPacket );

}  /* end _Sample_Render */

static GUIObject* _create_sample ( void ) {

 _Sample* retVal = ( _Sample* )calloc (  1, sizeof ( _Sample )  );

 retVal -> Render  = _Sample_Render;
 retVal -> Cleanup = GUIObject_Cleanup;

 return ( GUIObject* )retVal;

}  /* end _create_sample */

static void _redraw_sample ( void ) {

 GSContext_NewPacket ( 1, 0, GSPaintMethod_Init );
  GSContext_CallList ( 1, s_pSample -> m_pGSPacket );
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );

}  /* end _redraw_sample */

static void _setup_dimensions ( GUIMenu* apMenu ) {

 int lWidth  = g_GSCtx.m_Width  - ( g_GSCtx.m_Width  >> 2 );
 int lHeight = g_GSCtx.m_Height - ( g_GSCtx.m_Height >> 2 );

 lWidth += lWidth / 12;

 apMenu -> m_X      = ( g_GSCtx.m_Width  - lWidth  ) >> 1;
 apMenu -> m_Y      = (  ( g_GSCtx.m_Height - lHeight ) >> 1  ) + 8;
 apMenu -> m_Width  = lWidth;
 apMenu -> m_Height = lHeight;

}  /* end _setup_dimensions */

void GUIMenuSMS_UpdateStatus ( GUIMenu* apMenu ) {

 int           lIdx;
 char          lBuf[ 1024 ];
 SMS_ListNode* lpNode = apMenu -> m_pState -> m_pHead;
 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )lpNode -> m_Param;

 memset (  lBuf, 0, sizeof ( lBuf )  );
 strncpy ( lBuf, lpState -> m_pTitle -> m_pStr, lpState -> m_pTitle -> m_Len );
 lpNode = lpNode -> m_pNext;
 lIdx   = strlen ( lBuf );

 while ( lpNode ) {

  GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )lpNode -> m_Param;

  lBuf[ lIdx++ ] = '/';
  strncpy ( &lBuf[ lIdx ], lpState -> m_pTitle -> m_pStr, lpState -> m_pTitle -> m_Len );

  lIdx  += lpState -> m_pTitle -> m_Len;
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 GUI_Status ( lBuf );

}  /* end GUIMenuSMS_UpdateStatus */

extern int GUIMenu_HandleEvent ( GUIObject*, u64           );

int GUIMenuSMS_HandleEvent ( GUIObject* apObj, u64           anEvent ) {

 GUIMenu*      lpMenu  = ( GUIMenu* )apObj;
 GUIMenuState* lpState = (  ( GUIMenuState* )( unsigned int )lpMenu -> m_pState -> m_pTail -> m_Param  );

 switch ( anEvent & GUI_MSG_PAD_MASK ) {

  case RC_STOP         :
  case RC_RETURN       :
  case RC_RESET        :
  case SMS_PAD_TRIANGLE: {

   if ( lpState -> m_pCurr -> Leave ) lpState -> m_pCurr -> Leave ( lpMenu );

   if ( lpState -> HandleEvent ) apObj -> HandleEvent = lpState -> HandleEvent;

   if ( !GUI_MenuPopState ( lpMenu )  ) {

    void* lpActive = lpMenu -> m_pActiveObj;

    GUI_DeleteObject ( g_SMSMenuStr );

    g_pActiveNode = lpActive;

    GUI_Redraw ( GUIRedrawMethod_Redraw );
    GUI_UpdateStatus ();

    if ( s_fHDD ) GUI_PostMessage ( GUI_MSG_MOUNT_BIT | GUI_MSG_HDD );

   } else {

    GUIMenuSMS_UpdateStatus ( lpMenu );
    lpMenu -> Redraw ( lpMenu );

   }  /* end else */

  } return GUIHResult_Handled;

 }  /* end switch */

 return GUIMenu_HandleEvent ( apObj, anEvent );

}  /* end GUIMenuSMS_HandleEvent */

void GUIMenuSMS_Redraw ( GUIMenu* apMenu ) {

 if ( apMenu ) apMenu -> Cleanup (  ( GUIObject* )apMenu  );

 GUI_Redraw ( GUIRedrawMethod_Redraw );
 SMS_GUIClockRedraw ();

 if ( apMenu ) GUIMenuSMS_UpdateStatus ( apMenu );

}  /* end GUIMenuSMS_Redraw */

static SMString* s_Charsets[ 4 ] __attribute__(   (  section( ".data" )  )   ) = {
 &STR_WINLATIN2, &STR_WINCYRILLIC, &STR_WINLATIN1, &STR_WINGREEK
};

static SMString* s_VModeName[ 9 ] __attribute__(   (  section( ".data" )  )   ) = {
 &STR_NTSC,     &STR_PAL,       &STR_DTV_480P,   &STR_DTV_576P,
 &STR_DTV_720P, &STR_DTV_1080I, &STR_VESA_60_HZ, &STR_VESA_75_HZ,
 &STR_AUTO
};

static void _update_display_menu ( void ) {

 SMString* lpStr = s_VModeName[ GS_VMode2Index ( g_Config.m_DisplayMode ) ];

 s_DispMenu[ 0 ].m_IconRight = ( unsigned int )lpStr;
 s_DispMenu[ 1 ].m_IconRight = ( unsigned int )s_Charsets[ g_Config.m_DisplayCharset ];

 if (  strcmp ( g_Config.m_Language, g_pDefStr ) && ( g_Config.m_BrowserFlags & SMS_BF_UDFL )  )
  lpStr = &STR_USER_DEFINED;
 else lpStr = &STR_DEFAULT;

 s_DispMenu[ 2 ].m_IconRight = ( unsigned int )lpStr;

}  /* end _update_display_menu */

static void _display_handler ( GUIMenu* apMenu, int aDir ) {

 GUIMenuState* lpState = GUI_MenuPushState ( apMenu );

 _update_display_menu ();

 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_DispMenu;
 lpState -> m_pLast  = &s_DispMenu[ sizeof ( s_DispMenu ) / sizeof ( s_DispMenu[ 0 ] ) - 1 ];
 lpState -> m_pTitle = &STR_DISPLAY_SETTINGS1;

 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _display_handler */

static SMString* s_Speeds[ 3 ] = {
 &STR_LOW, &STR_MEDIUM, &STR_HIGH
};

static void _device_handler ( GUIMenu* apMenu, int aDir ) {

 GUIMenuState* lpState = GUI_MenuPushState ( apMenu );
 unsigned int  lSize   = 7;

 if ( g_Config.m_NetworkFlags & SMS_DF_GAMEPAD )
  s_DevMenu[ 1 ].m_IconRight = ( unsigned int )&STR_GAMEPAD;
 else if (  ( g_IOPFlags & SMS_IOPF_RMMAN ) && ( g_Config.m_NetworkFlags & SMS_DF_REMOTE )  )
  s_DevMenu[ 1 ].m_IconRight = ( unsigned int )&STR_REMOTE_CONTROL;
 else s_DevMenu[ 1 ].m_IconRight = ( unsigned int )&STR_NONE;

 s_DevMenu[ 2 ].m_IconRight = g_Config.m_NetworkFlags & SMS_DF_AUTO_NET ? GUICON_ON   : GUICON_OFF;
 s_DevMenu[ 3 ].m_IconRight = g_Config.m_NetworkFlags & SMS_DF_AUTO_USB ? GUICON_ON   : GUICON_OFF;
 s_DevMenu[ 4 ].m_IconRight = g_Config.m_NetworkFlags & SMS_DF_AUTO_HDD ? GUICON_ON   : GUICON_OFF;
 s_DevMenu[ 5 ].m_IconRight = g_Config.m_NetworkFlags & SMS_DF_CDVD     ? GUICON_ON   : GUICON_OFF;
 s_DevMenu[ 6 ].m_IconRight = ( unsigned int )s_Speeds[ g_Config.m_CDVDSpeed ];
 s_DevMenu[ 7 ].m_IconRight = ( unsigned int )( g_Config.m_BrowserFlags & SMS_BF_DIRB ? &STR_REMOTE_CONTROL : &STR_GAMEPAD );

 if ( g_IOPFlags & SMS_IOPF_DEV9_IS ) {

  if (   !(  g_IOPFlags & ( SMS_IOPF_NET | SMS_IOPF_SMB )  )   ) {
   s_DevMenu[ ++lSize ].m_pOptionName = &STR_START_NETWORK_NOW;
   s_DevMenu[   lSize ].Handler       = _startnet_handler;
  }  /* end if */

  if (  !( g_IOPFlags & SMS_IOPF_HDD )  ) {
   s_DevMenu[ ++lSize ].m_pOptionName = &STR_START_HDD_NOW;
   s_DevMenu[   lSize ].Handler       = _starthdd_handler;
  }  /* end if */

 }  /* end if */

 if (  !( g_IOPFlags & SMS_IOPF_USB )  ) {

  s_DevMenu[ ++lSize ].m_pOptionName = &STR_START_USB_NOW;
  s_DevMenu[   lSize ].Handler       = _startusb_handler;

 }  /* end if */

 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_DevMenu;
 lpState -> m_pLast  = &s_DevMenu[ lSize ];
 lpState -> m_pTitle = &STR_DEVICE_SETTINGS1;

 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _device_handler */

static SMString* s_ExitTo[ 3 ] = {
 &STR_BOOT_BROWSER, &STR_EXEC0, &STR_EXEC1
};

static void _update_skin ( void ) {

 static SMString sl_SkinName;

 if ( !g_Config.m_SkinName[ 0 ] ) {
  sl_SkinName.m_pStr = STR_NONE.m_pStr;
  sl_SkinName.m_Len  = STR_NONE.m_Len;
 } else {
  sl_SkinName.m_pStr = g_Config.m_SkinName;
  sl_SkinName.m_Len  = strlen ( g_Config.m_SkinName );
 }  /* end else */

 s_BrowserMenu[ 0 ].m_IconRight = ( unsigned int )&sl_SkinName;

}  /* end _update_skin */

static void _browser_handler ( GUIMenu* apMenu, int aDir ) {

 GUIMenuState* lpState = GUI_MenuPushState ( apMenu );

 _update_skin ();

 s_BrowserMenu[  1 ].m_IconRight = g_Config.m_BrowserFlags & SMS_BF_SDFX ? GUICON_ON : GUICON_OFF;
 s_BrowserMenu[  2 ].m_IconRight = g_Config.m_BrowserFlags & SMS_BF_SORT ? GUICON_ON : GUICON_OFF;
 s_BrowserMenu[  3 ].m_IconRight = g_Config.m_BrowserFlags & SMS_BF_AVIF ? GUICON_ON : GUICON_OFF;
 s_BrowserMenu[  4 ].m_IconRight = g_Config.m_BrowserFlags & SMS_BF_HDLP ? GUICON_ON : GUICON_OFF;
 s_BrowserMenu[  5 ].m_IconRight = g_Config.m_BrowserFlags & SMS_BF_SYSP ? GUICON_ON : GUICON_OFF;
 s_BrowserMenu[  6 ].m_IconRight = g_Config.m_BrowserFlags & SMS_BF_UXH  ? GUICON_ON : GUICON_OFF;
 s_BrowserMenu[  7 ].m_IconRight = ( unsigned int )&g_Config.m_BrowserABCIdx;
 s_BrowserMenu[  8 ].m_IconRight = ( unsigned int )&g_Config.m_BrowserIBCIdx;
 s_BrowserMenu[  9 ].m_IconRight = ( unsigned int )&g_Config.m_BrowserTxtIdx;
 s_BrowserMenu[ 10 ].m_IconRight = ( unsigned int )&g_Config.m_BrowserSBCIdx;
 s_BrowserMenu[ 11 ].m_IconRight = ( unsigned int )&g_Config.m_BrowserSCIdx;
 s_BrowserMenu[ 12 ].m_IconRight = ( unsigned int )(  ( g_Config.m_BrowserFlags & SMS_BF_EXIT ) ? &STR_EXIT : &STR_POWER_OFF  );
 s_BrowserMenu[ 13 ].m_IconRight = ( unsigned int )s_ExitTo[ g_Config.m_BrowserFlags >> 28 ];
 s_BrowserMenu[ 13 ].m_Type     |= (  GSFont_WidthEx ( STR_EXIT_TO.m_pStr, STR_EXIT_TO.m_Len, -7 ) + 16  ) << 24;

 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_BrowserMenu;
 lpState -> m_pLast  = &s_BrowserMenu[ sizeof ( s_BrowserMenu ) / sizeof ( s_BrowserMenu[ 0 ] ) - 1 ];
 lpState -> m_pTitle = &STR_BROWSER_SETTINGS1;

 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _browser_handler */

static char s_VolumeBuffer[  5 ] __attribute__(   (  section( ".bss" )  )   );
static char s_OffsetBuffer[ 32 ] __attribute__(   (  section( ".bss" )  )   );
static char s_ScrollBuffer[  9 ] __attribute__(   (  section( ".bss" )  )   );
static char s_PowoffBuffer[ 32 ] __attribute__(   (  section( ".bss" )  )   );
static char s_SHSizeBuffer[ 32 ] __attribute__(   (  section( ".bss" )  )   );
static char s_SVSizeBuffer[ 32 ] __attribute__(   (  section( ".bss" )  )   );

SMString g_StrPlayer[ 6 ] = {
 { 0, s_VolumeBuffer },
 { 0, s_OffsetBuffer },
 { 0, s_ScrollBuffer },
 { 0, s_PowoffBuffer },
 { 0, s_SHSizeBuffer },
 { 0, s_SVSizeBuffer }
};

void _update_poff ( int anIncr ) {

 int lTime = ( int )g_Config.m_PowerOff;

 lTime += anIncr;

 if ( lTime < 0 ) {

  strcpy ( s_PowoffBuffer, STR_AUTO.m_pStr );
  lTime = -60000;

 } else if ( lTime == 0 ) {

  strcpy ( s_PowoffBuffer, STR_OFF.m_pStr );

 } else {

  if ( lTime > 5400000 ) lTime = 5400000;

  sprintf ( s_PowoffBuffer, STR_MIN_FORMAT.m_pStr, lTime / 60000 );

 }  /* end else */

 g_Config.m_PowerOff = lTime;

 g_StrPlayer[ 3 ].m_Len = strlen ( s_PowoffBuffer );

}  /* end _update_poff */

static void _update_pstr ( int anIncr ) {

 SMString* lpStr;

 _update_poff ( anIncr );

 switch ( g_Config.m_PlayerSAlign ) {

  default:
  case 0 : lpStr = &STR_CENTER;       break;
  case 1 : lpStr = &STR_LEFT;         break;
  case 2 : lpStr = &STR_RIGHT;        break;
  case 3 : lpStr = &STR_CENTER_RIGHT; break;

 }  /* end switch */

 s_PlayerMenu[ 14 ].m_IconRight = ( unsigned int )lpStr;
 s_PlayerMenu[ 15 ].m_IconRight = ( unsigned int )&g_StrPlayer[ 3 ];

 if ( g_Config.m_PlayerFlags & SMS_PF_C32 )
  lpStr = &STR_32_BIT;
 else if ( g_Config.m_PlayerFlags & SMS_PF_C16 )
  lpStr = &STR_16_BIT;
 else lpStr = &STR_AUTO;

 s_PlayerMenu[ 12 ].m_IconRight = ( unsigned int )lpStr;

 sprintf (    s_OffsetBuffer, "%d",                  g_Config.m_PlayerSubOffset                                                      );
 sprintf (    s_VolumeBuffer, "%d%%",                ( int )(   (  ( float )g_Config.m_PlayerVolume / 24.0F ) * 100.0F + 0.5F   )    );
 sprintf (    s_ScrollBuffer, STR_PTS_FORMAT.m_pStr, g_Config.m_ScrollBarNum                                                         );
 sprintf (    s_SHSizeBuffer, "%d",                  32 + g_Config.m_SubHIncr                                                        );
 sprintf (    s_SVSizeBuffer, "%d",                  32 + g_Config.m_SubVIncr                                                        );

 g_StrPlayer[ 0 ].m_Len = strlen ( s_VolumeBuffer );
 g_StrPlayer[ 1 ].m_Len = strlen ( s_OffsetBuffer );
 g_StrPlayer[ 2 ].m_Len = strlen ( s_ScrollBuffer );
 g_StrPlayer[ 4 ].m_Len = strlen ( s_SHSizeBuffer );
 g_StrPlayer[ 5 ].m_Len = strlen ( s_SVSizeBuffer );

 s_PlayerMenu[ 13 ].m_IconRight = ( unsigned int )&g_StrPlayer[ 0 ];
 s_PlayerMenu[ 18 ].m_IconRight = ( unsigned int )&g_StrPlayer[ 1 ];
 s_PlayerMenu[ 16 ].m_IconRight = ( unsigned int )&g_StrPlayer[ 2 ];
 s_PlayerMenu[ 19 ].m_IconRight = ( unsigned int )&g_StrPlayer[ 4 ];
 s_PlayerMenu[ 20 ].m_IconRight = ( unsigned int )&g_StrPlayer[ 5 ];

 switch ( g_Config.m_ScrollBarPos ) {

  case SMScrollBarPos_Top     : lpStr = &STR_TOP;    break;
  case SMScrollBarPos_Bottom  : lpStr = &STR_BOTTOM; break;
  case SMScrollBarPos_Inactive: lpStr = &STR_OFF;    break;

 }  /* end switch */

 s_PlayerMenu[ 17 ].m_IconRight = ( unsigned int )lpStr;

}  /* end _update_pstr */

static void _update_mbf ( void ) {

 static SMString sl_MBFName;

 if ( !g_Config.m_MBFName[ 0 ] ) {
  sl_MBFName.m_pStr = STR_NONE.m_pStr;
  sl_MBFName.m_Len  = STR_NONE.m_Len;
 } else {
  sl_MBFName.m_pStr = g_Config.m_MBFName;
  sl_MBFName.m_Len  = strlen ( g_Config.m_MBFName );
 }  /* end else */

 s_PlayerMenu[ 21 ].m_IconRight = ( unsigned int )&sl_MBFName;

}  /* end _update_mbf */

static void _player_handler ( GUIMenu* apMenu, int aDir ) {

 GUIMenuState*  lpState = GUI_MenuPushState ( apMenu );
 unsigned short lVideoMode = GS_Params () -> m_GSCRTMode;

 s_PlayerMenu[  1 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_SUBS  ? GUICON_ON : GUICON_OFF;
 s_PlayerMenu[  2 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_OSUB  ? GUICON_ON : GUICON_OFF;
 s_PlayerMenu[  3 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_TIME  ? GUICON_ON : GUICON_OFF;
 s_PlayerMenu[  4 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_SPDIF ? GUICON_ON : GUICON_OFF;
 s_PlayerMenu[  5 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_PDW22 ? GUICON_ON : GUICON_OFF;
 s_PlayerMenu[  6 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerSCNIdx;
 s_PlayerMenu[  7 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerSCBIdx;
 s_PlayerMenu[  8 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerSCIIdx;
 s_PlayerMenu[  9 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerSCUIdx;
 s_PlayerMenu[ 10 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerSBCIdx;
 s_PlayerMenu[ 11 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerVBCIdx;

 if (  ( lVideoMode == GSVideoMode_PAL ) || ( lVideoMode == GSVideoMode_DTV_640x576P )  ) {
  s_PlayerMenu[ 5 ].m_Type &= ~MENU_ITEM_HIDDEN;
  if ( g_Config.m_PlayerFlags & SMS_PF_PDW22 ) s_PlayerMenu[ 4 ].m_Type |= MENU_ITEM_HIDDEN;
 } else {
  s_PlayerMenu[ 5 ].m_Type |=  MENU_ITEM_HIDDEN;
  s_PlayerMenu[ 4 ].m_Type &= ~MENU_ITEM_HIDDEN;
 }  /* end else */

 _update_pstr ( 0 );
 _update_mbf  ();

 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_PlayerMenu;
 lpState -> m_pLast  = &s_PlayerMenu[ sizeof ( s_PlayerMenu ) / sizeof ( s_PlayerMenu[ 0 ] ) - 1 ];
 lpState -> m_pTitle = &STR_PLAYER_SETTINGS1;

 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _player_handler */

static void _help_handler ( GUIMenu* apMenu, int aDir ) {

 GUIMenuState* lpState = GUI_MenuPushState ( apMenu );

 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_HelpMenu;
 lpState -> m_pLast  = &s_HelpMenu[ sizeof ( s_HelpMenu ) / sizeof ( s_HelpMenu[ 0 ] ) - 1 ];
 lpState -> m_pTitle = &STR_QUICK_HELP;
 lpState -> m_Flags  = MENU_FLAGS_TEXT;

 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _help_handler */

void _save_handler ( GUIMenu* apMenu, int aDir ) {

 GUI_Status ( STR_SAVING_CONFIGURATION.m_pStr );

 if (  !SMS_SaveConfig ()  ) GUI_Error ( STR_ERROR.m_pStr );

 if ( apMenu )

  GUIMenuSMS_UpdateStatus ( apMenu );

 else GUI_UpdateStatus ();

}  /* end _save_handler */

void _shutdown_handler ( GUIMenu* apMenu, int aDir ) {

 SMS_IOPowerOff ();

}  /* end _shutdown_handler */
#ifdef EMBEDDED
void SMS_EExec ( char* );
__asm__(
 ".text\n\t"
 "SMS_EExec:\n\t"
 "ori   $v1, $zero, 252\n\t"
 "syscall\n\t"
);
#endif  /* EMBEDDED */
void _exit_handler ( GUIMenu* apMenu, int aDir ) {

 int  lIdx = g_Config.m_BrowserFlags >> 28;
 char lBuffer[ 1024 ] __attribute__(   (  aligned( 4 )  )   );

 SMS_TimerDestroy ();
 SPU_Shutdown     ();

 sprintf ( lBuffer, STR_LOADING.m_pStr, s_ExitTo[ lIdx ] -> m_pStr );
 GUI_Status ( lBuffer );

 if ( g_PD >= 0 ) SMS_IOCtl ( g_pPFS, PFS_IOCTL_UMOUNT, NULL );

 if ( !lIdx ) {
#ifdef EMBEDDED
  if ( g_IOPFlags & SMS_IOPF_DEV9_IS ) SMS_IOCtl ( g_pDEV9X, DEV9CTLSHUTDOWN, NULL );
#endif  /* EMBEDDED */
  SMS_IOPReset ( 1 );
#ifndef EMBEDDED
  Exit ( 0 );
#else
  SifExitRpc ();
  __asm__ __volatile__(
   "ori $v1, $zero, 252\n\t"
   "syscall\n\t"
  );
#endif  /* EMBEDDED */

 } else {
#ifdef EMBEDDED
  SifExitRpc ();
#endif  /* EMBEDDED */
  SMS_EExec ( s_ExitTo[ lIdx ] -> m_pStr );
 }  /* end else */

}  /* end _exit_handler */

static void _reinitialize ( GUIMenu* apMenu ) {

 GUI_MenuPopState ( apMenu );

 _display_handler ( apMenu, 0 );

 apMenu -> Cleanup (  ( GUIObject* )apMenu  );
 GUI_Initialize ( 0 );
 _setup_dimensions ( apMenu );
 apMenu -> Redraw ( apMenu );
 SMS_GUIClockStop ();
 GUIMenuSMS_UpdateStatus ( apMenu );
 SMS_GUIClockStart ( &g_Clock );

}  /* end _reinitialize */

static void _tvsys_handler ( GUIMenu* apMenu, int aDir ) {

 static const unsigned char sl_DMode[ 9 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  GSVideoMode_Default,        GSVideoMode_NTSC,
  GSVideoMode_PAL,            GSVideoMode_DTV_720x480P,
  GSVideoMode_DTV_640x576P,   GSVideoMode_DTV_1280x720P,
  GSVideoMode_DTV_1920x1080I, GSVideoMode_VESA_60Hz,
  GSVideoMode_VESA_75Hz
 };

 int i, lDispMode = g_Config.m_DisplayMode;

 for (  i = 0; i < sizeof ( sl_DMode ) / sizeof ( sl_DMode[ 0 ] ); ++i  ) if ( sl_DMode[ i ] == lDispMode ) break;

 i += aDir;

 if ( i < 0 )
  i = sizeof ( sl_DMode ) / sizeof ( sl_DMode[ 0 ] ) - 1;
 else if (  i == sizeof ( sl_DMode ) / sizeof ( sl_DMode[ 0 ] )  )
  i = 0;

 g_Config.m_DisplayMode = sl_DMode[ i ];

 _reinitialize ( apMenu );

}  /* end _tvsys_handler */

static void _charset_handler ( GUIMenu* apMenu, int aDir ) {

 switch ( g_Config.m_DisplayCharset ) {

  case GSCodePage_WinLatin2  : g_Config.m_DisplayCharset = GSCodePage_WinCyrillic; break;
  case GSCodePage_WinCyrillic: g_Config.m_DisplayCharset = GSCodePage_WinGreek;    break;
  default                    :
  case GSCodePage_WinGreek   : g_Config.m_DisplayCharset = GSCodePage_WinLatin1;   break;
  case GSCodePage_WinLatin1  : g_Config.m_DisplayCharset = GSCodePage_WinLatin2;   break;

 }  /* end switch */

 _update_display_menu ();

 g_GSCtx.m_CodePage = g_Config.m_DisplayCharset;
 GSFont_Init   ();
 SMS_IOPSetXLT ();

 GUI_Redraw ( GUIRedrawMethod_InitClearObj );
 GUIMenuSMS_UpdateStatus ( apMenu );

}  /* end _charset_handler */

static void _lang_handler ( GUIMenu* apMenu, int aDir ) {

 if ( g_Config.m_BrowserFlags & SMS_BF_UDFL ) {

  if (  !strcmp ( g_Config.m_Language, g_pDefStr )  )

   g_Config.m_Language[ 0 ] = '\x00';

  else strcpy ( g_Config.m_Language, g_pDefStr );

  SMS_LocaleSet ();

  _update_display_menu ();
  GUI_Redraw ( GUIRedrawMethod_InitClearObj );
  GUIMenuSMS_UpdateStatus ( apMenu );

 }  /* end if */

}  /* end _lang_handler */

extern void _set_dx_dy ( int**, int** );

void _check_dc_offset ( void ) {

 int* lpDX;
 int* lpDY;

 _set_dx_dy ( &lpDX, &lpDY );

 g_GSCtx.m_OffsetX = SMS_clip ( g_GSCtx.m_OffsetX, -160, 160 );
 g_GSCtx.m_OffsetY = SMS_clip ( g_GSCtx.m_OffsetY,  -64,  64 );

 *lpDX = g_GSCtx.m_OffsetX;
 *lpDY = g_GSCtx.m_OffsetY;

}  /* end _check_dc_offset */

static void _init_set_dc ( void ) {

 int lMode       = g_Config.m_DisplayMode;
 int lHDI        = lMode == GSVideoMode_DTV_1920x1080I;
 int lColorDepth = g_Config.m_ColorDepth || lHDI ? GSPixelFormat_PSMCT16
                                                 : GSPixelFormat_PSMCT24;

 if ( lMode == GSVideoMode_Default ) lMode = g_pBXDATASYS[ 6 ] == 'E' ? GSVideoMode_PAL : GSVideoMode_NTSC;

 _check_dc_offset ();

 GS_VSync ();
 GS_InitDC ( &g_GSCtx.m_DispCtx, lColorDepth, g_GSCtx.m_PWidth, g_GSCtx.m_PHeight, g_GSCtx.m_OffsetX, g_GSCtx.m_OffsetY );
 GS_SetDC (  &g_GSCtx.m_DispCtx, ( lMode <= GSVideoMode_PAL ) || lHDI  );

}  /* end _init_set_dc */

void _adjleft_handler ( GUIMenu* apMenu, int aDir ) {

 int* lpDX;
 int* lpDY;

 _set_dx_dy ( &lpDX, &lpDY );

 g_GSCtx.m_OffsetX = *lpDX -= 1;
 _init_set_dc ();

}  /* end _adjleft_handler */

void _adjright_handler ( GUIMenu* apMenu, int aDir ) {

 int* lpDX;
 int* lpDY;

 _set_dx_dy ( &lpDX, &lpDY );

 g_GSCtx.m_OffsetX = *lpDX += 1;
 _init_set_dc ();

}  /* end _adjright_handler */

void _adjup_handler ( GUIMenu* apMenu, int aDir ) {

 int* lpDX;
 int* lpDY;

 _set_dx_dy ( &lpDX, &lpDY );

 *lpDY += *lpDY & 1;

 g_GSCtx.m_OffsetY = *lpDY -= 2;
 _init_set_dc ();

}  /* end _adjup_handler */

void _adjdown_handler ( GUIMenu* apMenu, int aDir ) {

 int* lpDX;
 int* lpDY;

 _set_dx_dy ( &lpDX, &lpDY );

 *lpDY += *lpDY & 1;

 g_GSCtx.m_OffsetY = *lpDY += 2;
 _init_set_dc ();

}  /* end _adjdown_handler */

static void _switch_flag ( GUIMenu* apMenu, int anIndex, unsigned int* apFlag, unsigned int aFlag ) {

 unsigned int  lIconIdx;
 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )apMenu -> m_pState -> m_pTail -> m_Param;

 if ( *apFlag & aFlag ) {

  *apFlag &= ~aFlag;
  lIconIdx = GUICON_OFF;

 } else {

  *apFlag |= aFlag;
  lIconIdx = GUICON_ON;

 }  /* end else */

 lpState -> m_pItems[ anIndex ].m_IconRight = lIconIdx;
 apMenu -> Redraw ( apMenu );

}  /* end _switch_flag */

extern int GUI_ReadButtons0 ( void );
extern int GUI_ReadButtons1 ( void );
extern int GUI_ReadButtons2 ( void );

extern unsigned char g_PadBuf1[ 256 ] __attribute__(   (  aligned( 64 ), section( ".data"  )  )   );

static void _cntslot_handler ( GUIMenu* apMenu, int aDir ) {

 unsigned int lStr;

 if ( g_Config.m_NetworkFlags & SMS_DF_GAMEPAD ) {

  g_Config.m_NetworkFlags &= ~SMS_DF_GAMEPAD;
  PAD_ClosePort ( 1, 0 );

  if ( g_IOPFlags & SMS_IOPF_RMMAN ) {

   RC_Open ( 1 );
   GUI_ReadButtons = GUI_ReadButtons0;
   g_Config.m_NetworkFlags |=  SMS_DF_REMOTE;
   lStr                     = ( unsigned int )&STR_REMOTE_CONTROL;

  } else {
setNone:
   if ( g_IOPFlags & SMS_IOPF_RMMAN2 ) {

    RCX_Open ();
    GUI_ReadButtons = GUI_ReadButtons0;

   } else GUI_ReadButtons = GUI_ReadButtons2;

   lStr = ( unsigned int )&STR_NONE;

  }  /* end else */

 } else if ( g_Config.m_NetworkFlags & SMS_DF_REMOTE ) {

  g_Config.m_NetworkFlags &= ~SMS_DF_REMOTE;
  goto setNone;

 } else {

  if ( g_IOPFlags & SMS_IOPF_RMMAN2 )
   RCX_Close ();
  else if ( g_IOPFlags & SMS_IOPF_RMMAN ) RC_Close ( 1 );

  PAD_OpenPort ( 1, 0, g_PadBuf1 );
  GUI_ReadButtons = GUI_ReadButtons1;

  g_Config.m_NetworkFlags |= SMS_DF_GAMEPAD;
  lStr                     = ( unsigned int )&STR_GAMEPAD;

 }  /* end else */

 s_DevMenu[ 1 ].m_IconRight = lStr;

 apMenu -> Redraw ( apMenu );

}  /* end _cntslot_handler */

static void _autonet_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 2, &g_Config.m_NetworkFlags, SMS_DF_AUTO_NET );

}  /* end _autonet_handler */

static void _autousb_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 3, &g_Config.m_NetworkFlags, SMS_DF_AUTO_USB );

}  /* end _autousb_handler */

static void _autohdd_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 4, &g_Config.m_NetworkFlags, SMS_DF_AUTO_HDD );

}  /* end _autohdd_handler */

static int _start_device (  GUIMenu* apMenu, int ( *Start ) ( int )  ) {

 int retVal;

 if (   !(  retVal = Start ( 1 )  )   ) {

  GUI_Error ( STR_ERROR.m_pStr );
  GUIMenuSMS_UpdateStatus ( apMenu );

 } else {

  GUI_MenuPopState ( apMenu );
  _device_handler ( apMenu, 0 );

 }  /* end else */

 return retVal;

}  /* end _start_device */

static void _startnet_handler ( GUIMenu* apMenu, int aDir ) {

 _start_device ( apMenu, SMS_IOPStartNet );

}  /* end _startnet_handler */

static void _startusb_handler ( GUIMenu* apMenu, int aDir ) {

 _start_device ( apMenu, SMS_IOPStartUSB );

}  /* end _startusb_handler */

static void _starthdd_handler ( GUIMenu* apMenu, int aDir ) {

 _start_device ( apMenu, SMS_IOPStartHDD );

 if ( g_IOPFlags & SMS_IOPF_HDD ) s_fHDD = 1;

}  /* end _starthdd_handler */

static char s_IP [ 4 ][ 4 ] __attribute__(   (  section( ".data" )  )   );
static char s_MSK[ 4 ][ 4 ] __attribute__(   (  section( ".data" )  )   );
static char s_GW [ 4 ][ 4 ] __attribute__(   (  section( ".data" )  )   );

static SMString s_StrIP[ 4 ] __attribute__(   (  section( ".data" )  )   ) = {
 { 0, s_IP[ 0 ] }, { 0, s_IP[ 1 ] }, { 0, s_IP[ 2 ] }, { 0, s_IP[ 3 ] }
};

static SMString s_StrMSK[ 4 ] __attribute__(   (  section( ".data" )  )   ) = {
 { 0, s_MSK[ 0 ] }, { 0, s_MSK[ 1 ] }, { 0, s_MSK[ 2 ] }, { 0, s_MSK[ 3 ] }
};

static SMString s_StrGW[ 4 ] __attribute__(   (  section( ".data" )  )   ) = {
 { 0, s_GW[ 0 ] }, { 0, s_GW[ 1 ] }, { 0, s_GW[ 2 ] }, { 0, s_GW[ 3 ] }
};

static void _update_ips ( void ) {

 int i;

 for ( i = 0; i < 4; ++i ) s_StrIP [ i ].m_Len = strlen ( s_IP [ i ] );
 for ( i = 0; i < 4; ++i ) s_StrMSK[ i ].m_Len = strlen ( s_MSK[ i ] );
 for ( i = 0; i < 4; ++i ) s_StrGW [ i ].m_Len = strlen ( s_GW [ i ] );

}  /* end _update_ips */

static void _set_ips ( char apDst[][ 4 ], char* apSrc ) {

 int i, j;

 for ( i = 0; i < 4; ++i ) {

  j = 0;

  while ( *apSrc && *apSrc != '.' ) apDst[ i ][ j++ ] = *apSrc++;

  ++apSrc;

 }  /* end for */

}  /* end _set_ips */

static void _editipc_handler ( GUIMenu* apMenu, int aDir ) {

 int           i = 0, j;
 GUIMenuState* lpState = GUI_MenuPushState ( apMenu );

 _set_ips ( s_IP,  g_pDefIP   );
 _set_ips ( s_MSK, g_pDefMask );
 _set_ips ( s_GW,  g_pDefGW   );

 _update_ips ();

 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_IPCMenu;
 lpState -> m_pLast  = &s_IPCMenu[ sizeof ( s_IPCMenu ) / sizeof ( s_IPCMenu[ 0 ] ) - 1 ];
 lpState -> m_pTitle = &STR_EDIT_IPCONFIG1;

 for ( j = 0; j < 4; ++j, ++i ) s_IPCMenu[ i ].m_IconRight = ( unsigned int )&s_StrIP [ j ];
 for ( j = 0; j < 4; ++j, ++i ) s_IPCMenu[ i ].m_IconRight = ( unsigned int )&s_StrMSK[ j ];
 for ( j = 0; j < 4; ++j, ++i ) s_IPCMenu[ i ].m_IconRight = ( unsigned int )&s_StrGW [ j ];

 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _editipc_handler */

static void _cdvd_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 5, &g_Config.m_NetworkFlags, SMS_DF_CDVD );

}  /* end _cdvd_handler */

static void _cdvd_spd_handler ( GUIMenu* apMenu, int aDir ) {

 int lSpeed = g_Config.m_CDVDSpeed + aDir;

 if ( lSpeed < 0 )
  lSpeed = 2;
 else if ( lSpeed > 2 ) lSpeed = 0;

 s_DevMenu[ 6 ].m_IconRight = ( unsigned int )s_Speeds[ g_Config.m_CDVDSpeed = lSpeed ];
 apMenu -> Redraw ( apMenu );
 CDVD_SetSpeed ();

}  /* end _cdvd_spd_handler */

static void _dirbtn_handler ( GUIMenu* apMenu, int aDir ) {

 g_Config.m_BrowserFlags ^= SMS_BF_DIRB;
 s_DevMenu[ 7 ].m_IconRight = ( unsigned int )( g_Config.m_BrowserFlags & SMS_BF_DIRB ? &STR_REMOTE_CONTROL : &STR_GAMEPAD );
 apMenu -> Redraw ( apMenu );

 SMS_SetDirButtons ();

}  /* end _dirbtn_handler */

static void _roll_ip_number ( GUIMenu* apMenu, char* apStr, int aDir ) {

 int lNum = atoi ( apStr );

 lNum += aDir;

 if ( lNum < 0 )
  lNum = 255;
 else if ( lNum > 255 ) lNum = 0;

 sprintf ( apStr, "%d", lNum );

 _update_ips ();
 apMenu -> Redraw ( apMenu );

}  /* end _roll_ip_number */

static int s_Masks[ 9 ] = {
 0, 128, 192, 224, 240, 248, 252, 254, 255
};

static void _roll_mask_number ( GUIMenu* apMenu, char* apStr, int aDir ) {

 int i;
 int lNum = atoi ( apStr );

 for ( i = 0; i < 9; ++i ) if ( lNum == s_Masks[ i ] ) break;

 if ( i == 9 ) i = 0;

 i += aDir;

 if ( i < 0 )
  i = 8;
 else if ( i > 8 ) i = 0;

 sprintf ( apStr, "%d", s_Masks[ i ] );

 _update_ips ();
 apMenu -> Redraw ( apMenu );

}  /* end _roll_mask_number */

static void _ip1_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_ip_number ( apMenu, s_IP[ 0 ], aDir );

}  /* end _ip1_handler */

static void _ip2_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_ip_number ( apMenu, s_IP[ 1 ], aDir );

}  /* end _ip2_handler */

static void _ip3_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_ip_number ( apMenu, s_IP[ 2 ], aDir );

}  /* end _ip3_handler */

static void _ip4_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_ip_number ( apMenu, s_IP[ 3 ], aDir );

}  /* end _ip4_handler */

static void _nm1_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_mask_number ( apMenu, s_MSK[ 0 ], aDir );

}  /* end _nm1_handler */

static void _nm2_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_mask_number ( apMenu, s_MSK[ 1 ], aDir );

}  /* end _nm2_handler */

static void _nm3_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_mask_number ( apMenu, s_MSK[ 2 ], aDir );

}  /* end _nm3_handler */

static void _nm4_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_mask_number ( apMenu, s_MSK[ 3 ], aDir );

}  /* end _nm4_handler */

static void _gw1_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_ip_number ( apMenu, s_GW[ 0 ], aDir );

}  /* end _gw1_handler */

static void _gw2_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_ip_number ( apMenu, s_GW[ 1 ], aDir );

}  /* end _gw2_handler */

static void _gw3_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_ip_number ( apMenu, s_GW[ 2 ], aDir );

}  /* end _gw3_handler */

static void _gw4_handler ( GUIMenu* apMenu, int aDir ) {

 _roll_ip_number ( apMenu, s_GW[ 3 ], aDir );

}  /* end _gw4_handler */

static void _saveipc_handler ( GUIMenu* apMenu, int aDir ) {

 int  i, j;
 int  lRes;
 char lBuf[ 64 ];
 char lDir[ 14 ];
 int  lSts = 0;

 GUI_Status ( STR_SAVING_IPCONFIG.m_pStr );

 lBuf[ 0 ] = '\x00';

 for ( i = 0; i < 4; ++i ) {

  strcat ( lBuf, s_IP[ i ] );
  strcat ( lBuf, g_DotStr  );

 }  /* end for */

 j = strlen ( lBuf ) - 1;
 lBuf[ j   ] = '\x00';
 strcpy ( g_pDefIP, lBuf );
 lBuf[ j++ ] = '\n';

 for ( i = 0; i < 4; ++i ) {

  strcat ( lBuf, s_MSK[ i ] );
  strcat ( lBuf, g_DotStr  );

 }  /* end for */

 i = strlen ( lBuf ) - 1;

 lBuf[ i ] = '\x00';
 strcpy ( g_pDefMask, &lBuf[ j ] );
 lBuf[ i ] = '\n';
 j = i + 1;

 for ( i = 0; i < 4; ++i ) {

  strcat ( lBuf, s_GW[ i ] );
  strcat ( lBuf, g_DotStr  );

 }  /* end for */

 i = strlen ( lBuf ) - 1;

 lBuf[ i ] = '\x00';
 strcpy ( g_pDefGW, &lBuf[ j ] );
 lBuf[ i ] = '\n';

 strncpy ( lDir, g_pIPConf, 13 );
 lDir[ 13 ] = '\x00';

 MC_GetInfo ( g_MCSlot, 0, &lRes, &lRes, &lRes );
 MC_Sync ( &lRes );

 if ( lRes > -2 ) {

  SMS_MCTable lTbl __attribute__(   (  aligned( 64 )  )   );

  MC_GetDir ( g_MCSlot, 0, lDir, 0, 1, &lTbl );
  MC_Sync ( &lRes );

  if ( lRes ) {

   lRes = fioMkdir ( lDir );

   if ( lRes == 0 || lRes == -4 ) {

    int lFD = fioOpen ( g_pIPConf, O_CREAT | O_WRONLY );

    if ( lFD >= 0 ) {

     i = strlen ( lBuf );

     lSts = fioWrite ( lFD, lBuf, i ) == i;
     fioClose ( lFD );

    }  /* end if */

   }  /* end if */

  }  /* end if */

 }  /* end if */

 if ( !lSts ) GUI_Error ( STR_ERROR.m_pStr );

 GUIMenuSMS_UpdateStatus ( apMenu );

}  /* end _saveipc_handler */

static void _usebg_handler ( GUIMenu* apMenu, int aDir ) {

 SMS_List*     lpList;
 SMS_ListNode* lpNode = SMS_ListFind (
  lpList = g_Config.m_pSkinList, g_Config.m_SkinName
 );

 if ( !lpNode ) lpNode = lpList -> m_pHead;

 if ( aDir > 0 ) {
  lpNode = lpNode -> m_pNext;
  if ( !lpNode ) lpNode = lpList -> m_pHead;
 } else {
  lpNode = lpNode -> m_pPrev;
  if ( !lpNode ) lpNode = lpList -> m_pTail;
 }  /* end else */

 strcpy (  g_Config.m_SkinName, _STR( lpNode )  );

 _update_skin ();

 GUI_Initialize ( 0 );
 GUIMenuSMS_UpdateStatus ( apMenu );

}  /* end _usebg_handler */

static void _sound_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 1, &g_Config.m_BrowserFlags, SMS_BF_SDFX );

}  /* end _sound_handler */

static void _sortfs_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 2, &g_Config.m_BrowserFlags, SMS_BF_SORT );

}  /* end _sortfs_handler */

static void _filter_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 3, &g_Config.m_BrowserFlags, SMS_BF_AVIF );

 g_pFileMenu -> HandleEvent ( g_pFileMenu, GUI_MSG_REFILL_BROWSER );

 GUI_Redraw ( GUIRedrawMethod_InitClearObj );
 GUIMenuSMS_UpdateStatus ( apMenu );

}  /* end _filter_handler */

static void _apply_hdd_filter ( GUIMenu* apMenu ) {

 if ( g_CMedia == 2 && g_CWD[ 0 ] == 'h' &&
                       g_CWD[ 1 ] == 'd' &&
                       g_CWD[ 2 ] == 'd' &&
                       g_CWD[ 3 ] == '0'
 ) {

  g_pFileMenu -> HandleEvent ( g_pFileMenu, GUI_MSG_REFILL_BROWSER );

  GUI_Redraw ( GUIRedrawMethod_InitClearObj );
  GUIMenuSMS_UpdateStatus ( apMenu );

 }  /* end if */

}  /* end _apply_hdd_filter */

static void _dsphdl_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 4, &g_Config.m_BrowserFlags, SMS_BF_HDLP );
 _apply_hdd_filter ( apMenu );

}  /* end _dsphdl_handler */

static void _hidesp_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 5, &g_Config.m_BrowserFlags, SMS_BF_SYSP );
 _apply_hdd_filter ( apMenu );

} /* end _hidesp_handler */

static void _usexh_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 6, &g_Config.m_BrowserFlags, SMS_BF_UXH );

}  /* end _usexh_handler */

static void _rotate_palette ( GUIMenu* apMenu, unsigned int* apVal, int aDir ) {

 int lVal = *apVal;

 lVal += aDir;

 if ( lVal == 17 )
  lVal = 1;
 else if ( lVal < 0 ) lVal = 16;

 *apVal = lVal;

 GUI_Redraw ( GUIRedrawMethod_InitClearObj );
 GUIMenuSMS_UpdateStatus ( apMenu );

}  /* end _rotate_palette */

static void _abclr_handler ( GUIMenu* apMenu, int aDir ) {

 _rotate_palette ( apMenu, &g_Config.m_BrowserABCIdx, aDir );

}  /* end _abclr_handler */

static void _ibclr_handler ( GUIMenu* apMenu, int aDir ) {

 _rotate_palette ( apMenu, &g_Config.m_BrowserIBCIdx, aDir );

}  /* end _ibclr_handler */

static void _txtclr_handler ( GUIMenu* apMenu, int aDir ) {

 int           lVal = g_Config.m_BrowserTxtIdx;
 u64           lColor;

 lVal += aDir;

 if ( lVal == 17 )
  lVal = 1;
 else if ( lVal < 0 ) lVal = 16;

 g_Config.m_BrowserTxtIdx = lVal;
 lColor                   = g_Palette[ g_Config.m_BrowserTxtIdx - 1 ];

 GSContext_SetTextColor (  1,   lColor                              );
 GSContext_SetTextColor (  2, ( lColor & 0x00FFFFFF ) | 0x20000000  );

 GUI_Redraw ( GUIRedrawMethod_InitClearObj );
 GUIMenuSMS_UpdateStatus ( apMenu );

}  /* end _txtclr_handler */

static void _sltxt_handler ( GUIMenu* apMenu, int aDir ) {

 int lVal = g_Config.m_BrowserSBCIdx;

 lVal += aDir;

 if ( lVal == 17 )
  lVal = 1;
 else if ( lVal < 0 ) lVal = 16;

 g_Config.m_BrowserSBCIdx = lVal;

 GSContext_SetTextColor ( 0, g_Palette[ g_Config.m_BrowserSBCIdx - 1 ] );

 GUI_Redraw ( GUIRedrawMethod_InitClearObj );
 GUIMenuSMS_UpdateStatus ( apMenu );

}  /* end _sltxt_handler */

static void _sclr_handler ( GUIMenu* apMenu, int aDir ) {

 _rotate_palette ( apMenu, &g_Config.m_BrowserSCIdx, aDir );

}  /* end _sclr_handler */

static void _exit_2_handler ( GUIMenu* apMenu, int aDir ) {
#ifndef EMBEDDED
 int lIndex = g_Config.m_BrowserFlags >> 28;

 lIndex += aDir;

 if ( lIndex < 0 )

  lIndex = 2;

 else if ( lIndex > 2 ) lIndex = 0;

 s_BrowserMenu[ 13 ].m_IconRight = ( unsigned int )s_ExitTo[ lIndex ];

 g_Config.m_BrowserFlags &= 0x0FFFFFFF;
 g_Config.m_BrowserFlags |= lIndex << 28;

 apMenu -> Redraw ( apMenu );
#endif  /* EMBEDDED */
}  /* end _exit_2_handler */

static void _pwrbtn_handler ( GUIMenu* apMenu, int aDir ) {
#ifndef EMBEDDED
 unsigned int lFlags = g_Config.m_BrowserFlags ^ SMS_BF_EXIT;

 s_BrowserMenu[ 12 ].m_IconRight = ( unsigned int )(  ( lFlags & SMS_BF_EXIT ) ? &STR_EXIT : &STR_POWER_OFF  );

 g_Config.m_BrowserFlags = lFlags;

 apMenu -> Redraw ( apMenu );
#endif  /* EMBEDDED */
}  /* end _exit_2_handler */

static void _autols_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 1, &g_Config.m_PlayerFlags, SMS_PF_SUBS );

}  /* end _autols_handler */

static void _opaqs_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 2, &g_Config.m_PlayerFlags, SMS_PF_OSUB );

}  /* end _opaqs_handler */

static void _dsbt_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 3, &g_Config.m_PlayerFlags, SMS_PF_TIME );

}  /* end _dsbt_handler */

static void _spdif_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 4, &g_Config.m_PlayerFlags, SMS_PF_SPDIF );

}  /* end _spdif_handler */

static void _pdw22_handler ( GUIMenu* apMenu, int aDir ) {

 unsigned int  lIconIdx;
 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )apMenu -> m_pState -> m_pTail -> m_Param;
 unsigned int* lpFlag  = &g_Config.m_PlayerFlags;
 int           lfShow;

 if ( *lpFlag & SMS_PF_PDW22 ) {
  *lpFlag &= ~SMS_PF_PDW22;
  lIconIdx = GUICON_OFF;
  lfShow   = 1;
 } else {
  *lpFlag |= SMS_PF_PDW22;
  lIconIdx = GUICON_ON;
  lfShow   = 0;
 }  /* end else */

 lpState -> m_pItems[ 5 ].m_IconRight = lIconIdx;

 GUIMenu_ShowItem ( apMenu, 4, lfShow );

 apMenu -> Redraw ( apMenu );

}  /* end _pdw22_handler */

static void _rotate_palette_2 ( GUIMenu* apMenu, int* apVal, int aDir ) {

 int lVal = *apVal;

 lVal += aDir;

 if ( lVal == 17 )
  lVal = 1;
 else if ( lVal < 0 ) lVal = 16;

 *apVal = lVal;

 apMenu -> Redraw ( apMenu );

}  /* end _rotate_palette_2 */

void _subclr_handler ( GUIMenu* apMenu, int aDir ) {

 _rotate_palette_2 ( apMenu, ( int * )( &g_Config.m_PlayerSCNIdx ), aDir );

}  /* end _subclr_handler */

void _subbclr_handler ( GUIMenu* apMenu, int aDir ) {

 _rotate_palette_2 ( apMenu, ( int * )( &g_Config.m_PlayerSCBIdx ), aDir );

}  /* end _subbclr_handler */

void _subiclr_handler ( GUIMenu* apMenu, int aDir ) {

 _rotate_palette_2 ( apMenu, ( int * )( &g_Config.m_PlayerSCIIdx ), aDir );

}  /* end _subiclr_handler */

void _subu_handler ( GUIMenu* apMenu, int aDir ) {

 _rotate_palette_2 ( apMenu, ( int * )( &g_Config.m_PlayerSCUIdx ), aDir );

}  /* end _subu_handler */

static void _sbclr_handler ( GUIMenu* apMenu, int aDir ) {

 _rotate_palette_2 ( apMenu, ( int * )( &g_Config.m_PlayerSBCIdx ), aDir );

}  /* end _sbclr_handler */

static void _vbclr_handler ( GUIMenu* apMenu, int aDir ) {

 _rotate_palette_2 ( apMenu, ( int * )( &g_Config.m_PlayerVBCIdx ), aDir );

}  /* end _vbclr_handler */

static void _clres_handler ( GUIMenu* apMenu, int aDir ) {

 if (   !(  g_Config.m_PlayerFlags & ( SMS_PF_C32 | SMS_PF_C16 )  )   )

  g_Config.m_PlayerFlags |= SMS_PF_C32;

 else if ( g_Config.m_PlayerFlags & SMS_PF_C32 ) {

  g_Config.m_PlayerFlags &= ~SMS_PF_C32;
  g_Config.m_PlayerFlags |=  SMS_PF_C16;

 } else g_Config.m_PlayerFlags &= ~( SMS_PF_C32 | SMS_PF_C16 );

 _update_pstr ( 0 );
 apMenu -> Redraw ( apMenu );

}  /* end _clres_handler */

static void _vol_handler ( GUIMenu* apMenu, int aDir ) {

 if ( aDir > 0 ) {

  if ( ++g_Config.m_PlayerVolume == 25 ) g_Config.m_PlayerVolume = 0;

 } else if ( g_Config.m_PlayerVolume ) --g_Config.m_PlayerVolume;

 _update_pstr ( 0 );
 apMenu -> Redraw ( apMenu );

}  /* end _vol_handler */

static void _salign_handler ( GUIMenu* apMenu, int aDir ) {

 g_Config.m_PlayerSAlign = (  ( g_Config.m_PlayerSAlign + 1 ) % 4  );

 _update_pstr ( 0 );
 apMenu -> Redraw ( apMenu );

}  /* end _salign_handler */

static void _suboff_handler ( GUIMenu* apMenu, int aDir ) {

 int lBottom = 32 + g_Config.m_SubVIncr;

 if ( aDir > 0 ) {

  if ( ++g_Config.m_PlayerSubOffset == 145 ) g_Config.m_PlayerSubOffset = lBottom;

 } else if ( g_Config.m_PlayerSubOffset > lBottom )

  --g_Config.m_PlayerSubOffset;

 else g_Config.m_PlayerSubOffset = 144;

 _update_pstr ( 0 );
 s_pSample -> Cleanup ( s_pSample );
 apMenu -> Redraw ( apMenu );
 _redraw_sample ();

}  /* end _suboff_handler */

static void _poff_handler ( GUIMenu* apMenu, int aDir ) {

 aDir *= 60000;

 _update_pstr ( aDir );
 apMenu -> Redraw ( apMenu );

}  /* end _poff_handler */

static void _sblen_handler ( GUIMenu* apMenu, int aDir ) {

 if ( aDir < 0 ) {

  g_Config.m_ScrollBarNum -= 16;
 
  if ( g_Config.m_ScrollBarNum == 16 ) g_Config.m_ScrollBarNum = 128;

 } else {

  g_Config.m_ScrollBarNum += 16;
 
  if ( g_Config.m_ScrollBarNum == 144 ) g_Config.m_ScrollBarNum = 32;

 }  /* end else */

 _update_pstr ( 0 );
 apMenu -> Redraw ( apMenu );

}  /* end _sblen_handler */

static void _sbpos_handler ( GUIMenu* apMenu, int aDir ) {

 switch ( g_Config.m_ScrollBarPos ) {

  default                     :
  case SMScrollBarPos_Top     : g_Config.m_ScrollBarPos = SMScrollBarPos_Bottom;   break;
  case SMScrollBarPos_Bottom  : g_Config.m_ScrollBarPos = SMScrollBarPos_Inactive; break;
  case SMScrollBarPos_Inactive: g_Config.m_ScrollBarPos = SMScrollBarPos_Top;      break;

 }  /* end switch */

 _update_pstr ( 0 );
 apMenu -> Redraw ( apMenu );

}  /* end _sbpos_handler */

static void _sfonth_handler ( GUIMenu* apMenu, int aDir ) {

 if ( aDir > 0 && g_Config.m_SubHIncr < 48 )

  ++g_Config.m_SubHIncr;

 else if ( aDir < 0 && g_Config.m_SubHIncr > -16 ) --g_Config.m_SubHIncr;

 _update_pstr ( 0 );
 s_pSample -> Cleanup ( s_pSample );
 apMenu -> Redraw ( apMenu );
 _redraw_sample ();

}  /* end _sfonth_handler */

static void _sfontv_handler ( GUIMenu* apMenu, int aDir ) {

 if ( aDir > 0 && g_Config.m_SubVIncr < 48 ) {

  int lDelta = 32 + g_Config.m_SubVIncr;

  ++g_Config.m_SubVIncr;

  if ( lDelta > g_Config.m_PlayerSubOffset ) g_Config.m_PlayerSubOffset = lDelta;

 } else if ( aDir < 0 && g_Config.m_SubVIncr > -16 ) --g_Config.m_SubVIncr;

 _update_pstr ( 0 );
 s_pSample -> Cleanup ( s_pSample );
 apMenu -> Redraw ( apMenu );
 _redraw_sample ();

}  /* end _sfontv_handler */

static void _enter_sample ( GUIMenu* apMenu ) {

 void* lpActiveNode = g_pActiveNode;

 GUI_AddObject (  STR_SAMPLE.m_pStr, s_pSample = _create_sample ()  );
 g_pActiveNode = lpActiveNode;
 apMenu -> Redraw ( apMenu );
 _redraw_sample ();

}  /* end _enter_sample */

static void _leave_sample ( GUIMenu* apMenu ) {

 void* lpActiveNode = g_pActiveNode;

 GUI_DeleteObject ( STR_SAMPLE.m_pStr );
 g_pActiveNode = lpActiveNode;
 apMenu -> Redraw ( apMenu );

}  /* end _leave_sample */

static void _smbcs_handler ( GUIMenu* apMenu, int aDir ) {

 SMS_List*     lpList;
 SMS_ListNode* lpNode = SMS_ListFind (
  lpList = g_Config.m_pMBFList, g_Config.m_MBFName
 );

 if ( !lpNode ) lpNode = lpList -> m_pHead;

 if ( aDir > 0 ) {
  lpNode = lpNode -> m_pNext;
  if ( !lpNode ) lpNode = lpList -> m_pHead;
 } else {
  lpNode = lpNode -> m_pPrev;
  if ( !lpNode ) lpNode = lpList -> m_pTail;
 }  /* end else */

 strcpy (  g_Config.m_MBFName, _STR( lpNode )  );

 _update_mbf ();

 apMenu -> Redraw ( apMenu );

}  /* end _smbcs_handler */

static char s_DispHBuff[ 5 ] __attribute__(   (  section( ".data" ), aligned( 1 )  )   );
static char s_DispWBuff[ 5 ] __attribute__(   (  section( ".data" ), aligned( 1 )  )   );
static char s_SynP1Buff[ 5 ] __attribute__(   (  section( ".data" ), aligned( 1 )  )   );
static char s_SynP2Buff[ 5 ] __attribute__(   (  section( ".data" ), aligned( 1 )  )   );
static char s_SynP3Buff[ 5 ] __attribute__(   (  section( ".data" ), aligned( 1 )  )   );

static SMString  s_StrAdvH __attribute__(   (  section( ".data" )  )   ) = { 0, s_DispHBuff };
static SMString  s_StrAdvW __attribute__(   (  section( ".data" )  )   ) = { 0, s_DispWBuff };
static SMString  s_StrSyP1 __attribute__(   (  section( ".data" )  )   ) = { 0, s_SynP1Buff };
static SMString  s_StrSyP2 __attribute__(   (  section( ".data" )  )   ) = { 0, s_SynP2Buff };
static SMString  s_StrSyP3 __attribute__(   (  section( ".data" )  )   ) = { 0, s_SynP3Buff };
static SMString* s_ClrRes[ 2 ] __attribute__(   (  section( ".data" )  )   ) = {
 &STR_32_BIT, &STR_16_BIT
};

static void _update_advs ( void ) {

 int   lIdx = GS_VMode2Index (  GS_Params () -> m_GSCRTMode  );
 int   lH, lW;
 short lSP1, lSP2, lSP3;
 
 lW   = g_Config.m_DispWH[ lIdx ][ 0 ];
 lH   = g_Config.m_DispWH[ lIdx ][ 1 ];
 lSP1 = g_Config.m_SyncPar[ lIdx ][ 0 ];
 lSP2 = g_Config.m_SyncPar[ lIdx ][ 1 ];
 lSP3 = g_Config.m_SyncPar[ lIdx ][ 2 ];
 
 sprintf ( s_DispHBuff, "%d", lH   );
 sprintf ( s_DispWBuff, "%d", lW   );
 sprintf ( s_SynP1Buff, "%d", lSP1 );
 sprintf ( s_SynP2Buff, "%d", lSP2 );
 sprintf ( s_SynP3Buff, "%d", lSP3 );

 s_StrAdvH.m_Len = strlen ( s_DispHBuff );
 s_StrAdvW.m_Len = strlen ( s_DispWBuff );
 s_StrSyP2.m_Len = strlen ( s_SynP2Buff );
 s_StrSyP3.m_Len = strlen ( s_SynP3Buff );

 if ( lSP1 ) {
  s_StrSyP1.m_Len  = strlen ( s_SynP1Buff );
  s_StrSyP1.m_pStr = s_SynP1Buff;
 } else {
  s_StrSyP1.m_Len  = STR_AUTO.m_Len;
  s_StrSyP1.m_pStr = STR_AUTO.m_pStr;
 }  /* end else */

}  /* end _update_advs */

static void _advset_handler ( GUIMenu* apMenu, int aDir ) {

 GUIMenuState* lpState = GUI_MenuPushState ( apMenu );

 _update_advs ();

 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_AdvDispMenu;
 lpState -> m_pLast  = &s_AdvDispMenu[ sizeof ( s_AdvDispMenu ) / sizeof ( s_AdvDispMenu[ 0 ] ) - 1 ];
 lpState -> m_pTitle = &STR_ADVANCED_SETTINGS1;

 s_AdvDispMenu[ 0 ].m_IconRight = ( unsigned int )&s_StrAdvH;
 s_AdvDispMenu[ 1 ].m_IconRight = ( unsigned int )&s_StrAdvW;
 s_AdvDispMenu[ 2 ].m_IconRight = ( unsigned int )s_ClrRes[ g_Config.m_ColorDepth ];
 s_AdvDispMenu[ 3 ].m_IconRight = ( unsigned int )&s_StrSyP1;
 s_AdvDispMenu[ 4 ].m_IconRight = ( unsigned int )&s_StrSyP2;
 s_AdvDispMenu[ 5 ].m_IconRight = ( unsigned int )&s_StrSyP3;

 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _advset_handler */

static void _dispw_handler ( GUIMenu* apMenu, int aDir ) {

 static int s_lMin[ 8 ] __attribute__(   (  section( ".data" )  )   ) = { 640, 640, 640, 640, 1090, 1400, 640, 640 };
 static int s_lMax[ 8 ] __attribute__(   (  section( ".data" )  )   ) = { 720, 720, 720, 720, 1190, 1830, 720, 720 };

 int lIdx = GS_VMode2Index (  GS_Params () -> m_GSCRTMode  );
 int lWidth;

 lWidth = g_Config.m_DispWH[ lIdx ][ 0 ] + aDir;

 if ( lWidth > s_lMax[ lIdx ] )
  lWidth = s_lMax[ lIdx ];
 else if ( lWidth < s_lMin[ lIdx ] )
  lWidth = s_lMin[ lIdx ];

 g_Config.m_DispWH[ lIdx ][ 0 ] = lWidth;

 _update_advs ();

 apMenu -> Redraw ( apMenu );

}  /* end _dispw_handler */

static void _disph_handler ( GUIMenu* apMenu, int aDir ) {

 static int s_lMin[ 8 ] __attribute__(   (  section( ".data" )  )   ) = { 416, 448, 480, 512, 630,  790, 480, 480 };
 static int s_lMax[ 8 ] __attribute__(   (  section( ".data" )  )   ) = { 480, 576, 576, 592, 830, 1024, 576, 576 };

 int lIdx = GS_VMode2Index (  GS_Params () -> m_GSCRTMode   );
 int lHeight;

 lHeight = g_Config.m_DispWH[ lIdx ][ 1 ] + aDir;

 if ( lHeight > s_lMax[ lIdx ] )
  lHeight = s_lMax[ lIdx ];
 else if ( lHeight < s_lMin[ lIdx ] )
  lHeight = s_lMin[ lIdx ];

 g_Config.m_DispWH[ lIdx ][ 1 ] = lHeight;
 g_Config.m_PAR[ lIdx ]         = ( float )lHeight / 480.0F;

 _update_advs ();

 apMenu -> Redraw ( apMenu );

}  /* end _disph_handler */

static void _dispc_handler ( GUIMenu* apMenu, int aDir ) {

 g_Config.m_ColorDepth = ( g_Config.m_ColorDepth + aDir ) & 1;

 s_AdvDispMenu[ 2 ].m_IconRight = ( unsigned int )s_ClrRes[ g_Config.m_ColorDepth ];

 apMenu -> Redraw ( apMenu );

}  /* end _dispc_handler */

short g_MaxSyncVal[ 8 ] __attribute__(   (  section( ".data" )  )   ) = {
 263, 312, 525, 576, 750, 563, 525, 500
};

static void SMS_INLINE _verify_sp ( int anIdx, short* apVal ) {

 if ( apVal[ 0 ] < 0 )
  apVal[ 0 ] = g_MaxSyncVal[ anIdx ];
 else if ( apVal[ 0 ] > g_MaxSyncVal[ anIdx ] )
  apVal[ 0 ] = 0;

}  /* end _verify_sp */

static void _update_sp ( GUIMenu* apMenu, int aDir, int anIdx ) {

 short* lpVal;
 short  lVal;
 int    lIdx = GS_VMode2Index (  GS_Params () -> m_GSCRTMode  );

 lpVal  = &g_Config.m_SyncPar[ lIdx ][ anIdx ];
 lVal   = *lpVal;
 lVal  += aDir;
 *lpVal = lVal;

 _verify_sp ( lIdx, lpVal );
 _update_advs ();

 apMenu -> Redraw ( apMenu );

}  /* end _update_sp */

static void _disps1_handler ( GUIMenu* apMenu, int aDir ) {

 _update_sp ( apMenu, aDir, 0 );

}  /* end _disps1_handler */

static void _disps2_handler ( GUIMenu* apMenu, int aDir ) {

 _update_sp ( apMenu, aDir, 1 );

}  /* end _disps2_handler */

static void _disps3_handler ( GUIMenu* apMenu, int aDir ) {

 _update_sp ( apMenu, aDir, 2 );

}  /* end _disps2_handler */

static void _apply_handler ( GUIMenu* apMenu, int aDir ) {

 GUI_MenuPopState ( apMenu );

 _reinitialize ( apMenu );

}  /* end _apply_handler */

static char     s_MP3DPS[ 3 ] __attribute__(   (  section( ".data" ), aligned( 1 )  )   );
static SMString s_StrMP3DPS = { 0, s_MP3DPS };

static void _mp3_update_dps ( void ) {

 sprintf ( s_MP3DPS, g_pPercDStr, g_Config.m_MP3AutoPar );
 s_StrMP3DPS.m_Len = strlen ( s_MP3DPS );

}  /* end _mp3_update_dps */

static void _mp3_handler ( GUIMenu* apMenu, int aDir ) {

 GUIMenuState* lpState = GUI_MenuPushState ( apMenu );

 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_MP3Menu;
 lpState -> m_pLast  = &s_MP3Menu[ sizeof ( s_MP3Menu ) / sizeof ( s_MP3Menu[ 0 ] ) - 1 ];
 lpState -> m_pTitle = &STR_MP3_SETTINGS;

 _mp3_update_dps ();

 s_MP3Menu[ 0 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_ANIM  ? GUICON_ON : GUICON_OFF;
 s_MP3Menu[ 1 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_RAND  ? GUICON_ON : GUICON_OFF;
 s_MP3Menu[ 2 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_REP   ? GUICON_ON : GUICON_OFF;
 s_MP3Menu[ 3 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_ASD   ? GUICON_ON : GUICON_OFF;
 s_MP3Menu[ 4 ].m_IconRight = ( unsigned int )&s_StrMP3DPS;

 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _mp3_handler */

static void _aadsp_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 0, &g_Config.m_PlayerFlags, SMS_PF_ANIM );

}  /* end _aadsp_handler */

static void _mp3_rand_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 1, &g_Config.m_PlayerFlags, SMS_PF_RAND );

}  /* end _mp3_rand_handler */

static void _mp3_repeat_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 2, &g_Config.m_PlayerFlags, SMS_PF_REP );

}  /* end _mp3_repeat_handler */

static void _mp3_asd_handler ( GUIMenu* apMenu, int aDir ) {

 _switch_flag ( apMenu, 3, &g_Config.m_PlayerFlags, SMS_PF_ASD );

}  /* end _mp3_asd_handler */

static void _mp3_adp_handler ( GUIMenu* apMenu, int aDir ) {

 g_Config.m_MP3AutoPar = ( char )g_Config.m_MP3AutoPar + aDir;

 if (  ( char )g_Config.m_MP3AutoPar > 99  )
  g_Config.m_MP3AutoPar = 1;
 else if (  ( char )g_Config.m_MP3AutoPar < 1  ) g_Config.m_MP3AutoPar = 99;

 _mp3_update_dps ();

 apMenu -> Redraw ( apMenu );

}  /* end _mp3_adp_handler */

static void _build_net_menu ( GUIMenu* );

static void _netprot_handler ( GUIMenu* apMenu, int aDir ) {

 if ( g_IOPFlags & SMS_IOPF_SMBINFO ) {

  SMS_ListNode* lpNode = apMenu -> m_pState -> m_pTail;
  GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )lpNode -> m_Param;

  if ( g_Config.m_NetworkFlags & SMS_DF_SMB )
   g_Config.m_NetworkFlags &= ~SMS_DF_SMB;
  else g_Config.m_NetworkFlags |= SMS_DF_SMB;

  lpState -> m_pCurr -> m_IconRight = ( unsigned int )(  ( g_Config.m_NetworkFlags & SMS_DF_SMB ) ? &STR_SMB_CIFS : &STR_PS2DEV_HOST );

  _build_net_menu ( apMenu );
  apMenu -> Redraw ( apMenu );

 }  /* end if */

}  /* end _netprot_handler */

static void _update_opmode ( GUIMenu* apMenu ) {

 SMString*     lpStr;
 int           lMode   = ( g_Config.m_NetworkFlags >> 8 ) & 3;
 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )apMenu -> m_pState -> m_pTail -> m_Param;

 switch ( lMode ) {

  case 0 : lpStr = &STR_AUTONEGOTIATION; break;
  case 1 : lpStr = &STR_AUTOMATIC;       break;
  default: lpStr = &STR_MANUAL;          break;

 }  /* end switch */

 lpState -> m_pLast = lMode == 2 ? &s_NetMenu[ lpState -> m_Count - 1 ]
                                 : &s_NetMenu[ lpState -> m_Count - 3 ];

 s_NetMenu[ lpState -> m_Count - 3 ].m_IconRight = ( unsigned int )lpStr;

}  /* end _update_opmode */

static void _update_dup_mode ( GUIMenu* apMenu ) {

 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )apMenu -> m_pState -> m_pTail -> m_Param;

 s_NetMenu[ lpState -> m_Count - 2 ].m_IconRight = ( unsigned int )( g_Config.m_NetworkFlags & SMS_DF_HALF ? &STR_HALF : &STR_FULL );

}  /* end _update_dup_mode */

static void _update_standard ( GUIMenu* apMenu ) {

 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )apMenu -> m_pState -> m_pTail -> m_Param;

 s_NetMenu[ lpState -> m_Count - 1 ].m_IconRight = ( unsigned int )( g_Config.m_NetworkFlags & SMS_DF_10 ? &STR_10_BASE_T : &STR_100_BASE_TX );

}  /* end _update_standard */

static void _build_net_menu ( GUIMenu* apMenu ) {

 int           lIdx    = 2;
 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )apMenu -> m_pState -> m_pTail -> m_Param;

 lpState -> m_Count = 5;

 if ( g_Config.m_NetworkFlags & SMS_DF_SMB ) {
  s_NetMenu[ 2 ].m_Type        = 0;
  s_NetMenu[ 2 ].m_pOptionName = &STR_SMB_SERVER;
  s_NetMenu[ 2 ].m_IconLeft    = 0;
  s_NetMenu[ 2 ].m_IconRight   = 0;
  s_NetMenu[ 2 ].Handler       = _smbsrv_handler;
  s_NetMenu[ 2 ].Enter         = NULL;
  s_NetMenu[ 2 ].Leave         = NULL;
  lpState -> m_Count = 6;
  ++lIdx;
 }  /* end if */

 s_NetMenu[ lIdx   ].m_Type        = MENU_ITEM_TYPE_TEXT;
 s_NetMenu[ lIdx   ].m_pOptionName = &STR_OPERATING_MODE;
 s_NetMenu[ lIdx   ].m_IconLeft    = 0;
 s_NetMenu[ lIdx   ].m_IconRight   = 0;
 s_NetMenu[ lIdx   ].Handler       = _opmode_handler;
 s_NetMenu[ lIdx   ].Enter         = NULL;
 s_NetMenu[ lIdx++ ].Leave         = NULL;

 s_NetMenu[ lIdx   ].m_Type        = MENU_ITEM_TYPE_TEXT;
 s_NetMenu[ lIdx   ].m_pOptionName = &STR_DUPLEX_MODE;
 s_NetMenu[ lIdx   ].m_IconLeft    = 0;
 s_NetMenu[ lIdx   ].m_IconRight   = 0;
 s_NetMenu[ lIdx   ].Handler       = _duplex_handler;
 s_NetMenu[ lIdx   ].Enter         = NULL;
 s_NetMenu[ lIdx++ ].Leave         = NULL;

 s_NetMenu[ lIdx ].m_Type        = MENU_ITEM_TYPE_TEXT;
 s_NetMenu[ lIdx ].m_pOptionName = &STR_STANDARD;
 s_NetMenu[ lIdx ].m_IconLeft    = 0;
 s_NetMenu[ lIdx ].m_IconRight   = 0;
 s_NetMenu[ lIdx ].Handler       = _standt_handler;
 s_NetMenu[ lIdx ].Enter         = NULL;
 s_NetMenu[ lIdx ].Leave         = NULL;

 _update_opmode ( apMenu );
 _update_dup_mode ( apMenu );
 _update_standard ( apMenu );

}  /* end _build_net_menu */

static void _network_handler ( GUIMenu* apMenu, int aDir ) {

 GUIMenuState* lpState = GUI_MenuPushState ( apMenu );

 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_NetMenu;
 lpState -> m_pTitle = &STR_NETWORK_SETTINGS1;

 s_NetMenu[ 1 ].m_IconRight = ( unsigned int )(  ( g_Config.m_NetworkFlags & SMS_DF_SMB ) ? &STR_SMB_CIFS : &STR_PS2DEV_HOST );

 _build_net_menu ( apMenu );
 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _network_handler */

static void _opmode_handler ( GUIMenu* apMenu, int aDir ) {

 int lMode = ( g_Config.m_NetworkFlags >> 8 ) & 3;

 lMode += aDir;

 if ( lMode < 0 )
  lMode = 2;
 else if ( lMode > 2 )
  lMode = 0;

 lMode <<= 8;

 g_Config.m_NetworkFlags &= 0xFFFFFCFF;
 g_Config.m_NetworkFlags |= lMode;

 _update_opmode ( apMenu );

 apMenu -> Redraw ( apMenu );

}  /* end _opmode_handler */

static void _duplex_handler ( GUIMenu* apMenu, int aDir ) {

 g_Config.m_NetworkFlags ^= SMS_DF_HALF;

 _update_dup_mode ( apMenu );

 apMenu -> Redraw ( apMenu );

}  /* end _duplex_handler */

static void _standt_handler ( GUIMenu* apMenu, int aDir ) {

 g_Config.m_NetworkFlags ^= SMS_DF_10;

 _update_standard ( apMenu );

 apMenu -> Redraw ( apMenu );

}  /* end _standt_handler */

extern void _smb_menu ( GUIMenu* );

static void _smbsrv_handler ( GUIMenu* apMenu, int aDir ) {

 _smb_menu ( apMenu );

}  /* end _smbsrv_handler */

GUIObject* GUI_CreateMenuSMS ( void ) {

 GUIMenu*      retVal  = ( GUIMenu* )GUI_CreateMenu ();
 GUIMenuState* lpState;

 retVal -> m_Color      = 0x78301010UL;
 retVal -> HandleEvent  = GUIMenuSMS_HandleEvent;
 retVal -> Redraw       = GUIMenuSMS_Redraw;
 retVal -> m_pActiveObj = g_pActiveNode;
 retVal -> m_pState     = SMS_ListInit ();

 lpState = GUI_MenuPushState ( retVal );
 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_SMSMenu;
 lpState -> m_pLast  = &s_SMSMenu[ sizeof ( s_SMSMenu ) / sizeof ( s_SMSMenu[ 0 ] ) - 1 ];
 lpState -> m_pTitle = &STR_SMS_MENU;

 GUIMenuSMS_UpdateStatus ( retVal );
 _setup_dimensions (  ( GUIMenu* )retVal  );

 s_fHDD = 0;

 return ( GUIObject* )retVal;

}  /* end GUI_CreateMenuSMS */
