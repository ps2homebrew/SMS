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
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_SubtitleContext.h"
#include "SMS_FileContext.h"
#include "SMS_List.h"
#include "SMS_GS.h"
#include "SMS_VIF.h"
#include "SMS_IPU.h"
#include "SMS_Config.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define SUB_GSP_SIZE( n )  (  ( n << 2 ) + 8  )
#define SUB_GSP_OSIZE( n ) (  ( n << 2 ) + 10  )
#define SUB_MAXLEN         512
#define SUB_DEF_LINE_LEN   4.0F

extern int TranslateUTF8 ( int, char*, int, const char* );

typedef struct SubNode {

 struct SubNode* m_pNext;

 SMS_List* m_pList;
 int64_t   m_Begin;
 int64_t   m_End;

} SubNode;

static SubtitleContext s_SubCtx;
static SubNode*        s_pHead;
static SubNode*        s_pTail;
static unsigned int    s_nAlloc;

static unsigned int ( *_update_size_func ) ( SMS_List*, unsigned int );

static void sub_gets ( char* apStr, FileContext* apFileCtx ) {

 File_GetString ( apFileCtx, apStr, SUB_MAXLEN );

 ++s_SubCtx.m_ErrorLine;

}  /* end sub_gets */

static unsigned int _update_size ( SMS_List* apList, unsigned int aLen ) {

 SMS_ListNode* lpNode = apList -> m_pHead;

 aLen += SUB_GSP_SIZE(  strlen ( lpNode -> m_pString )  );

 lpNode = lpNode -> m_pNext;

 while ( lpNode ) {

  aLen += strlen ( lpNode -> m_pString ) << 2;
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 return aLen;

}  /* end _update_size */

static unsigned int _update_osize ( SMS_List* apList, unsigned int aLen ) {

 SMS_ListNode* lpNode = apList -> m_pHead;

 aLen += 6;

 while ( lpNode ) {

  aLen += SUB_GSP_OSIZE(  strlen ( lpNode -> m_pString )  );
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 return aLen;

}  /* end _update_osize */

static char* strrchrx ( const char* apLine, char* apPos, int aChr ) {

 char* retVal = NULL;

 while ( apPos >= apLine ) {

  if ( *apPos == aChr ) {

   retVal = apPos;
   break;

  }  /* end if */

  --apPos;

 }  /* end while */

 return retVal;

}  /* end strrchrx */

static void _add_line ( SMS_List* apList, char* apLine, unsigned int aColorIdx ) {

 int lScrW = g_GSCtx.m_Width;
 int lDW   = g_Config.m_SubHIncr;
 int lLen;
 int lTxtW;
next:
 lLen  = strlen ( apLine );
 lTxtW = GSFont_WidthEx ( apLine, lLen, lDW );

 if ( lTxtW < lScrW ) {

  if ( g_Config.m_PlayerSAlign == 2 ) SMS_ReverseString ( apLine, lLen );

  SMS_ListPushBack ( apList, apLine );
  apList -> m_pTail -> m_Param = ( unsigned int )( void* )aColorIdx;

 } else {

  char* lpSpace;

  lpSpace = strrchr ( apLine, ' ' );

  while ( 1 ) {

   if ( lpSpace ) {

    lTxtW = GSFont_WidthEx ( apLine, lpSpace - apLine, lDW );

    if ( lTxtW < lScrW ) {

     *lpSpace = '\x00';

     if ( g_Config.m_PlayerSAlign == 2 ) {

      int lX, lLen = strlen ( apLine );

      SMS_ReverseString ( apLine, lpSpace - apLine );

      while ( 1 ) {

       lX = g_GSCtx.m_Width - GSFont_WidthEx ( apLine, lLen, g_Config.m_SubHIncr ) - 32;

       if ( lX >= 0 ) break;

       ++apLine;
       --lLen;

      }  /* end while */

     }  /* end if */

     SMS_ListPushBack ( apList, apLine );
     apList -> m_pTail -> m_Param = ( unsigned int )( void* )aColorIdx;

     apLine = ++lpSpace;
     goto next;

    } else lpSpace = strrchrx ( apLine, lpSpace - 1, ' ' );

   } else {

    if ( g_Config.m_PlayerSAlign == 2 )
     ++apLine;
    else apLine[ lLen - 1 ] = '\x00';

    goto next;

   }  /* end else */

  }  /* end while */

 }  /* end else */

}  /* end _add_line */

static int _load_sub ( FileContext* apFileCtx, float aFPS ) {

 char     lLine[ SUB_MAXLEN ];
 float    lMSPerFrame = 1000.0F / aFPS;
 int      retVal      = 0;
 int      lfUTF8      = 0;
 unsigned lPos        = apFileCtx -> m_CurPos;
 SubNode* lpNode;

 if (  File_GetByte ( apFileCtx ) == 0xEF &&
       File_GetByte ( apFileCtx ) == 0xBB &&
       File_GetByte ( apFileCtx ) == 0xBF
 )

  lfUTF8 = 1;

 else apFileCtx -> Seek ( apFileCtx, lPos );

 s_SubCtx.m_Cnt = 0;

 while ( 1 ) {

  int64_t lTime [ 2 ];
  int     lFrame[ 2 ];
  int     i;
  int     lHour   = 0;
  int     lMinute = 0;
  int     lSecond = 0;
  char*   lpPtr;

  lFrame[ 1 ] = 0;

  sub_gets ( lLine, apFileCtx );

  if ( lLine[ 0 ] == '\x00' ) {

   retVal = 1;
   break;

  }  /* end if */

  if (   (  sscanf ( lLine, "{%d}{%d}", &lFrame[ 0 ], &lFrame[ 1 ] ) == 2  )   ) {

   if ( lFrame[ 0 ] == 0 ) continue;
   if ( lFrame[ 1 ] == 0 ) lFrame[ 1 ] = lFrame[ 0 ] + ( int )(  ( SUB_DEF_LINE_LEN * aFPS ) + 0.5F  );

   for ( i = 0; i < 2; ++i ) lTime[ i ] = ( int64_t )(  ( float )lFrame[ i ] * lMSPerFrame + 0.5F  );

   lpPtr = strchr ( lLine, '}' ) + 1;
   lpPtr = strchr ( lpPtr, '}' ) + 1;

  } else if (   (  sscanf ( lLine, "[%d][%d]", &lFrame[ 0 ], &lFrame[ 1 ] ) == 2  )   ) {

   if ( lFrame[ 0 ] == 0 ) continue;
   if ( lFrame[ 1 ] == 0 ) lFrame[ 1 ] = lFrame[ 0 ] + ( int )( SUB_DEF_LINE_LEN * 10.0F );

   for ( i = 0; i < 2; ++i ) lTime[ i ] = ( int64_t )( lFrame[ i ] * 100  );

   lpPtr = strchr ( lLine, ']' ) + 1;
   lpPtr = strchr ( lpPtr, ']' ) + 1;

  } else if (   (  sscanf ( lLine, "%d:%d:%d:", &lHour, &lMinute, &lSecond ) == 3  )   ) {

   lTime[ 0 ] = ( int64_t )(   ( lSecond + ( lMinute * 60 ) + ( lHour * 3600 )  ) * 1000   );
   lTime[ 1 ] = lTime[ 0 ] + ( int64_t )( SUB_DEF_LINE_LEN * 1000.0F );

   lpPtr = strchr ( lLine, ':' ) + 1;
   lpPtr = strchr ( lpPtr, ':' ) + 1;
   lpPtr = strchr ( lpPtr, ':' ) + 1;

  } else {

   s_SubCtx.m_ErrorCode = SubtitleError_Format;
   break;

  }  /* end else */

  lpNode = ( SubNode* )malloc (  sizeof ( SubNode )  );
  lpNode -> m_pList = SMS_ListInit ();
  lpNode -> m_pNext = NULL;

  if ( !s_pHead )

   s_pHead = s_pTail = lpNode;

  else {

   s_pTail -> m_pNext = lpNode;
   s_pTail            = lpNode;

  }  /* end else */

  lpPtr = strtok ( lpPtr, "|" );

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

   lpPtr = strtok ( NULL, "|" );

  }  /* end while */

  if ( !lpNode -> m_pList -> m_Size ) {

   s_SubCtx.m_ErrorCode = SubtitleError_Format;
   break;

  } else s_nAlloc = _update_size_func ( lpNode -> m_pList, s_nAlloc );

  lpNode -> m_Begin = lTime[ 0 ];
  lpNode -> m_End   = lTime[ 1 ];

  ++s_SubCtx.m_Cnt;

 }  /* end while */

 return retVal;

}  /* end _load_sub */

static int _blank_line ( char* apStr ) {

 while ( *apStr ) if (  !isspace ( *apStr++ )  ) return 0;

 return 1;

}  /* end _blank_line */

static int _load_srt ( FileContext* apFileCtx ) {

 char     lLine[ SUB_MAXLEN ];
 int      lTotSerial;
 int      retVal = 0;
 unsigned lPos   = apFileCtx -> m_CurPos;
 int      lfUTF8 = 0;

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

  if ( !s_pHead )

   s_pHead = s_pTail = lpNode;

  else {

   s_pTail -> m_pNext = lpNode;
   s_pTail            = lpNode;

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

  s_nAlloc = _update_size_func ( lpNode -> m_pList, s_nAlloc );

  lpNode -> m_Begin = lTime[ 0 ];
  lpNode -> m_End   = lTime[ 1 ];

  if ( retVal ) break;

 }  /* end while */

 return retVal;

}  /* end _load_srt */

static void _clear_nodes ( void ) {

 SubNode* lpNode = s_pHead;

 while ( lpNode ) {

  SubNode* lpNext = lpNode -> m_pNext;

  SMS_ListDestroy ( lpNode -> m_pList, 1 );
  free ( lpNode );

  lpNode = lpNext;

 }  /* end while */

 s_pHead = s_pTail = NULL;

}  /* end _clear_nodes */

static void _produce_packets ( void ) {

 unsigned int i;
 uint64_t*    lpDMA;
 SubNode*     lpNode = s_pHead;
 int          lDW    = g_Config.m_SubHIncr << 4;
 float        lAR    = ( 32.0F + g_Config.m_SubHIncr ) / 32.0F;
 int          lDH    = g_Config.m_SubVIncr;
 int          lChrH  = 32 + lDH;

 s_SubCtx.m_pDMA     = ( uint64_t*       )calloc (  s_nAlloc,       sizeof ( uint64_t       )  );
 s_SubCtx.m_pPackets = ( SubtitlePacket* )calloc (  s_SubCtx.m_Cnt, sizeof ( SubtitlePacket )  );

 lpDMA = s_SubCtx.m_pDMA;

 for ( i = 0; i < s_SubCtx.m_Cnt; ++i, lpNode = lpNode -> m_pNext ) {

  unsigned int    j, k     = 8;
  SMS_ListNode*   lpSNode  = lpNode -> m_pList -> m_pHead;
  SubtitlePacket* lpPacket = &s_SubCtx.m_pPackets[ i ];
  unsigned int    lY       = g_GSCtx.m_Height - lChrH * ( lpNode -> m_pList -> m_Size - 1 ) - g_Config.m_PlayerSubOffset;
  unsigned int    lLen     = strlen ( lpSNode -> m_pString );
  unsigned int    lCumLen  = lLen;

  lpPacket -> m_Begin = lpNode -> m_Begin;
  lpPacket -> m_End   = lpNode -> m_pNext ? ( lpNode -> m_End > lpNode -> m_pNext -> m_Begin ? lpNode -> m_pNext -> m_Begin - 1 : lpNode -> m_End ) : lpNode -> m_End;
  lpPacket -> m_pDMA  = lpDMA;
  lpPacket -> m_QWC   = SUB_GSP_SIZE( lLen ) >> 1;

  lpDMA[ 0 ] = 0;
  lpDMA[ 2 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
  lpDMA[ 3 ] = GS_TEX0_1 | ( GS_PRIM << 4 );
  lpDMA[ 4 ] = GS_SET_TEX0( g_GSCtx.m_VRAMFontPtr, 8, GSPixelFormat_PSMT4, 9, 9, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, g_GSCtx.m_CLUT[ ( unsigned int )lpSNode -> m_Param ], GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, 0, GS_TEX_CLD_LOAD );
  lpDMA[ 5 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, 0, 0 );
  lpDMA[ 7 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

  while ( 1 ) {

   unsigned int lVX[ 32 ];
   unsigned int lX;
   unsigned int lY1, lY2;
   unsigned int lU, lV;
   unsigned int lTxtWidth = GSFont_WidthEx ( lpSNode -> m_pString, lLen, g_Config.m_SubHIncr );
   int          lCurX;

   switch ( g_Config.m_PlayerSAlign ) {

    default:
    case 0 :  /* center alignment */

     lX = ( g_GSCtx.m_Width - lTxtWidth ) >> 1;

     if ( lX > g_GSCtx.m_Width ) lX = 0;

    break;

    case 1:  /* left alignment */

     lX = 32;

    break;

    case 2:  /* right alignment */

     lX = g_GSCtx.m_Width - lTxtWidth - 32;

    break;

   }  /* end switch */

   __asm__ __volatile__ (
    ".set noreorder\n\t"
    "move     $t9, $ra\n\t"
    "addiu    $v0, %2, 32\n\t"
    "addu     $v0, $v0, %3\n\t"
    "move     $a1, %2\n\t"
    "move     $a0, $zero\n\t"
    "dsll32   $v0, $v0, 0\n\t"
    "move     $a2, $zero\n\t"
    "jal      GS_XYZ\n\t"
    "or       $a1, $a1, $v0\n\t"
    "srl      $v0, $v0, 16\n\t"
    "dsrl32   $a1, $a1, 0\n\t"
    "sll      $v0, $v0, 16\n\t"
    "move     %0, $v0\n\t"
    "move     %1, $a1\n\t"
    "move     $ra, $t9\n\t"
    ".set reorder\n\t"
    : "=r"( lY1 ), "=r"( lY2 ) : "r"( lY ), "r"( lDH ) : "a0", "a1", "a2", "v0", "v1", "t9"
   );

   for ( j = 0; j < 32; ++j ) lVX[ j ] = lX;

   for ( j = 0; j < lLen; ++j, k += 4 ) {

    unsigned int  l;
    unsigned char lChr = lpSNode -> m_pString[ j ] - ' ';

    lCurX = -INT_MAX;

    for ( l = 0; l < 32; ++l ) {

     int lOffset = lVX[ l ] - g_GSCharIndent[ lChr ].m_Left[ l ] * lAR;

     __asm__ __volatile__(
      "pmaxw %0, %1, %2\n\t"
      : "=r"( lCurX ) : "r"( lCurX ), "r"( lOffset )
     );

    }  /* end for */

    lX = lCurX << 4;

    for ( l = 0; l < 32; ++l ) lVX[ l ] = lCurX + ( 31 - g_GSCharIndent[ lChr ].m_Right[ l ] ) * lAR;

    lU = ( lChr & 0x0000000F ) << 9;
    lV = ( lChr & 0xFFFFFFF0 ) << 5;

    lpDMA[ k + 0 ] = GS_SET_UV( lU + 8, lV + 8 );
    lpDMA[ k + 1 ] = lX | lY1;
    lpDMA[ k + 2 ] = GS_SET_UV( lU + 504, lV + 504 );
    lpDMA[ k + 3 ] = ( lX + 512 + lDW ) | lY2;

   }  /* end for */

   if (  !( lpSNode = lpSNode -> m_pNext )  ) break;

   lLen               = strlen ( lpSNode -> m_pString );
   lpPacket -> m_QWC += lLen << 1;
   lY                += 32 + lDH;
   lCumLen           += lLen;

  }  /* end while */

  lpDMA[ 1 ] = VIF_DIRECT( lpPacket -> m_QWC - 1 );
  lpDMA[ 6 ] = GIF_TAG( lCumLen, 1, 0, 0, 1, 4 );

  lpDMA += lpPacket -> m_QWC << 1;

 }  /* end for */

 SyncDCache ( s_SubCtx.m_pDMA, s_SubCtx.m_pDMA + s_nAlloc );

 _clear_nodes ();

}  /* end _produce_packets */

static void _produce_opackets ( void ) {

 unsigned int i;
 uint64_t*    lpDMA;
 SubNode*     lpNode = s_pHead;
 int          lDW    = g_Config.m_SubHIncr << 4;
 float        lAR    = ( 32.0F + g_Config.m_SubHIncr ) / 32.0F;
 int          lDH    = g_Config.m_SubVIncr;
 int          lChrH  = 32 + lDH;

 s_SubCtx.m_pDMA     = ( uint64_t*       )calloc (  s_nAlloc,       sizeof ( uint64_t       )  );
 s_SubCtx.m_pPackets = ( SubtitlePacket* )calloc (  s_SubCtx.m_Cnt, sizeof ( SubtitlePacket )  );

 lpDMA = s_SubCtx.m_pDMA;

 for ( i = 0; i < s_SubCtx.m_Cnt; ++i, lpNode = lpNode -> m_pNext ) {

  unsigned int    j, k;
  SMS_ListNode*   lpSNode  = lpNode -> m_pList -> m_pHead;
  SubtitlePacket* lpPacket = &s_SubCtx.m_pPackets[ i ];
  unsigned int    lY       = g_GSCtx.m_Height - lChrH * ( lpNode -> m_pList -> m_Size - 1 ) - g_Config.m_PlayerSubOffset;
  unsigned int    lLen     = strlen ( lpSNode -> m_pString );
  uint64_t*       lpVIF    = &lpDMA[ 1 ];

  lpPacket -> m_Begin = lpNode -> m_Begin;
  lpPacket -> m_End   = lpNode -> m_pNext ? ( lpNode -> m_End > lpNode -> m_pNext -> m_Begin ? lpNode -> m_pNext -> m_Begin - 1 : lpNode -> m_End ) : lpNode -> m_End;
  lpPacket -> m_pDMA  = lpDMA;
  lpPacket -> m_QWC   = (  6 + SUB_GSP_OSIZE( lLen )  ) >> 1;

  lpDMA[ 0 ] = 0;
  lpDMA[ 2 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
  lpDMA[ 3 ] = GIFTAG_REGS_AD;
  lpDMA[ 4 ] = GS_SET_TEX1( 0, 0, 1, 1, 0, 0, 0 );
  lpDMA[ 5 ] = GS_TEX1_1;
  lpDMA     += 6;

  while ( 1 ) {

   unsigned int lVX[ 32 ];
   unsigned int lX;
   unsigned int lY1, lY2;
   unsigned int lU, lV;
   int          lCurX;
   unsigned int lTxtWidth = GSFont_WidthEx ( lpSNode -> m_pString, lLen, g_Config.m_SubHIncr );

   switch ( g_Config.m_PlayerSAlign ) {

    default:
    case 0 :  /* center alignment */

     lX = (  g_GSCtx.m_Width - lTxtWidth  ) >> 1;

     if ( lX > g_GSCtx.m_Width ) lX = 0;

    break;

    case 1:  /* left alignment */

     lX = 32;

    break;

    case 2:  /* right alignment */

     lX = g_GSCtx.m_Width - lTxtWidth - 32;

    break;

   }  /* end switch */

   __asm__ __volatile__ (
    ".set noreorder\n\t"
    "move     $t9, $ra\n\t"
    "addiu    $v0, %2, 32\n\t"
    "addu     $v0, $v0, %3\n\t"
    "move     $a1, %2\n\t"
    "move     $a0, $zero\n\t"
    "dsll32   $v0, $v0, 0\n\t"
    "move     $a2, $zero\n\t"
    "jal      GS_XYZ\n\t"
    "or       $a1, $a1, $v0\n\t"
    "srl      $v0, $v0, 16\n\t"
    "dsrl32   $a1, $a1, 0\n\t"
    "sll      $v0, $v0, 16\n\t"
    "move     %0, $v0\n\t"
    "move     %1, $a1\n\t"
    "move     $ra, $t9\n\t"
    ".set reorder\n\t"
    : "=r"( lY1 ), "=r"( lY2 ) : "r"( lY ), "r"( lDH ) : "a0", "a1", "a2", "v0", "v1", "t9"
   );

   for ( j = 0; j < 32; ++j ) lVX[ j ] = lX;

   lpDMA[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 6 );
   lpDMA[ 1 ] = GS_PRIM | ( GS_RGBAQ  <<  4 ) |
                          ( GS_XYZ2   <<  8 ) |
                          ( GS_XYZ2   << 12 ) |
                          ( GS_TEX0_1 << 16 ) |
                          ( GS_PRIM   << 20 );
   lpDMA[ 2 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
   lpDMA[ 3 ] = GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x00, 0x00 );
   lpDMA[ 4 ] = ( lX << 4 ) | lY1;
   lpDMA[ 5 ] = (  ( lX + lTxtWidth ) << 4  ) | lY2;
   lpDMA[ 6 ] = GS_SET_TEX0( g_GSCtx.m_VRAMFontPtr, 8, GSPixelFormat_PSMT4, 9, 9, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, g_GSCtx.m_CLUT[ ( unsigned int )lpSNode -> m_Param ], GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, 0, GS_TEX_CLD_LOAD );
   lpDMA[ 7 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, 0, 0 );
   lpDMA[ 8 ] = GIF_TAG( lLen, 1, 0, 0, 1, 4 );
   lpDMA[ 9 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

   for ( j = 0, k = 10; j < lLen; ++j, k += 4 ) {

    unsigned int  l;
    unsigned char lChr = lpSNode -> m_pString[ j ] - ' ';

    lCurX = -INT_MAX;

    for ( l = 0; l < 32; ++l ) {

     int lOffset = lVX[ l ] - g_GSCharIndent[ lChr ].m_Left[ l ] * lAR;

     __asm__ __volatile__(
      "pmaxw %0, %1, %2\n\t"
      : "=r"( lCurX ) : "r"( lCurX ), "r"( lOffset )
     );

    }  /* end for */

    lX = lCurX << 4;

    for ( l = 0; l < 32; ++l ) lVX[ l ] = lCurX + ( 31 - g_GSCharIndent[ lChr ].m_Right[ l ] ) * lAR;

    lU = ( lChr & 0x0000000F ) << 9;
    lV = ( lChr & 0xFFFFFFF0 ) << 5;

    lpDMA[ k + 0 ] = GS_SET_UV( lU + 8, lV + 8 );
    lpDMA[ k + 1 ] = lX | lY1;
    lpDMA[ k + 2 ] = GS_SET_UV( lU + 504, lV + 504 );
    lpDMA[ k + 3 ] = ( lX + 512 + lDW ) | lY2;

   }  /* end for */

   lpDMA += SUB_GSP_OSIZE( lLen );

   if (  !( lpSNode = lpSNode -> m_pNext )  ) break;

   lLen               = strlen ( lpSNode -> m_pString );
   lpPacket -> m_QWC += SUB_GSP_OSIZE( lLen ) >> 1;
   lY                += 32 + lDH;

  }  /* end while */

  *lpVIF = VIF_DIRECT( lpPacket -> m_QWC - 1 );

 }  /* end for */

 SyncDCache ( s_SubCtx.m_pDMA, s_SubCtx.m_pDMA + s_nAlloc );

 _clear_nodes ();

}  /* end _produce_opackets */

static void _display ( int64_t aPTS ) {

 SubtitlePacket* lpPacket = s_SubCtx.m_pPackets;

 while ( s_SubCtx.m_Idx < s_SubCtx.m_Cnt ) {

  if ( aPTS < lpPacket[ s_SubCtx.m_Idx ].m_Begin ) break;

  if ( aPTS >= lpPacket[ s_SubCtx.m_Idx ].m_Begin &&
       aPTS <  lpPacket[ s_SubCtx.m_Idx ].m_End
  ) {

   g_IPUCtx.PQueuePacket (
    lpPacket[ s_SubCtx.m_Idx ].m_QWC, lpPacket[ s_SubCtx.m_Idx ].m_pDMA
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

 if ( s_SubCtx.m_pDMA ) {

  free ( s_SubCtx.m_pDMA );
  s_SubCtx.m_pDMA = NULL;

 }  /* end if */

}  /* end _destroy */

SubtitleContext* SubtitleContext_Init ( FileContext* apFileCtx, SubtitleFormat aFmt, float aFPS ) {

 int lSts = 0;

 s_pHead  = NULL;
 s_pTail  = NULL;
 s_nAlloc =    0;

 s_SubCtx.m_ErrorCode = 0;
 s_SubCtx.m_ErrorLine = 0;

 if ( g_Config.m_PlayerFlags & SMS_PF_OSUB ) {

  _update_size_func = _update_osize;
  s_SubCtx.Prepare  = _produce_opackets;

 } else {

  _update_size_func = _update_size;
  s_SubCtx.Prepare  = _produce_packets;

 }  /* end else */

 switch ( aFmt ) {

  case SubtitleFormat_SRT: lSts = _load_srt ( apFileCtx       ); break;
  case SubtitleFormat_SUB: lSts = _load_sub ( apFileCtx, aFPS ); break;

 }  /* end switch */

 if ( !lSts ) _clear_nodes ();

 s_SubCtx.m_Idx   = 0;
 s_SubCtx.Display = _display;
 s_SubCtx.Destroy = _destroy;

 return &s_SubCtx;

}  /* end SubtitleContext_Init */
