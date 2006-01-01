/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 BraveDog
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SubtitleContext.h"
#include "FileContext.h"
#include "StringList.h"
#include "GS.h"
#include "VIF.h"
#include "IPU.h"
#include "Config.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define SUB_GSP_SIZE( n )  (  ( n << 2 ) + 8  )
#define SUB_GSP_OSIZE( n ) (  ( n << 2 ) + 10  )
#define SUB_MAXLEN         512

typedef struct SubNode {

 struct SubNode* m_pNext;

 StringList* m_pList;
 int64_t     m_Begin;
 int64_t     m_End;

} SubNode;

static SubtitleContext s_SubCtx;
static SubNode*        s_pHead;
static SubNode*        s_pTail;
static unsigned int    s_nAlloc;

static unsigned int ( *_update_size_func ) ( StringList*, unsigned int );

static void sub_gets ( char* apStr, FileContext* apFileCtx ) {

 File_GetString ( apFileCtx, apStr, SUB_MAXLEN );

 ++s_SubCtx.m_ErrorLine;

}  /* end sub_gets */

static unsigned int _update_size ( StringList* apList, unsigned int aLen ) {

 StringListNode* lpNode = apList -> m_pHead;

 aLen += SUB_GSP_SIZE(  strlen ( lpNode -> m_pString )  );

 lpNode = lpNode -> m_pNext;

 while ( lpNode ) {

  aLen += strlen ( lpNode -> m_pString ) << 2;
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 return aLen;

}  /* end _update_size */

static unsigned int _update_osize ( StringList* apList, unsigned int aLen ) {

 StringListNode* lpNode = apList -> m_pHead;

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

static void _add_line ( StringList* apList, char* apLine, unsigned int aColorIdx ) {

 int lScrW = g_GSCtx.m_Width;
 int lTxtW = g_GSCtx.TextWidth ( apLine, 0 );

 if ( lTxtW < lScrW ) {
addLine:
  apList -> PushBack ( apList, apLine );
  apList -> m_pTail -> m_pParam = ( void* )aColorIdx;

 } else {

  char* lpSpace;
next:
  lpSpace = strrchr ( apLine, ' ' );

  while ( 1 ) {

   if ( lpSpace ) {

    lTxtW = g_GSCtx.TextWidth ( apLine, lpSpace - apLine );

    if ( lTxtW < lScrW ) {

     *lpSpace++ = '\x00';
     apList -> PushBack ( apList, apLine );
     apList -> m_pTail -> m_pParam = ( void* )aColorIdx;

     apLine = lpSpace;
     goto next;

    } else lpSpace = strrchrx ( apLine, lpSpace - 1, ' ' );

   } else goto addLine;

  }  /* end while */

 }  /* end else */

}  /* end _add_line */

static int _load_sub ( FileContext* apFileCtx, float aFPS ) {

 char     lLine[ SUB_MAXLEN ];
 float    lMSPerFrame = 1000.0F / aFPS;
 int      retVal      = 0;
 SubNode* lpNode;

 s_SubCtx.m_Cnt = 0;

 while ( 1 ) {

  int64_t lTime [ 2 ];
  int     lFrame[ 2 ];
  char    lBrace[ 4 ];
  int     i;
  char*   lpPtr;

  sub_gets ( lLine, apFileCtx );

  if ( lLine[ 0 ] == '\x00' ) {

   retVal = 1;
   break;

  }  /* end if */

  if (  sscanf (
         lLine, "%c%d%c%c%d%c",
         &lBrace[ 0 ], &lFrame[ 0 ], &lBrace[ 1 ],
         &lBrace[ 2 ], &lFrame[ 1 ], &lBrace[ 3 ]
        ) != 6 || lBrace[ 0 ] != '{' || lBrace[ 1 ] != '}'
               || lBrace[ 2 ] != '{' || lBrace[ 3 ] != '}'
  ) {

   s_SubCtx.m_ErrorCode = SubtitleError_Format;
   break;

  }  /* end  if */

  for ( i = 0; i < 2; ++i ) lTime[ i ] = ( int64_t )(  ( float )lFrame[ i ] * lMSPerFrame + 0.5F  );

  lpNode = ( SubNode* )malloc (  sizeof ( SubNode )  );
  lpNode -> m_pList = StringList_Init ();
  lpNode -> m_pNext = NULL;

  if ( !s_pHead )

   s_pHead = s_pTail = lpNode;

  else {

   s_pTail -> m_pNext = lpNode;
   s_pTail            = lpNode;

  }  /* end else */

  lpPtr = strchr ( lLine, '}' ) + 1;
  lpPtr = strchr ( lpPtr, '}' ) + 1;
  lpPtr = strtok ( lpPtr, "|" );

  while ( lpPtr ) {
// skip tag (I didn't find any clear specs about them)
   if ( *lpPtr == '{' ) {

    while ( *lpPtr != '}' ) ++lpPtr;

    ++lpPtr;

   }  /* end if */

   _add_line ( lpNode -> m_pList, lpPtr, 0 );

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

 char lLine[ SUB_MAXLEN ];
 int  lPrevSerial;
 int  retVal = 0;

 sub_gets ( lLine, apFileCtx );

 if (  !sscanf ( lLine, "%d", &lPrevSerial )  ) {
seqError:
  s_SubCtx.m_ErrorCode = SubtitleError_Sequence;
  return retVal;

 }  /* end if */

 --lPrevSerial;

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

    s_SubCtx.m_Cnt = lSerial;
    retVal         = 1;

    break;

   }  /* end if */

   continue;

  }  /* end if */

  if (  !sscanf ( lLine, "%d", &lSerial ) ||
        lSerial != lPrevSerial + 1
  ) goto seqError;

  sub_gets ( lLine, apFileCtx );  /* timestamps */

  if (  sscanf (
         lLine, "%u:%u:%u,%u", &lH[ 0 ], &lM[ 0 ], &lS[ 0 ], &lMS[ 0 ]
        ) != 4
  ) {

   s_SubCtx.m_ErrorCode = SubtitleError_Format;
   break;

  }  /* end if */

  lpPtr = strstr ( lLine, " --> " );

  if ( !lpPtr ) {

   s_SubCtx.m_ErrorCode = SubtitleError_Format;
   break;

  }  /* end if */

  if (  sscanf (
         lpPtr + 5, "%u:%u:%u,%u", &lH[ 1 ], &lM[ 1 ], &lS[ 1 ], &lMS[ 1 ]
        ) != 4
  ) {

   s_SubCtx.m_ErrorCode = SubtitleError_Format;
   break;

  }  /* end if */

  for ( i = 0; i < 2; ++i ) lTime[ i ] = lH[ i ] * 3600000 + lM[ i ] * 60000 + lS[ i ] * 1000 + lMS[ i ];

  lpNode = ( SubNode* )malloc (  sizeof ( SubNode )  );
  lpNode -> m_pList = StringList_Init ();
  lpNode -> m_pNext = NULL;

  if ( !s_pHead )

   s_pHead = s_pTail = lpNode;

  else {

   s_pTail -> m_pNext = lpNode;
   s_pTail            = lpNode;

  }  /* end else */

  while ( 1 ) {

   sub_gets ( lLine, apFileCtx );  /* actual subtitle strings */

   if (  lLine[ 0 ] == '\x00' || _blank_line ( lLine )  ) {

    if (  FILE_EOF( apFileCtx )  ) {

     s_SubCtx.m_Cnt = lSerial;
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

    _add_line ( lpNode -> m_pList, lpPtr, lColorIdx );

   }  /* end else */

  }  /* end while */

  if ( !lpNode -> m_pList -> m_Size ) _add_line ( lpNode -> m_pList, " ", lColorIdx );

  s_nAlloc = _update_size_func ( lpNode -> m_pList, s_nAlloc );

  lpNode -> m_Begin = lTime[ 0 ];
  lpNode -> m_End   = lTime[ 1 ];

  if ( retVal ) break;

  lPrevSerial = lSerial;

 }  /* end while */

 return retVal;

}  /* end _load_srt */

static void _clear_nodes ( void ) {

 SubNode* lpNode = s_pHead;

 while ( lpNode ) {

  SubNode* lpNext = lpNode -> m_pNext;

  lpNode -> m_pList -> Destroy ( lpNode -> m_pList, 1 );
  free ( lpNode );

  lpNode = lpNext;

 }  /* end while */

 s_pHead = s_pTail = NULL;

}  /* end _clear_nodes */

static void _produce_packets ( void ) {

 unsigned int i;
 uint64_t*    lpDMA;
 SubNode*     lpNode = s_pHead;
 unsigned int lXIncr = g_GSCtx.m_OffsetX << 4;
 unsigned int lYIncr = g_GSCtx.m_OffsetY << 4;

 s_SubCtx.m_pDMA     = ( uint64_t*       )calloc (  s_nAlloc,       sizeof ( uint64_t       )  );
 s_SubCtx.m_pPackets = ( SubtitlePacket* )calloc (  s_SubCtx.m_Cnt, sizeof ( SubtitlePacket )  );

 lpDMA = s_SubCtx.m_pDMA;

 for ( i = 0; i < s_SubCtx.m_Cnt; ++i, lpNode = lpNode -> m_pNext ) {

  unsigned int    j, k     = 8;
  StringListNode* lpSNode  = lpNode -> m_pList -> m_pHead;
  SubtitlePacket* lpPacket = &s_SubCtx.m_pPackets[ i ];
  unsigned int    lY       = g_GSCtx.m_Height - g_Config.m_PlayerSubOffset - 32 * lpNode -> m_pList -> m_Size;
  unsigned int    lLen     = strlen ( lpSNode -> m_pString );
  unsigned int    lCumLen  = lLen;

  lpPacket -> m_Begin = lpNode -> m_Begin;
  lpPacket -> m_End   = lpNode -> m_End;
  lpPacket -> m_pDMA  = lpDMA;
  lpPacket -> m_QWC   = SUB_GSP_SIZE( lLen ) >> 1;

  lpDMA[ 0 ] = 0;
  lpDMA[ 2 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
  lpDMA[ 3 ] = ( GS_TEX0_1 + !g_GSCtx.m_PrimCtx ) | ( GS_PRIM << 4 );
  lpDMA[ 4 ] = GS_SETREG_TEX0( g_GSCtx.m_Font.m_Text, 16, GSPSM_4, 10, 8, 1, 1, g_GSCtx.m_Font.m_CLUT[ ( unsigned int )lpSNode -> m_pParam ], 0, 0, 0, 1 );
  lpDMA[ 5 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, !g_GSCtx.m_PrimCtx, 0 );
  lpDMA[ 7 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

  while ( 1 ) {

   unsigned int lX[ 32 ];
   unsigned int lX1, lX2;
   unsigned int lY1, lY2;
   unsigned int lU1, lU2;
   unsigned int lV1, lV2;
   unsigned int lTX, lTY;
   int          lCurX;

   if ( !g_Config.m_PlayerSAlign ) {

    lX1 = (  g_GSCtx.m_Width - g_GSCtx.TextWidth ( lpSNode -> m_pString, lLen )  ) >> 1;

    if ( lX1 > g_GSCtx.m_Width ) lX1 = 0;

   } else lX1 = 32;

   lY1 = ( lY << 3 ) + lYIncr;
   lY2 = (  ( lY + 32 ) << 3  ) + lYIncr;

   for ( j = 0; j < 32; ++j ) lX[ j ] = lX1;

   for ( j = 0; j < lLen; ++j, k += 4 ) {

    unsigned int  l;
    unsigned char lChr = lpSNode -> m_pString[ j ] - ' ';

    lCurX = -INT_MAX;

    for ( l = 0; l < 32; ++l ) {

     int lOffset = lX[ l ] - g_Kerns[ lChr ].m_Kern[ l ].m_Left;

     if ( lOffset > lCurX ) lCurX = lOffset;

    }  /* end for */

    lX1  = ( lCurX << 4 ) + lXIncr;
    lX2  = (  ( lCurX + 32 ) << 4  ) + lXIncr;

    for ( l = 0; l < 32; ++l ) lX[ l ] = lCurX + 32 - g_Kerns[ lChr ].m_Kern[ l ].m_Right;

    lTY = 1;

    while ( lChr > 30 ) {

     lChr -= 31;
     lTY  += 32;

    }  /* end while */

    lTX = lChr * 32;

    lU1 = ( lTX << 4 ) + lXIncr;
    lU2 = (  ( lTX + 32 ) << 4  ) + lXIncr;

    lV1 = ( lTY << 4 ) + lXIncr;
    lV2 = (  ( lTY + 32 ) << 4  ) + lXIncr;

    lpDMA[ k + 0 ] = GS_SETREG_UV( lU1, lV1 );
    lpDMA[ k + 1 ] = GS_SETREG_XYZ( lX1, lY1, 0 );
    lpDMA[ k + 2 ] = GS_SETREG_UV( lU2, lV2 );
    lpDMA[ k + 3 ] = GS_SETREG_XYZ( lX2, lY2, 0 );

   }  /* end for */

   if (  !( lpSNode = lpSNode -> m_pNext )  ) break;

   lLen               = strlen ( lpSNode -> m_pString );
   lpPacket -> m_QWC += lLen << 1;
   lY                += 32;
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
 unsigned int lXIncr = g_GSCtx.m_OffsetX << 4;
 unsigned int lYIncr = g_GSCtx.m_OffsetY << 4;

 s_SubCtx.m_pDMA     = ( uint64_t*       )calloc (  s_nAlloc,       sizeof ( uint64_t       )  );
 s_SubCtx.m_pPackets = ( SubtitlePacket* )calloc (  s_SubCtx.m_Cnt, sizeof ( SubtitlePacket )  );

 lpDMA = s_SubCtx.m_pDMA;

 for ( i = 0; i < s_SubCtx.m_Cnt; ++i, lpNode = lpNode -> m_pNext ) {

  unsigned int    j, k;
  StringListNode* lpSNode  = lpNode -> m_pList -> m_pHead;
  SubtitlePacket* lpPacket = &s_SubCtx.m_pPackets[ i ];
  unsigned int    lY       = g_GSCtx.m_Height - g_Config.m_PlayerSubOffset - 32 * lpNode -> m_pList -> m_Size;
  unsigned int    lLen     = strlen ( lpSNode -> m_pString );
  uint64_t*       lpVIF    = &lpDMA[ 1 ];

  lpPacket -> m_Begin = lpNode -> m_Begin;
  lpPacket -> m_End   = lpNode -> m_End;
  lpPacket -> m_pDMA  = lpDMA;
  lpPacket -> m_QWC   = (  6 + SUB_GSP_OSIZE( lLen )  ) >> 1;

  lpDMA[ 0 ] = 0;
  lpDMA[ 2 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
  lpDMA[ 3 ] = GIF_AD;
  lpDMA[ 4 ] = GS_SETREG_TEX1( 0, 0, 1, 1, 0, 0, 0 );
  lpDMA[ 5 ] = GS_TEX1_1 + !g_GSCtx.m_PrimCtx;
  lpDMA     += 6;

  while ( 1 ) {

   unsigned int lX[ 32 ];
   unsigned int lX1, lX2;
   unsigned int lY1, lY2;
   unsigned int lU1, lU2;
   unsigned int lV1, lV2;
   unsigned int lTX, lTY;
   int          lCurX;
   unsigned int lTxtWidth = g_GSCtx.TextWidth ( lpSNode -> m_pString, lLen );

   if ( !g_Config.m_PlayerSAlign ) {

    lX1 = (  g_GSCtx.m_Width - lTxtWidth  ) >> 1;

    if ( lX1 > g_GSCtx.m_Width ) lX1 = 0;

   } else lX1 = 32;

   lY1 = ( lY << 3 ) + lYIncr;
   lY2 = (  ( lY + 32 ) << 3  ) + lYIncr;

   for ( j = 0; j < 32; ++j ) lX[ j ] = lX1;

   lpDMA[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 6 );
   lpDMA[ 1 ] = GS_PRIM | ( GS_RGBAQ <<  4 )                            |
                          ( GS_XYZ2  <<  8 )                            |
                          ( GS_XYZ2  << 12 )                            |
                          (  ( GS_TEX0_1 + !g_GSCtx.m_PrimCtx ) << 16 ) |
                          ( GS_PRIM << 20 );
   lpDMA[ 2 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, !g_GSCtx.m_PrimCtx, 0 );
   lpDMA[ 3 ] = GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 );
   lpDMA[ 4 ] = GS_SETREG_XYZ(  ( lX1 << 4 ) + lXIncr, lY1, 0  );
   lpDMA[ 5 ] = GS_SETREG_XYZ(   (  ( lX1 + lTxtWidth ) << 4  ) + lXIncr, lY2, 0  );
   lpDMA[ 6 ] = GS_SETREG_TEX0( g_GSCtx.m_Font.m_Text, 16, GSPSM_4, 10, 8, 1, 1, g_GSCtx.m_Font.m_CLUT[ ( unsigned int )lpSNode -> m_pParam ], 0, 0, 0, 1 );
   lpDMA[ 7 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, !g_GSCtx.m_PrimCtx, 0 );
   lpDMA[ 8 ] = GIF_TAG( lLen, 1, 0, 0, 1, 4 );
   lpDMA[ 9 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

   for ( j = 0, k = 10; j < lLen; ++j, k += 4 ) {

    unsigned int  l;
    unsigned char lChr = lpSNode -> m_pString[ j ] - ' ';

    lCurX = -INT_MAX;

    for ( l = 0; l < 32; ++l ) {

     int lOffset = lX[ l ] - g_Kerns[ lChr ].m_Kern[ l ].m_Left;

     if ( lOffset > lCurX ) lCurX = lOffset;

    }  /* end for */

    lX1  = ( lCurX << 4 ) + lXIncr;
    lX2  = (  ( lCurX + 32 ) << 4  ) + lXIncr;

    for ( l = 0; l < 32; ++l ) lX[ l ] = lCurX + 32 - g_Kerns[ lChr ].m_Kern[ l ].m_Right;

    lTY = 1;

    while ( lChr > 30 ) {

     lChr -= 31;
     lTY  += 32;

    }  /* end while */

    lTX = lChr * 32;

    lU1 = ( lTX << 4 ) + lXIncr;
    lU2 = (  ( lTX + 32 ) << 4  ) + lXIncr;

    lV1 = ( lTY << 4 ) + lXIncr;
    lV2 = (  ( lTY + 32 ) << 4  ) + lXIncr;

    lpDMA[ k + 0 ] = GS_SETREG_UV( lU1, lV1 );
    lpDMA[ k + 1 ] = GS_SETREG_XYZ( lX1, lY1, 0 );
    lpDMA[ k + 2 ] = GS_SETREG_UV( lU2, lV2 );
    lpDMA[ k + 3 ] = GS_SETREG_XYZ( lX2, lY2, 0 );

   }  /* end for */

   lpDMA += SUB_GSP_OSIZE( lLen );

   if (  !( lpSNode = lpSNode -> m_pNext )  ) break;

   lLen               = strlen ( lpSNode -> m_pString );
   lpPacket -> m_QWC += SUB_GSP_OSIZE( lLen ) >> 1;
   lY                += 32;

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
