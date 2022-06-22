/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 BraveDog
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2006 Voldemar_u2
# (c) 2006 ffgriever
# (c) 2006 Npl
# (c) 2007 lior e
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_SubtitleContext.h"
#include "SMS_FileContext.h"
#include "SMS_GS.h"
#include "SMS_VIF.h"
#include "SMS_IPU.h"
#include "SMS_DMA.h"
#include "SMS_GUI.h"
#include "SMS_Config.h"
#include "SMS_Locale.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <mbstring.h>

#define SUB_MAXLEN       512
#define SUB_DEF_LINE_LEN 4.0F

extern int TranslateUTF8 ( int, char*, int, const char* );

static SubtitleContext s_SubCtx __attribute__(   (  section( ".bss" )  )   );

static char*          ( *_strtok_func        ) ( char*, const char*                                   );
static unsigned int   ( *_strlen_func        ) ( const char*                                          );
static u64*           ( *_render_string_func ) ( const char*, int, int, int, u64*          , int, int );
static int            ( *_string_width_func  ) ( char*, int, int                                      );

static u64*           _sb_render_string (
                       const char* apStr, int aLen,
                       int aX, int anY, u64*           apDMA,
                       int aDW, int aDH
                      ) {

 const char*      lpEnd  = apStr + aLen;
 float            lARX   = ( 32 + aDW ) / 32.0F;
 float            lARY   = ( 32 + aDH ) / 32.0F;
 int              lW     = ( int )( 32.0F * lARX + 0.5F );
 int              lH     = anY + ( int )( 32.0F * lARY + 0.5F );
 GSMTKFontHeader* lpXHdr = g_Fonts[ g_GSCtx.m_CodePage ];

 while ( apStr < lpEnd ) {

  char             lChr = apStr[ 0 ];
  int              lCW;
  GSMTKFontHeader* lpHdr;

  if ( lChr < 0 ) {
   lpHdr = lpXHdr;
   lChr += 128;
  } else {
   lpHdr  = g_pASCII;
   lChr  -= ' ';
  }  /* end else */

  lCW = ( int )(  ( float )GSFont_UnpackChr ( lpHdr, ( unsigned char )lChr, &apDMA[ 6 ] ) * lARX + 0.5F  );

  apDMA[  0 ] = GIF_TAG( 1, 0, 0, 0, GIFTAG_FLG_PACKED, 1 );
  apDMA[  1 ] = GIFTAG_REGS_AD;
  apDMA[  2 ] = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL );
  apDMA[  3 ] = GS_TRXDIR;
  apDMA[  4 ] = GIF_TAG( 32, 0, 0, 0, GIFTAG_FLG_IMAGE, 0 );
  apDMA[  5 ] = 0L;
  apDMA[ 70 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
  apDMA[ 71 ] = GIFTAG_REGS_AD;
  apDMA[ 72 ] = GS_SET_TEXFLUSH( 0 );
  apDMA[ 73 ] = GS_TEXFLUSH;
  apDMA[ 74 ] = GIF_TAG( 2, 0, 0, 0, 1, 2 );
  apDMA[ 75 ] = GS_UV | ( GS_XYZ2 << 4 );
  apDMA[ 76 ] = GS_SET_UV( 8, 8 );
  apDMA[ 77 ] = GS_XYZ ( aX, anY, 0 );
  apDMA[ 78 ] = GS_SET_UV( 506, 506 );
  apDMA[ 79 ] = GS_XYZ ( aX + lW, lH, 0 );

  aX      += lCW;
  apDMA   +=  80;
  apStr   +=   1;

 }  /* end while */

 return apDMA;

}  /* end _sb_render_string */

static int _sb_string_width ( char* apStr, int aLen, int aDW ) {

 int                  retVal = 0;
 const          char* lpEnd  = apStr + aLen;
 float                lAR    = ( 32 + aDW ) / 32.0F;
 GSMTKFontHeader*     lpXHdr = g_Fonts[ g_GSCtx.m_CodePage ];

 while ( apStr <= lpEnd ) {

  char             lChr  = (  ( char* )apStr  )[ 0 ];
  GSMTKFontHeader* lpHdr;

  if ( lChr < 0 ) {
   lpHdr = lpXHdr;
   lChr += 128;
  } else {
   lpHdr  = g_pASCII;
   lChr  -= ' ';
  }  /* end else */

  retVal += ( int )(    ( float )GSFont_CharWidth (  lpHdr, ( unsigned char )lChr  ) * lAR + 0.5F   );
  apStr  += 1;

 }  /* end while */

 return retVal;

}  /* end _sb_string_width */

static u64*           _mb_render_string (
                       const char* apStr, int aLen,
                       int aX, int anY, u64*           apDMA,
                       int aDW, int aDH
                      ) {

 const char* lpEnd = apStr + aLen;
 float       lARX  = ( 32 + aDW ) / 32.0F;
 float       lARY  = ( 32 + aDH ) / 32.0F;
 int         lW    = ( int )( 32.0F * lARX + 0.5F );
 int         lH    = anY + ( int )( 32.0F * lARY + 0.5F );

 while ( apStr < lpEnd ) {

  char             lChr  = apStr[ 0 ];
  int              lOff  = ( lChr & 0x7F ) + 1;
  int              lCW;
  GSMTKFontHeader* lpHdr;

  lCW    = lChr < 0;
  lOff   = lCW ? lOff : 0;
  apStr += lCW;
  lpHdr  = ( GSMTKFontHeader* )(   (  ( unsigned char* )g_MBFont  ) + g_MBFont[ lOff ]   );
  lCW    = ( int )(   ( float )GSFont_UnpackChr (
   lpHdr, (  ( unsigned char* )apStr  )[ 0 ], &apDMA[ 6 ]  ) * lARX + 0.5F
  );

  apDMA[  0 ] = GIF_TAG( 1, 0, 0, 0, GIFTAG_FLG_PACKED, 1 );
  apDMA[  1 ] = GIFTAG_REGS_AD;
  apDMA[  2 ] = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL );
  apDMA[  3 ] = GS_TRXDIR;
  apDMA[  4 ] = GIF_TAG( 32, 0, 0, 0, GIFTAG_FLG_IMAGE, 0 );
  apDMA[  5 ] = 0L;
  apDMA[ 70 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
  apDMA[ 71 ] = GIFTAG_REGS_AD;
  apDMA[ 72 ] = GS_SET_TEXFLUSH( 0 );
  apDMA[ 73 ] = GS_TEXFLUSH;
  apDMA[ 74 ] = GIF_TAG( 2, 0, 0, 0, 1, 2 );
  apDMA[ 75 ] = GS_UV | ( GS_XYZ2 << 4 );
  apDMA[ 76 ] = GS_SET_UV( 8, 8 );
  apDMA[ 77 ] = GS_XYZ ( aX, anY, 0 );
  apDMA[ 78 ] = GS_SET_UV( 506, 506 );
  apDMA[ 79 ] = GS_XYZ ( aX + lW, lH, 0 );

  aX      += lCW;
  apDMA   +=  80;
  apStr   +=   1;

 }  /* end while */

 return apDMA;

}  /* end _mb_render_string */

static int _mb_string_width ( char* apStr, int aLen, int aDW ) {

 int                  retVal = 0;
 const char*          lpEnd  = apStr + aLen;
 float                lAR    = ( 32 + aDW ) / 32.0F;

 while ( apStr <= lpEnd ) {

  char             lChr  = (  ( char* )apStr  )[ 0 ];
  int              lOff  = ( lChr & 0x7F ) + 1;
  int              lCW;
  GSMTKFontHeader* lpHdr;

  lCW    = lChr < 0;
  lOff   = lCW ? lOff : 0;
  apStr += lCW;
  lpHdr  = ( GSMTKFontHeader* )(   (  ( unsigned char* )g_MBFont  ) + g_MBFont[ lOff ]   );

  retVal += ( int )(  ( float )GSFont_CharWidth ( lpHdr, apStr[ 0 ] ) * lAR + 0.5F );
  apStr  += 1;

 }  /* end while */

 return retVal;

}  /* end _mb_string_width */

static void sub_gets ( char* apStr, FileContext* apFileCtx ) {

 File_GetString ( apFileCtx, apStr, SUB_MAXLEN );

 ++s_SubCtx.m_ErrorLine;

}  /* end sub_gets */

static unsigned int _update_size ( SMS_List* apList, unsigned int aLen ) {

 SMS_ListNode* lpNode   = apList -> m_pHead;
 unsigned int  lPackLen = s_SubCtx.m_nPreAlloc;
 int           lfOSub   = g_Config.m_PlayerFlags & SMS_PF_OSUB;

 while ( lpNode ) {

  if ( lfOSub ) lPackLen += 64;

  lPackLen += _strlen_func (  _STR( lpNode )  ) * 640;
  lpNode    = lpNode -> m_pNext;

 }  /* end while */

 if ( lPackLen > aLen ) aLen = lPackLen;

 return aLen;

}  /* end _update_size */

static char* strrspc ( const char* apLine, char* apPos ) {

 char* retVal = NULL;

 while ( apPos >= apLine ) {

  if ( *apPos == ' ' ) {

   retVal = apPos;
   break;

  }  /* end if */

  --apPos;

 }  /* end while */

 return retVal;

}  /* end strrspc */

static void _add_line ( SMS_List* apList, char* apLine, unsigned int aColorIdx ) {

 int lScrW = g_GSCtx.m_Width;
 int lDW   = g_Config.m_SubHIncr;
 int lLen;
 int lTxtW;
next:
 lLen  = strlen ( apLine );
 lTxtW = _string_width_func ( apLine, lLen, lDW );

 if ( lTxtW < lScrW ) {

  if (  ( g_Config.m_PlayerSAlign == 2 ||
          g_Config.m_PlayerSAlign == 3
        ) && !g_MBFont
  ) SMS_ReverseString ( apLine, lLen );

  SMS_ListPushBack ( apList, apLine );
  apList -> m_pTail -> m_Param = ( unsigned int )( void* )aColorIdx;

 } else {

  char* lpSpace;

  lpSpace = strrchr ( apLine, ' ' );

  while ( 1 ) {

   if ( lpSpace ) {

    lTxtW = _string_width_func ( apLine, lpSpace - apLine, lDW );

    if ( lTxtW < lScrW ) {

     *lpSpace = '\x00';

     if (  g_Config.m_PlayerSAlign == 2 && !g_MBFont  ) {

      int lX, lLen = strlen ( apLine );

      SMS_ReverseString ( apLine, lpSpace - apLine );

      while ( 1 ) {

       lX = g_GSCtx.m_Width - _string_width_func ( apLine, lLen, g_Config.m_SubHIncr ) - 32;

       if ( lX >= 0 ) break;

       ++apLine;
       --lLen;

      }  /* end while */

     } else if ( g_Config.m_PlayerSAlign == 3 && !g_MBFont ) {

      int lX, lLen = strlen ( apLine );

      SMS_ReverseString ( apLine, lpSpace - apLine );

      while ( 1 ) {

       lX = (  g_GSCtx.m_Width - _string_width_func ( apLine, lLen, g_Config.m_SubHIncr )  ) >> 1;

       if ( lX > g_GSCtx.m_Width ) lX = 0;
       if ( lX >= 0              ) break;

       ++apLine;
       --lLen;

      }  /* end while */

     }  /* end if */

     SMS_ListPushBack ( apList, apLine );
     apList -> m_pTail -> m_Param = ( unsigned int )( void* )aColorIdx;

     apLine = ++lpSpace;
     goto next;

    } else lpSpace = strrspc ( apLine, lpSpace - 1 );

   } else {

    char lChr;

    if ( g_Config.m_PlayerSAlign == 2 || g_Config.m_PlayerSAlign == 3 ) {
     lChr = apLine[ 0 ];
     apLine += 1 + ( g_MBFont && lChr < 0 );
    } else {
     lChr = apLine[ lLen - 1 ];
     apLine[ lLen - 1 - ( g_MBFont && lChr < 0 ) ] = '\x00';
    }  /* end else */

    goto next;

   }  /* end else */

  }  /* end while */

 }  /* end else */

}  /* end _add_line */

static int _load_sub ( FileContext* apFileCtx, float aFPS, int aBase, int aRatio ) {

 char             lLine[ SUB_MAXLEN ];
 float            lMSPerFrame = 1000.0F / aFPS;
 int              retVal      = 0;
 int              lfUTF8      = 0;
 unsigned         lPos        = apFileCtx -> m_CurPos;
 SubtitleContext* lpCtx       = &s_SubCtx;
 int              lnLines;
 SubNode*         lpNode;

 if (  File_GetByte ( apFileCtx ) == 0xEF &&
       File_GetByte ( apFileCtx ) == 0xBB &&
       File_GetByte ( apFileCtx ) == 0xBF
 )

  lfUTF8 = 1;

 else apFileCtx -> Seek ( apFileCtx, lPos );

 s_SubCtx.m_Cnt = 0;

 while ( 1 ) {

  s64     lTime [ 2 ];
  int     lFrame[ 2 ];
  int     i;
  int     lHour   = 0;
  int     lMinute = 0;
  int     lSecond = 0;
  char*   lpPtr;

  lFrame[ 1 ] = 0;

  sub_gets ( lLine, apFileCtx );

  if ( lLine[ 0 ] == '\x00' ) {

   if (  !FILE_EOF( apFileCtx )  ) continue;

   retVal = 1;
   break;

  }  /* end if */

  if (   (  sscanf ( lLine, "{%d}{%d}", &lFrame[ 0 ], &lFrame[ 1 ] ) == 2  )   ) {

   if ( lFrame[ 0 ] == 0 ) continue;
   if ( lFrame[ 1 ] == 0 ) lFrame[ 1 ] = lFrame[ 0 ] + ( int )(  ( SUB_DEF_LINE_LEN * aFPS ) + 0.5F  );

   for ( i = 0; i < 2; ++i ) lTime[ i ] = ( s64  )(  ( float )lFrame[ i ] * lMSPerFrame + 0.5F  );

   lpPtr = strchr ( lLine, '}' ) + 1;
   lpPtr = strchr ( lpPtr, '}' ) + 1;

   if (  lpPtr == ( char* )1  ) continue;

  } else if (   (  sscanf ( lLine, "[%d][%d]", &lFrame[ 0 ], &lFrame[ 1 ] ) == 2  )   ) {

   if ( lFrame[ 0 ] == 0 ) continue;
   if ( lFrame[ 1 ] == 0 ) lFrame[ 1 ] = lFrame[ 0 ] + ( int )( SUB_DEF_LINE_LEN * 10.0F );

   for ( i = 0; i < 2; ++i ) lTime[ i ] = ( s64  )( lFrame[ i ] * 100  );

   lpPtr = strchr ( lLine, ']' ) + 1;
   lpPtr = strchr ( lpPtr, ']' ) + 1;

   if (  lpPtr == ( char* )1  ) continue;

  } else if (   (  sscanf ( lLine, "%d:%d:%d:", &lHour, &lMinute, &lSecond ) == 3  )   ) {

   lTime[ 0 ] = ( s64  )(   ( lSecond + ( lMinute * 60 ) + ( lHour * 3600 )  ) * 1000   );
   lTime[ 1 ] = lTime[ 0 ] + ( s64  )( SUB_DEF_LINE_LEN * 1000.0F );

   lpPtr = strchr ( lLine, ':' ) + 1;
   lpPtr = strchr ( lpPtr, ':' ) + 1;
   lpPtr = strchr ( lpPtr, ':' ) + 1;

   if (  lpPtr == ( char* )1  ) continue;

  } else {

   s_SubCtx.m_ErrorCode = SubtitleError_Format;
   break;

  }  /* end else */

  lpNode = ( SubNode* )malloc (  sizeof ( SubNode )  );
  lpNode -> m_pList = SMS_ListInit ();
  lpNode -> m_pNext = NULL;

  if ( !lpCtx -> m_pHead )

   lpCtx -> m_pHead = lpCtx -> m_pTail = lpNode;

  else {

   lpCtx -> m_pTail -> m_pNext = lpNode;
   lpCtx -> m_pTail            = lpNode;

  }  /* end else */

  lpPtr   = _strtok_func ( lpPtr, "|" );
  lnLines = 0;

  while ( lpPtr ) {
// skip tag (I didn't find any clear specs about them)
   if ( *lpPtr == '{' ) {

    while ( *lpPtr && *lpPtr != '}' ) ++lpPtr;

    if ( *lpPtr )
     ++lpPtr;
    else break;

   }  /* end if */

   if ( lfUTF8 ) {
    char lBuff[ SUB_MAXLEN ];
    TranslateUTF8 ( g_GSCtx.m_CodePage, lBuff, SUB_MAXLEN, lpPtr );
    _add_line ( lpNode -> m_pList, lBuff, 0 );
   } else _add_line ( lpNode -> m_pList, lpPtr, 0 );

   lpPtr = _strtok_func ( NULL, "|" );
   ++lnLines;

  }  /* end while */

  if ( !lnLines ) _add_line ( lpNode -> m_pList, " ", 0 );

  lpCtx -> m_nAlloc = _update_size ( lpNode -> m_pList, lpCtx -> m_nAlloc );

  lpNode -> m_Begin = SMS_Rescale ( lTime[ 0 ], aBase, aRatio );
  lpNode -> m_End   = SMS_Rescale ( lTime[ 1 ], aBase, aRatio );

  ++s_SubCtx.m_Cnt;

 }  /* end while */

 return retVal;

}  /* end _load_sub */

static int _blank_line ( char* apStr ) {

 while ( *apStr ) if (  !isspace ( *(( unsigned char* )apStr++) )  ) return 0;

 return 1;

}  /* end _blank_line */

static int _load_srt ( FileContext* apFileCtx, int aBase, int aRatio ) {

 char             lLine[ SUB_MAXLEN ];
 int              lTotSerial;
 int              retVal = 0;
 unsigned int     lPos   = apFileCtx -> m_CurPos;
 int              lfUTF8 = 0;
 SubtitleContext* lpCtx  = &s_SubCtx;

 if (  File_GetByte ( apFileCtx ) == 0xEF &&
       File_GetByte ( apFileCtx ) == 0xBB &&
       File_GetByte ( apFileCtx ) == 0xBF
 )

  lfUTF8 = 1;

 else apFileCtx -> Seek ( apFileCtx, lPos );

 sub_gets ( lLine, apFileCtx );

 if (  !sscanf ( lLine, "%d", &lTotSerial )  ) {
  s_SubCtx.m_ErrorCode = SubtitleError_Sequence;
  return retVal;
 }  /* end if */

 lTotSerial = 0;

 s_SubCtx.m_ErrorLine = 0;

 apFileCtx -> Seek ( apFileCtx, 0 );

 while ( 1 ) {

  int64_t      lTime[ 2 ];
  unsigned int lH[ 2 ], lM[ 2 ], lS[ 2 ], lMS[ 2 ];
  unsigned int i, lColorIdx = 0;
  int          lSerial;
  SubNode*     lpNode;
  char*        lpPtr;

  sub_gets ( lLine, apFileCtx );  /* serial number */

  if (  _blank_line ( lLine )  ) {

   if (  FILE_EOF( apFileCtx )  ) {

    s_SubCtx.m_Cnt = lTotSerial;
    retVal         = 1;

    break;

   }  /* end if */

   continue;

  }  /* end if */

  if (  !sscanf ( lLine, "%d", &lSerial )  ) continue;

  sub_gets ( lLine, apFileCtx );  /* timestamps */

  if (  sscanf (
         lLine, "%u:%u:%u,%u", &lH[ 0 ], &lM[ 0 ], &lS[ 0 ], &lMS[ 0 ]
        ) != 4
  )	continue;

  lpPtr = strstr ( lLine, " --> " );

  if ( !lpPtr ) continue;

  if (  sscanf (
         lpPtr + 5, "%u:%u:%u,%u", &lH[ 1 ], &lM[ 1 ], &lS[ 1 ], &lMS[ 1 ]
        ) != 4
  ) continue;

  for ( i = 0; i < 2; ++i ) lTime[ i ] = lH[ i ] * 3600000 + lM[ i ] * 60000 + lS[ i ] * 1000 + lMS[ i ];

  lpNode = ( SubNode* )malloc (  sizeof ( SubNode )  );
  lpNode -> m_pList = SMS_ListInit ();
  lpNode -> m_pNext = NULL;

  if ( !lpCtx -> m_pHead )

   lpCtx -> m_pHead = lpCtx -> m_pTail = lpNode;

  else {

   lpCtx -> m_pTail -> m_pNext = lpNode;
   lpCtx -> m_pTail            = lpNode;

  }  /* end else */

  ++lTotSerial;

  while ( 1 ) {

   sub_gets ( lLine, apFileCtx );  /* actual subtitle strings */

   if (  lLine[ 0 ] == '\x00' || _blank_line ( lLine )  ) {

    if (  FILE_EOF( apFileCtx )  ) {

     s_SubCtx.m_Cnt = lTotSerial;
     retVal         = 1;

    }  /* end if */

    break;

   } else {

    char* lpPtr   = lLine;
          i       = strlen ( lLine );
          lH[ 0 ] = 0;

    if ( i > 4 ) {

     char* lpEnd = lpPtr + i - 4;

     if (  !strncmp ( lLine, "<b>", 3 )  )

      lColorIdx = lH[ 0 ] = 1;

     else if (  !strncmp ( lLine, "<i>", 3 )  )

      lColorIdx = lH[ 0 ] = 2;

     else if (  !strncmp ( lLine, "<u>", 3 )  ) lColorIdx = lH[ 0 ] = 3;

     if ( lColorIdx ) {

      if ( lH[ 0 ] ) lpPtr += 3;

      if (  !strncmp ( lpEnd, "</b>", 4 ) ||
            !strncmp ( lpEnd, "</i>", 4 ) ||
            !strncmp ( lpEnd, "</u>", 4 )
      ) *lpEnd = '\x00';

     }  /* end if */

    }  /* end if */

    if ( lfUTF8 ) TranslateUTF8 (
                   g_GSCtx.m_CodePage, lpPtr, lLine + SUB_MAXLEN - lpPtr, lpPtr
                  );

    _add_line ( lpNode -> m_pList, lpPtr, lColorIdx );

   }  /* end else */

  }  /* end while */

  if ( !lpNode -> m_pList -> m_Size ) _add_line ( lpNode -> m_pList, " ", lColorIdx );

  lpCtx -> m_nAlloc = _update_size ( lpNode -> m_pList, lpCtx -> m_nAlloc );

  lpNode -> m_Begin = SMS_Rescale ( lTime[ 0 ], aBase, aRatio );
  lpNode -> m_End   = SMS_Rescale ( lTime[ 1 ], aBase, aRatio );

  if ( retVal ) break;

 }  /* end while */

 return retVal;

}  /* end _load_srt */

static void _clear_nodes ( void ) {

 SubtitleContext* lpCtx  = &s_SubCtx;
 SubNode*         lpNode = lpCtx -> m_pHead;

 while ( lpNode ) {

  SubNode* lpNext = lpNode -> m_pNext;

  SMS_ListDestroy ( lpNode -> m_pList, 1 );
  free ( lpNode );

  lpNode = lpNext;

 }  /* end while */

 lpCtx -> m_pHead = lpCtx -> m_pTail = NULL;

}  /* end _clear_nodes */

static void _produce_packets ( void ) {

 SubtitleContext* lpCtx = &s_SubCtx;
 int              i, lf16, lfTSub = !( g_Config.m_PlayerFlags & SMS_PF_OSUB ), lnAlloc = ( lpCtx -> m_nAlloc + 63 ) & ~63;
 u64*             lpDMA0, *lpDMA1;
 SubNode*         lpNode = lpCtx -> m_pHead;

 if ( lfTSub ) {
  lpCtx -> m_nAlloc    += 16;
  lpCtx -> m_nPreAlloc += 16;
 }  /* end if */

 s_SubCtx.m_pDMA[ 0 ] = ( u64*           )memalign ( 64, lnAlloc << 1 );
 s_SubCtx.m_pDMA[ 1 ] = s_SubCtx.m_pDMA[ 0 ] + ( lnAlloc >> 3 );
 s_SubCtx.m_pPackets  = ( SubtitlePacket* )calloc (  s_SubCtx.m_Cnt, sizeof ( SubtitlePacket )  );

 lf16   = g_Config.m_ColorDepth;
 lpDMA0 = s_SubCtx.m_pDMA[ 0 ];
 lpDMA1 = s_SubCtx.m_pDMA[ 1 ];

 *lpDMA0++ = *lpDMA1++ = 0L;
 *lpDMA0++ = *lpDMA1++ = 0L;
 *lpDMA0++ = *lpDMA1++ = GIF_TAG( 4 + lf16 + lfTSub, 0, 0, 0, GIFTAG_FLG_PACKED, 1 );
 *lpDMA0++ = *lpDMA1++ = GIFTAG_REGS_AD;
 *lpDMA0++ = *lpDMA1++ = GS_SET_BITBLTBUF( 0, 0, 0, 0x3FC0, 1, GSPixelFormat_PSMT4 );
 *lpDMA0++ = *lpDMA1++ = GS_BITBLTBUF;
 *lpDMA0++ = *lpDMA1++ = GS_SET_TRXPOS( 0, 0, 0, 0, 0 );
 *lpDMA0++ = *lpDMA1++ = GS_TRXPOS;
 *lpDMA0++ = *lpDMA1++ = GS_SET_TRXREG( 32, 32 );
 *lpDMA0++ = *lpDMA1++ = GS_TRXREG;
 *lpDMA0++ = *lpDMA1++ = 0L;
 *lpDMA0++ = *lpDMA1++ = GS_TEX0_1;
 if ( lf16 ) {
  *lpDMA0++ = *lpDMA1++ = GS_SET_DTHE( 0 );
  *lpDMA0++ = *lpDMA1++ = GS_DTHE;
 }  /* end if */

 if ( lfTSub ) {

  *lpDMA0++ = *lpDMA1++ = GS_SET_PRIM(
                           GS_PRIM_PRIM_SPRITE, GS_PRIM_IIP_FLAT, GS_PRIM_TME_ON,
                           GS_PRIM_FGE_OFF, GS_PRIM_ABE_ON, GS_PRIM_AA1_OFF, GS_PRIM_FST_UV,
                           GS_PRIM_CTXT_1, GS_PRIM_FIX_UNFIXED
                          );
  *lpDMA0++ = *lpDMA1++ = GS_PRIM;

 }  /* end if */

 lf16 = lpDMA0 - s_SubCtx.m_pDMA[ 0 ];

 for ( i = 0; i < s_SubCtx.m_Cnt; ++i, lpNode = lpNode -> m_pNext ) {

  SubtitlePacket* lpPacket = &s_SubCtx.m_pPackets[ i ];

  lpPacket -> m_Begin    = lpNode -> m_Begin;
  lpPacket -> m_End      = lpNode -> m_pNext ? ( lpNode -> m_End > lpNode -> m_pNext -> m_Begin ? lpNode -> m_pNext -> m_Begin - 1 : lpNode -> m_End ) : lpNode -> m_End;
  lpPacket -> m_pDMA     = ( uint64_t* )lf16;
  lpPacket -> m_Pad[ 0 ] = (  ( unsigned int )lpNode  ) | 0x80000000;

  if ( i ) lpPacket -> m_Pad[ 1 ] = ( unsigned int )&lpPacket[ -1 ];

 }  /* end for */

 SyncDCache (  s_SubCtx.m_pDMA[ 0 ], ( char* )s_SubCtx.m_pDMA[ 0 ] + ( lnAlloc << 1 )  );

}  /* end _produce_packets */

static void _create_gs_packet ( SubtitlePacket* apPacket ) {

 SubNode*         lpSNode = ( SubNode* )(  ( apPacket -> m_Pad[ 0 ] << 1 ) >> 1  );
 SubtitleContext* lpCtx   = &s_SubCtx;
 int              lSize   = lpSNode -> m_pList -> m_Size;
 SMS_ListNode*    lpNode  = lpSNode -> m_pList -> m_pHead;
 int              lDH     = g_Config.m_SubVIncr;
 int              lDW     = g_Config.m_SubHIncr;
 int              lChrH   = 32 + lDH;
 unsigned int     lY      = g_GSCtx.m_Height - lChrH * ( lSize - 1 ) - g_Config.m_PlayerSubOffset;
 unsigned int     lQWC    = lpCtx -> m_nPreAlloc >> 4;
 u64*             lpDMA;

 s_SubCtx.m_DMAIdx ^= 1;

 lpDMA = UNCACHED_SEG( s_SubCtx.m_pDMA[ s_SubCtx.m_DMAIdx ] );

 apPacket -> m_Pad[ 0 ] = ( unsigned int )lpSNode;

 lpDMA[ 10 ] = GS_SET_TEX0(
                0x3FC0, 1, GSPixelFormat_PSMT4, 6, 6, GS_TEX_TCC_RGBA,
                GS_TEX_TFX_DECAL, 0, GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, ( unsigned int )lpNode -> m_Param, GS_TEX_CLD_NOUPDATE
               );

 lpDMA = lpDMA + ( unsigned int )apPacket -> m_pDMA;

 while ( lpNode ) {

  unsigned int   lX;
  u64*           lpDMANext;
  int            lLen = strlen (  _STR( lpNode )  );
  unsigned int   lW   = _string_width_func (  _STR( lpNode ), lLen, lDW  );

  switch ( g_Config.m_PlayerSAlign ) {

   default:
   case 0 :  /* center alignment */
    lX = (  g_GSCtx.m_Width - lW  ) >> 1;
    if ( lX > g_GSCtx.m_Width ) lX = 0;
   break;

   case 1:  /* left alignment */
    lX = 32;
   break;

   case 2:  /* right alignment */
    lX = g_GSCtx.m_Width - lW - 32;
   break;

  }  /* end switch */

  if ( g_Config.m_PlayerFlags & SMS_PF_OSUB ) {

   *lpDMA++ = GIF_TAG( 1, 0, 0, 0, 1, 6 );
   *lpDMA++ = GIFTAG_REGS_PRIM | ( GIFTAG_REGS_RGBAQ  <<  4 ) |
                                 ( GIFTAG_REGS_XYZ2   <<  8 ) |
                                 ( GIFTAG_REGS_XYZ2   << 12 ) |
                                 ( GIFTAG_REGS_PRIM   << 16 ) |
                                 ( GIFTAG_REGS_NOP    << 20 );
   *lpDMA++ = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
   *lpDMA++ = GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x80, 0x00 );
   *lpDMA++ = GS_XYZ ( lX,      lY,         0 );
   *lpDMA++ = GS_XYZ ( lX + lW, lY + lChrH, 0 );
   *lpDMA++ = GS_SET_PRIM(
              GS_PRIM_PRIM_SPRITE, GS_PRIM_IIP_FLAT, GS_PRIM_TME_ON,
              GS_PRIM_FGE_OFF, GS_PRIM_ABE_ON, GS_PRIM_AA1_OFF, GS_PRIM_FST_UV,
              GS_PRIM_CTXT_1, GS_PRIM_FIX_UNFIXED
             );
   *lpDMA++ = 0L;
   lQWC    += 4;

  }  /* end if */

  lpDMANext = _render_string_func (  _STR( lpNode ), lLen, lX, lY, lpDMA, lDW, lDH  );
  lQWC     += ( lpDMANext - lpDMA ) >> 1;
  lpDMA     = lpDMANext;
  lpNode    = lpNode -> m_pNext;
  lY       += 32 + lDH;

 }  /* end while */

 if ( g_GSCtx.m_FontTexFmt != GSPixelFormat_PSMT4HL ) {

  *lpDMA++ = GIF_TAG( 1, 1, 0, 0, 0, 1 ); 
  *lpDMA++ = GIFTAG_REGS_AD;
  *lpDMA++ = GS_SET_DTHE( 1 );
  *lpDMA   = GS_DTHE;

 } else lpDMA[ -6 ] = GIF_TAG( 2, 1, 0, 0, 1, 2 );

 lpDMA             = UNCACHED_SEG( s_SubCtx.m_pDMA[ s_SubCtx.m_DMAIdx ] );
 apPacket -> m_QWC = lQWC;
 lpDMA[ 1 ]        = VIF_DIRECT( lQWC - 1 );

 if (  ( int )apPacket -> m_Pad[ 1 ]  ) {
  apPacket = ( SubtitlePacket* )apPacket -> m_Pad[ 1 ];
  apPacket -> m_Pad[ 0 ] |= 0x80000000;
 }  /* end if */

}  /* end _create_gs_packet */

static void _display ( s64  aPTS ) {

 SubtitlePacket* lpPacket = s_SubCtx.m_pPackets;

 while ( s_SubCtx.m_Idx < s_SubCtx.m_Cnt ) {

  if ( aPTS < lpPacket[ s_SubCtx.m_Idx ].m_Begin ) break;

  if ( aPTS >= lpPacket[ s_SubCtx.m_Idx ].m_Begin &&
       aPTS <  lpPacket[ s_SubCtx.m_Idx ].m_End
  ) {

   if (  ( int )lpPacket[ s_SubCtx.m_Idx ].m_Pad[ 0 ] < 0  ) _create_gs_packet ( &lpPacket[ s_SubCtx.m_Idx ] );

   g_IPUCtx.QueuePacket (
    lpPacket[ s_SubCtx.m_Idx ].m_QWC, s_SubCtx.m_pDMA[ s_SubCtx.m_DMAIdx ]
   );
   break;

  }  /* end if */

  ++s_SubCtx.m_Idx;

 }  /* end while */

}  /* end _display */

static void _destroy ( void ) {

 _clear_nodes ();

 if ( s_SubCtx.m_pPackets ) {

  free ( s_SubCtx.m_pPackets );
  s_SubCtx.m_pPackets = NULL;

 }  /* end if */

 if ( s_SubCtx.m_pDMA[ 0 ] ) {

  free ( s_SubCtx.m_pDMA[ 0 ] );
  s_SubCtx.m_pDMA[ 0 ] = NULL;

 }  /* end if */

}  /* end _destroy */

static void _reset ( void ) {

 s_SubCtx.m_pPackets[ s_SubCtx.m_Idx ].m_Pad[ 0 ] |= 0x80000000;
 s_SubCtx.m_Idx                                    = 0;

}  /* end _reset */

SubtitleContext* SubtitleContext_Init ( FileContext* apFileCtx, SubtitleFormat aFmt, float aFPS, int aBase, int aRatio ) {

 int              lSts  = 0;
 SubtitleContext* lpCtx = &s_SubCtx;
 int              lf16  = g_Config.m_ColorDepth;

 lpCtx -> m_pHead  = NULL;
 lpCtx -> m_pTail  = NULL;
 lpCtx -> m_nAlloc =    0;

 lpCtx -> m_ErrorCode = 0;
 lpCtx -> m_ErrorLine = 0;

 if ( g_Config.m_MBFName[ 0 ] && !g_MBFont ) {

  char lPath[ strlen ( g_pMC0SMS ) + strlen ( g_Config.m_MBFName ) + 6 ];

  GUI_Progress ( STR_LOADING_FONT.m_pStr, 50, 0 );

  strcpy ( lPath, g_pMC0SMS          );
  strcat ( lPath, g_SlashStr         );
  strcat ( lPath, g_Config.m_MBFName );
  strcat ( lPath, g_pExtMBF          );

  if (  !GSFont_Load ( lPath )  ) GUI_Error ( STR_ERROR.m_pStr );

  GUI_Progress ( STR_LOADING_SUBTITLES.m_pStr, 100, 0 );

 }  /* end if */

 lpCtx -> m_nPreAlloc = lf16 ? 144 : 96;

 if ( g_MBFont ) {

  _strtok_func        = _mbstrtok;
  _strlen_func        = _mbstrlen;
  _render_string_func = _mb_render_string;
  _string_width_func  = _mb_string_width;

 } else {

  _strtok_func        = strtok;
  _strlen_func        = strlen;
  _render_string_func = _sb_render_string;
  _string_width_func  = _sb_string_width;

 }  /* end else */

 s_SubCtx.Prepare = _produce_packets;
 s_SubCtx.Display = _display;
 s_SubCtx.Reset   = _reset;

 switch ( aFmt ) {

  case SubtitleFormat_SRT: lSts = _load_srt ( apFileCtx, aBase, aRatio       ); break;
  case SubtitleFormat_SUB: lSts = _load_sub ( apFileCtx, aFPS, aBase, aRatio ); break;

 }  /* end switch */

 if ( !lSts ) _clear_nodes ();

 s_SubCtx.m_Idx    = 0;
 s_SubCtx.m_DMAIdx = 0;
 s_SubCtx.Destroy  = _destroy;

 return lpCtx;

}  /* end SubtitleContext_Init */
