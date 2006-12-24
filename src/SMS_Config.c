/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_Config.h"
#include "SMS_GS.h"
#include "SMS_MC.h"
#include "SMS_Locale.h"
#include "SMS_FileContext.h"
#include "SMS_IOP.h"

#include <malloc.h>
#include <fileio.h>
#include <string.h>

extern void _check_dc_offset ( void );
extern unsigned char g_IconSMS[ 2020 ] __attribute__(   (  section( ".data" )  )   );

SMSConfig    g_Config       __attribute__(   (  section( ".data" )  )   );
SMBLoginInfo g_SMBLoginInfo __attribute__(   (  section( ".data" )  )   );

unsigned int g_Palette[ 16 ] __attribute__(   (  section( ".data" )  )   ) = {
 GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x00, 0x00, 0x40, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x00, 0x40, 0x00, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x00, 0x40, 0x40, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x40, 0x00, 0x00, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x40, 0x00, 0x40, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x40, 0x40, 0x00, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x40, 0x40, 0x40, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x80, 0x80, 0x80, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x00, 0x00, 0xFF, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x00, 0xFF, 0x00, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0x00, 0xFF, 0xFF, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0xFF, 0x00, 0x00, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0xFF, 0x00, 0xFF, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0xFF, 0xFF, 0x00, 0x80, 0x00 ),
 GS_SET_RGBAQ( 0xFF, 0xFF, 0xFF, 0x80, 0x00 )
};

static char s_pASCII [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:SMS/ascii.mtf";
static char s_pLatin2[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:SMS/latin2.mtf";
static char s_pCyrill[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:SMS/cyrillic.mtf";
static char s_pLatin1[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:SMS/latin1.mtf";
static char s_pGreek [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:SMS/greek.mft";
static char s_pSMS   [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "/SMS";
static char s_pSMSCfg[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "/SMS/SMS.cfg";
static char s_pSMSPal[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "/SMS/SMS.pal";
static char s_pIcoSys[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:SMS/icon.sys";
static char s_pMC0SMS[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:SMS";
static char s_pSMSIcn[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:/SMS/SMS.icn";
static char s_pMC0SMC[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:/SMS/SMS.cfg";
static char s_pPS2D  [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "PS2D";
static char s_pSMSICN[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "SMS.icn";
static char s_pSMSSMB[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:/SMS/SMS.smb";

static const char* s_pFontNames[ 5 ] __attribute__(   (  section( ".data" )  )   ) = {
 s_pASCII, s_pLatin2, s_pCyrill, s_pLatin1, s_pGreek
};

static void _load_font ( unsigned int anIndex ) {

 int lFD = fioOpen ( s_pFontNames[ anIndex ], O_RDONLY );

 if ( lFD >= 0 ) {

  long lSize = fioLseek ( lFD, 0, SEEK_END );

  if ( lSize > 0 ) {

   unsigned char* lpBuff = ( unsigned char* )malloc ( lSize );

   fioLseek ( lFD, 0, SEEK_SET );
   fioRead ( lFD, lpBuff, lSize );

   GSFont_Set ( anIndex, lpBuff );

  }  /* end if */

  fioClose ( lFD );

 }  /* end if */

}  /* end LoadFont */

int SMS_LoadConfig ( void  ) {

 int retVal = 0;
 int lUDFL  = g_Config.m_BrowserFlags & SMS_BF_UDFL;
 int lRes;

 g_Config.m_BrowserABCIdx    = 16;
 g_Config.m_BrowserIBCIdx    = 13;
 g_Config.m_BrowserTxtIdx    = 15;
 g_Config.m_NetworkFlags     = SMS_DF_AUTO_HDD;
 g_Config.m_PlayerVolume     = 12;
 g_Config.m_DisplayMode      = GSVideoMode_Default;
 g_Config.m_DisplayCharset   = GSCodePage_WinLatin1;
 g_Config.m_PlayerFlags      = SMS_PF_SUBS | SMS_PF_ANIM | SMS_PF_TIME;
 g_Config.m_PlayerSCNIdx     = 15;
 g_Config.m_PlayerSCBIdx     = 16;
 g_Config.m_PlayerSCIIdx     = 11;
 g_Config.m_PlayerSCUIdx     = 10;
 g_Config.m_BrowserSCIdx     = 16;
 g_Config.m_BrowserSBCIdx    = 16;
 g_Config.m_PlayerSubOffset  = 32;
 g_Config.m_PlayerVBCIdx     = 11;
 g_Config.m_PlayerSBCIdx     = 11;
 g_Config.m_ScrollBarNum     = 32;
 g_Config.m_ScrollBarPos     = SMScrollBarPos_Bottom;
 g_Config.m_PlayerBrightness = 12;

 *(  ( unsigned int* )&g_Config.m_PAR[ 0 ]  ) = 0x3F6EEEEF;
 *(  ( unsigned int* )&g_Config.m_PAR[ 1 ]  ) = 0x3F888889;

 strcpy ( g_Config.m_Language, g_pDefStr );

 g_Config.m_DispH[ 0 ] = 448;
 g_Config.m_DispH[ 1 ] = 512;

 MC_GetInfo ( 0, 0, &lRes, &lRes, &lRes );
 MC_Sync ( &lRes );

 if ( lRes > -2 ) {

  SMS_MCTable lDir __attribute__(   (  aligned( 64 )  )   );

  MC_GetDir ( 0, 0, s_pSMS, 0, 1, &lDir );
  MC_Sync ( &lRes );

  if ( lRes ) {

   int lFD = MC_OpenS ( 0, 0, s_pSMSCfg, O_RDONLY );

   if ( lFD >= 0 ) {

    int lLen = MC_ReadS ( lFD, &g_Config, 272 );

    if ( lLen == 272 ) {

     if ( g_Config.m_Version > 0 ) {

      lLen = MC_ReadS ( lFD, &g_Config.m_BrowserABCIdx, 32 );

      if ( lLen == 32 ) {

       if ( g_Config.m_Version > 1 ) {

        lLen = MC_ReadS ( lFD, &g_Config.m_PlayerFlags, 60 );

        if ( lLen == 60 ) {

         if ( g_Config.m_Version > 2 ) {

          lLen = MC_ReadS ( lFD, &g_Config.m_SubHIncr, 80 );

          if ( lLen == 80 ) {

           if ( g_Config.m_Version > 3 ) {

            lLen = MC_ReadS ( lFD, g_Config.m_DXPALOther, 32 );

            if ( lLen == 32 ) {

             if ( g_Config.m_Version > 4 ) {

              lLen = MC_ReadS ( lFD, &g_Config.m_PlayerBrightness, 4 );

              if ( lLen == 4 ) retVal = 1;

             } else retVal = 1;

            }  /* end if */

           } else retVal = 1;

          }  /* end if */

         } else retVal = 1;

        }  /* end if */

       } else retVal = 1;

      }  /* end if */

     } else retVal = 1;

    }  /* end if */

    MC_CloseS ( lFD );

   }  /* end if */

   lFD = MC_OpenS ( 0, 0, s_pSMSPal, O_RDONLY );

   if ( lFD >= 0 ) {

    MC_ReadS (  lFD, g_Palette, sizeof ( g_Palette )  );
    MC_CloseS ( lFD );

   }  /* end if */

   for ( lRes = 0; lRes < 16; ++lRes ) g_Palette[ lRes ] = ( g_Palette[ lRes ] & 0x00FFFFFF ) | 0x60000000;
   for ( lRes = 0; lRes <  5; ++lRes ) _load_font ( lRes );

  }  /* end if */

 }  /* end if */

 if ( g_Config.m_Version < 3 ) {

  g_Config.m_NetworkFlags |= SMS_DF_AUTO_HDD;
  g_Config.m_DX            = 0;
  g_Config.m_DY            = 0;

 }  /* end if */

 g_Config.m_Version = 5;

 if ( !lUDFL ) g_Config.m_BrowserFlags &= ~SMS_BF_UDFL;

 SMS_clip ( g_Config.m_DX, -160, 160 );
 SMS_clip ( g_Config.m_DY,  -64,  64 );

 STIO_SetIOMode ( STIOMode_Ordinary );

 {  /* begin block */

  FileContext* lpFileCtx = STIO_InitFileContext ( s_pSMSSMB, NULL );

  if ( lpFileCtx ) {

   File_GetString (  lpFileCtx, g_SMBLoginInfo.m_ServerIP,   sizeof ( g_SMBLoginInfo.m_ServerIP   )  );
   File_GetString (  lpFileCtx, g_SMBLoginInfo.m_ServerName, sizeof ( g_SMBLoginInfo.m_ServerName )  );
   File_GetString (  lpFileCtx, g_SMBLoginInfo.m_ClientName, sizeof ( g_SMBLoginInfo.m_ClientName )  );
   File_GetString (  lpFileCtx, g_SMBLoginInfo.m_UserName,   sizeof ( g_SMBLoginInfo.m_UserName   )  );
   File_GetString (  lpFileCtx, g_SMBLoginInfo.m_Password,   sizeof ( g_SMBLoginInfo.m_Password   )  );

   if ( g_SMBLoginInfo.m_ServerIP  [ 0 ] &&
        g_SMBLoginInfo.m_ServerName[ 0 ] &&
        g_SMBLoginInfo.m_ClientName[ 0 ] &&
        g_SMBLoginInfo.m_UserName  [ 0 ] &&
        g_SMBLoginInfo.m_Password  [ 0 ]
   ) g_IOPFlags |= SMS_IOPF_SMBINFO;

   lpFileCtx -> Destroy ( lpFileCtx );

  }  /* end if */

 }  /* end block */

 return retVal;

}  /* end SMS_LoadCondig */

int SMS_SaveConfig ( void ) {

 int retVal = 0;
 int lRes;

 MC_GetInfo ( 0, 0, &lRes, &lRes, &lRes );
 MC_Sync ( &lRes );

 if ( lRes > -2 ) {

  SMS_MCTable lDir __attribute__(   (  aligned( 64 )  )   );

  MC_GetDir ( 0, 0, s_pSMS, 0, 1, &lDir );
  MC_Sync ( &lRes );

  if (  lRes || !fioMkdir ( s_pMC0SMS )  ) {

   int lFD = fioOpen ( s_pIcoSys, O_RDONLY );

   if ( lFD < 0 ) {

    static int lBgClr[ 4 ][ 4 ] __attribute__(   (  section( ".data" )  )    ) = {
     {  68,  23, 116,  0 },
     { 255, 255, 255,  0 },
     { 255, 255, 255,  0 },
     {  68,  23, 116,  0 }
	};
    static float lLightDir[ 3 ][ 4 ] __attribute__(   (  section( ".data" )  )    ) = {
     {  0.5F,  0.5F,  0.5F, 0.0F },
     {  0.0F, -0.4F, -0.1F, 0.0F },
     { -0.5F, -0.5F,  0.5F, 0.0F }
	};
	static float lLightCol[ 3 ][ 4 ] __attribute__(   (  section( ".data" )  )    ) = {
     { 0.3F, 0.3F, 0.3F, 0.0F },
     { 0.4F, 0.4F, 0.4F, 0.0F },
     { 0.5F, 0.5F, 0.5F, 0.0F }
	};
	static float lAmb[ 4 ] __attribute__(   (  section( ".data" )  )    ) = { 0.5F, 0.5F, 0.5F, 0.0F };

    SMS_MCIcon lIcon; memset ( &lIcon, 0, sizeof ( SMS_MCIcon )  );

	strcpy ( lIcon.m_Header, s_pPS2D );
	strcpy_sjis (  ( short* )&lIcon.m_Title, s_pSMS + 1  );

	lIcon.m_Offset =   16;
	lIcon.m_Trans  = 0x60;

    memcpy ( lIcon.m_ClrBg,    lBgClr,    sizeof ( lBgClr    )  );
    memcpy ( lIcon.m_LightDir, lLightDir, sizeof ( lLightDir )  );
    memcpy ( lIcon.m_LightCol, lLightCol, sizeof ( lLightCol )  );
    memcpy ( lIcon.m_LightAmb, lAmb,      sizeof ( lAmb      )  );

    strcpy ( lIcon.m_View, s_pSMSICN );
    strcpy ( lIcon.m_Copy, s_pSMSICN );
    strcpy ( lIcon.m_Del,  s_pSMSICN );

 	lFD = fioOpen ( s_pIcoSys, O_WRONLY | O_CREAT );

	if ( lFD >= 0 ) {

     fioWrite (  lFD, &lIcon, sizeof ( lIcon )  );
     fioClose ( lFD );

     lFD = fioOpen ( s_pSMSIcn, O_WRONLY | O_CREAT );

     if ( lFD >= 0 ) {

      fioWrite (  lFD, g_IconSMS, sizeof ( g_IconSMS )  );
      fioClose ( lFD );

     }  /* end if */

    }  /* end if */

   } else fioClose ( lFD );

   lFD = fioOpen ( s_pMC0SMC, O_WRONLY | O_CREAT );

   if ( lFD >= 0 ) {

    if (  fioWrite (  lFD, &g_Config, sizeof ( g_Config )  ) == sizeof ( g_Config )  ) retVal = 1;

    fioClose ( lFD );

   }  /* end if */

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end SMS_SaveConfig */
