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
#include "FileContext.h"

#include <malloc.h>
#include <string.h>

static unsigned char s_DataGUID[ 16 ] = {
 '\xED', '\x1A', '\xE2', '\xC9', '\x35', '\xE1', '\xF9', '\x44',
 '\xBF', '\x01', '\x8E', '\xCF', '\xDC', '\xE1', '\xBD', '\x7A'
};

typedef struct CDDAFileContext {

 FileContext*   m_pCtx;
 CDDADirectory* m_pNodes;

} CDDAFileContext;

#ifdef _WIN32
# define STRICT
# define WIN32_LEAN_AND_MEAN
# include <windows.h>

# define IOCTL_CDROM_READ_TOC 0x00024000
# define IOCTL_CDROM_RAW_READ 0x0002403E
# define UNCACHED_SEG( p ) ( unsigned char* )( p )

typedef enum TRACK_MODE_TYPE { YellowMode2, XAForm2, CDDA } TRACK_MODE_TYPE;

typedef struct CDDAWin32Private {

 HANDLE     m_hDevice;
 OVERLAPPED m_Overlap;

} CDDAWin32Private;

#define HCDROM( c ) (   ( CDDAWin32Private* )(   (  ( unsigned char* )( c )  ) + sizeof ( CDDAContext )   )    ) -> m_hDevice
#define OVERLP( c ) (   ( CDDAWin32Private* )(   (  ( unsigned char* )( c )  ) + sizeof ( CDDAContext )   )    ) -> m_Overlap
# pragma pack( 1 )
typedef struct TrackData {

 BYTE m_Reserved;
 BYTE m_Control;
 BYTE m_TrackNumber;
 BYTE m_Reserved1;
 BYTE m_Address[ 4 ];

} TrackData;

typedef struct CDROMToc {

 unsigned short m_Length;
 BYTE           m_FirstTrack;
 BYTE           m_LastTrack;
 TrackData      m_TrackData[ 100 ];

} CDROMToc;

typedef struct RAW_READ_INFO {

 LARGE_INTEGER   m_Offset;
 DWORD           m_SectorCount;
 TRACK_MODE_TYPE m_TrackMode;

} RAW_READ_INFO;
# pragma pack()
static unsigned int _start_sector ( CDROMToc* apTOC, unsigned int aTrack ) {

 unsigned int retVal = 0xFFFFFFFF;

 if ( aTrack >= apTOC -> m_FirstTrack && aTrack <= apTOC -> m_LastTrack ) {

  const TrackData* lpTrackData = &apTOC -> m_TrackData[ aTrack - 1 ];

  retVal = ( lpTrackData -> m_Address[ 1 ] * 60L * 75L +
             lpTrackData -> m_Address[ 2 ] * 75L       +
             lpTrackData -> m_Address[ 3 ]
            ) - 150L;

 }  /* end if */

 return retVal;

}  /* end _start_sector */

static unsigned int _end_sector ( CDROMToc* apTOC, unsigned int aTrack ) {

 unsigned int retVal = 0xFFFFFFFF;

 if ( aTrack >= apTOC -> m_FirstTrack && aTrack <= apTOC -> m_LastTrack ) {

  const TrackData* lpTrackData = &apTOC -> m_TrackData[ aTrack ];

  retVal = ( lpTrackData -> m_Address[ 1 ] * 60L * 75L +
             lpTrackData -> m_Address[ 2 ] * 75L       +
             lpTrackData -> m_Address[ 3 ]
            ) - 151L;

 }  /* end if */

 return retVal;

}  /* end _end_sector */

BOOL _is_audio ( CDROMToc* apTOC, unsigned int aTrack ) {

 BOOL retVal = FALSE;

 if ( aTrack >= apTOC -> m_FirstTrack && aTrack <= apTOC -> m_LastTrack )

  retVal = ( apTOC -> m_TrackData[ aTrack - 1 ].m_Control & 4 ) == 0; 

 return retVal;

}  /* end _is_audio */

BOOL __inline _read_sectors ( HANDLE ahDev, unsigned int aStart, unsigned int aCount, unsigned char* apBuf, OVERLAPPED* apOvlp ) {

 RAW_READ_INFO lInfo;
 DWORD         retVal = 0;
 DWORD         lSize  = 2352 * aCount;

 lInfo.m_Offset.QuadPart = (  ( __int64 )aStart  ) * 2048i64;
 lInfo.m_SectorCount     = aCount;
 lInfo.m_TrackMode       = CDDA;

 return DeviceIoControl (
         ahDev, IOCTL_CDROM_RAW_READ, &lInfo, sizeof ( lInfo ),
         apBuf, lSize, &retVal, apOvlp
        );

}  /* end _read_sectors */

DWORD _sync ( HANDLE ahDev, OVERLAPPED* apOvlp ) {

 DWORD retVal = 0;

 GetOverlappedResult ( ahDev, apOvlp, &retVal, TRUE );

 return retVal;

}  /* end _sync */

BOOL _find_signature ( CDDAContext* apCtx ) {

 unsigned char lBuf[ 2352 ];
 BOOL          retVal  = FALSE;
 BOOL          lFound  = FALSE;
 unsigned int  lSector = 0;
 unsigned int  lPos    = 0;
 int           i;

 while ( lSector < 256 ) {

  if (   _read_sectors (  HCDROM( apCtx ), lSector, 1, lBuf, &OVERLP( apCtx )  ) ||
         (   GetLastError () == ERROR_IO_PENDING &&
             _sync (  HCDROM( apCtx ), &OVERLP( apCtx )  ) == 2352
         )
  ) {

   for ( i = 0; i < 2352; ++i )

    if ( lBuf[ i ] == s_DataGUID[ lPos ] ) {

     if ( ++lPos == 16 ) break;

    } else lPos = 0;

   if ( lPos == 16 ) {

    i -= 15;

    if ( i > 0 )

     apCtx -> m_Offset = ( unsigned int )i;

    else {

     apCtx -> m_Offset = 2352 + i;
     --lSector;

    }  /* end else */

    retVal = TRUE;

   }  /* end if */

  } else break;

  if ( retVal ) {

   unsigned int i;

   apCtx -> m_StartSector[ 0 ] += lSector;
   apCtx -> m_EndSector  [ 0 ] += lSector;

   lSector -= 75;

   for ( i = 1; i < apCtx -> m_nTracks; ++i ) {

    apCtx -> m_StartSector[ i ] += lSector;
    apCtx -> m_EndSector  [ i ] += lSector;

   }  /* end for */

   break;

  }  /* end if */

  ++lSector;

 }  /* end while */

 return retVal;

}  /* end _find_signature */
#else  /* PS2 */
# include <kernel.h>
# include <fileio.h>
# include <fileXio_rpc.h>
# include <fcntl.h>
# include "CDDA.h"

# undef UNCACHED_SEG
# define UNCACHED_SEG( p ) ( unsigned char* )( p )

# define btoi( b ) (  ( b ) / 16 * 10 + ( b ) % 16  )

static int _do_open ( const char* apFileName ) {

 return fioOpen ( apFileName, O_RDONLY );

}  /* end _do_open */

static int _do_xopen ( const char* apFileName ) {

 return fileXioOpen ( apFileName, O_RDONLY, 0666 );

}  /* end _do_xopen */

static int _do_seek ( int aFD, int anOffset, int aWhence ) {

 return fioLseek ( aFD, anOffset, aWhence );

}  /* end _do_seek */

static int _do_xseek ( int aFD, int anOffset, int aWhence ) {

 return fileXioLseek ( aFD, anOffset, aWhence );

}  /* end _do_xseek */

static int  ( *IO_Close        ) ( int             ) = fileXioClose;
static void ( *IO_SetBlockMode ) ( int             ) = fileXioSetBlockMode;
static int  ( *IO_LSeek        ) ( int, int, int   ) = _do_xseek;
static int  ( *IO_Read         ) ( int, void*, int ) = (  int (*) ( int, void*, int )  )fileXioRead;
static int  ( *IO_Wait         ) ( int, int*       ) = fileXioWaitAsync;
static int  ( *IO_Open         ) ( const char*     ) = _do_xopen;

void STIO_SetIOMode ( STIOMode aMode ) {

 if ( aMode == STIOMode_Extended ) {

  IO_Close        = fileXioClose;
  IO_SetBlockMode = fileXioSetBlockMode;
  IO_LSeek        = _do_xseek;
  IO_Read         = (  int (*) ( int, void*, int )  )fileXioRead;
  IO_Wait         = fileXioWaitAsync;
  IO_Open         = _do_xopen;

 } else if ( aMode == STIOMode_Ordinary ) {

  IO_Close        = fioClose;
  IO_SetBlockMode = fioSetBlockMode;
  IO_LSeek        = _do_seek;
  IO_Read         = fioRead;
  IO_Wait         = fioSync;
  IO_Open         = _do_open;

 }  /* end if */

}  /* end STIO_SetIOMode */

static int _start_sector ( CDDA_TOC* apTOC, int aTrack ) {

 int retVal = -1;

 if ( apTOC -> m_StartTrack - 1 <= aTrack && aTrack < apTOC -> m_EndTrack ) {

  CDDA_Duration* lpDur = &apTOC -> m_Tracks[ aTrack ].m_Duration;

  retVal = (  btoi( lpDur -> m_Minute ) * 60 * 75 +
              btoi( lpDur -> m_Second ) * 75      +
              btoi( lpDur -> m_Frame  )
           ) - 150;

 }  // end if

 return retVal;

}  // end _start_sector

static int _end_sector ( CDDA_TOC* apTOC, int aTrack ) {

 int retVal = -1;

 if ( apTOC -> m_StartTrack - 1 <= aTrack++ ) {

  CDDA_Duration* lpDur;

  if ( aTrack < apTOC -> m_EndTrack )

   lpDur = &apTOC -> m_Tracks[ aTrack ].m_Duration;

  else if ( aTrack == apTOC -> m_EndTrack )

   lpDur = &apTOC -> m_DiskDuration;

  else return retVal;

  retVal = (  btoi( lpDur -> m_Minute ) * 60 * 75 +
              btoi( lpDur -> m_Second ) * 75      +
              btoi( lpDur -> m_Frame  )
           ) - 151;

 }  // end if

 return retVal;

}  // end _end_sector

static inline int _read_sectors ( int aStart, int aCount, u8* apBuf ) {

 return CDDA_RawRead ( aStart, aCount, apBuf );

}  /* end _read_sectors */

static int _find_signature ( CDDAContext* apCtx ) {

 u8  lBuf[ 2352 * 512 ];
 int retVal = 0;
 int lPos   = 0;
 int i;

 if (  _read_sectors ( 0, 512, lBuf ) && CDDA_Synchronize ()  ) {

  u8* lpBuf   = lBuf;
  int lSector = 0;

  while ( lSector < 256 ) {

   for ( i = 0; i < 2352; ++i )

    if ( *lpBuf++ == s_DataGUID[ lPos ] ) {

     if ( ++lPos == 16 ) break;

    } else lPos = 0;

   if ( lPos == 16 ) {

    i -= 15;

    if ( i > 0 ) 

     apCtx -> m_Offset = ( unsigned int )i;

    else {

     apCtx -> m_Offset = 2352 + i;
     --lSector;

    }  /* end else */

    retVal = 1;

   }  /* end if */

   if ( retVal ) {

    unsigned int i;

    apCtx -> m_StartSector[ 0 ] += lSector;
    apCtx -> m_EndSector  [ 0 ] += lSector;

    lSector -= 75;

    for ( i = 1; i < apCtx -> m_nTracks; ++i ) {

     apCtx -> m_StartSector[ i ] += lSector;
     apCtx -> m_EndSector  [ i ] += lSector;

    }  /* end for */

    break;

   }  /* end if */

   ++lSector;

  }  /* end while */

 }  /* end if */

 return retVal;

}  /* end _find_signature */
#endif  /* _WIN32 */
typedef struct CDDAFilePrivate {

 int          m_StartSector;
 int          m_EndSector;
 int          m_CurrentSector;
 int          m_ImgIdx;
 int          m_nSectors;
 int          m_nSectorsRead;
 int          m_Offset;
 CDDAContext* m_pCtx;

} CDDAFilePrivate;

static int CDDA_Sync ( CDDAContext* apCtx ) {
#ifdef _WIN32
 return _sync (  HCDROM( apCtx ), &OVERLP( apCtx )  ) != 0;
#else  /* PS2 */
 return CDDA_Synchronize ();
#endif  /* _WIN32 */
}  /* end CDDA_Sync */

static int CDDA_ReadSectors ( CDDAContext* apCtx, int aStartSector, int aCount, unsigned char* apBuf ) {
#ifdef _WIN32
 return _read_sectors (  HCDROM( apCtx ), aStartSector, aCount, apBuf, &OVERLP( apCtx )  ) || GetLastError () == ERROR_IO_PENDING;
#else  /* PS2 */
 return _read_sectors ( aStartSector, aCount, apBuf );
#endif  /* _WIN32 */
}  /* end if */

static int CDDA_Read ( FileContext* apCtx, void* apBuf, unsigned int aSize ) {

 unsigned int lLen, lSize = aSize;

 while ( aSize > 0 ) {

  lLen = apCtx -> m_pEnd - apCtx -> m_pPos;

  if ( lLen > aSize ) lLen = aSize;

  if ( lLen == 0 ) {

   apCtx -> Fill ( apCtx );

   lLen = apCtx -> m_pEnd - apCtx -> m_pPos;

   if ( lLen == 0 ) break;

  } else {

   memcpy ( apBuf, apCtx -> m_pPos, lLen );

   (  ( char* )apBuf  ) += lLen;
   apCtx -> m_pPos      += lLen;
   aSize                -= lLen;

  }  /* end else */

 }  /* end while */

 lLen               = lSize - aSize;
 apCtx -> m_CurPos += lLen;

 return lLen;

}  /* end CDDA_Read */

static int CDDA_Fill ( FileContext* apCtx ) {

 CDDAFilePrivate* lpPriv = ( CDDAFilePrivate* )apCtx -> m_pData;
 int              retVal = 0;

 if ( apCtx -> m_Pos < apCtx -> m_Size &&
      CDDA_ReadSectors (
       lpPriv -> m_pCtx, lpPriv -> m_CurrentSector, 1, apCtx -> m_pBuff[ apCtx -> m_CurBuf ]
      ) && CDDA_Sync ( lpPriv -> m_pCtx )
 ) {

  int lNextSector = lpPriv -> m_CurrentSector + 1;

  if ( lNextSector >= lpPriv -> m_EndSector ) {

   apCtx -> m_Pos = apCtx -> m_Size;

   if ( lpPriv -> m_CurrentSector == lpPriv -> m_StartSector ) {

    apCtx -> m_pPos = apCtx -> m_pBuff[ apCtx -> m_CurBuf ] + lpPriv -> m_pCtx -> m_Offset;
    apCtx -> m_pEnd = apCtx -> m_pPos + apCtx -> m_Size;

   } else {

    apCtx -> m_pPos = apCtx -> m_pBuff[ apCtx -> m_CurBuf ];
    apCtx -> m_pEnd = apCtx -> m_pPos + ( apCtx -> m_Size + lpPriv -> m_pCtx -> m_Offset ) % 2352;

   }  /* end else */

  } else {

   if ( lpPriv -> m_CurrentSector == lpPriv -> m_StartSector ) {

    apCtx -> m_pPos = apCtx -> m_pBuff[ apCtx -> m_CurBuf ] + lpPriv -> m_pCtx -> m_Offset;
    apCtx -> m_pEnd = apCtx -> m_pBuff[ apCtx -> m_CurBuf ] + 2352;
    apCtx -> m_Pos  = 2352 - lpPriv -> m_pCtx -> m_Offset;

   } else {

    apCtx -> m_pPos = apCtx -> m_pBuff[ apCtx -> m_CurBuf ];
    apCtx -> m_pEnd = apCtx -> m_pBuff[ apCtx -> m_CurBuf ] + 2352;
    apCtx -> m_Pos += 2352;

   }  /* end else */

  }  /* end else */

  lpPriv -> m_CurrentSector = lNextSector;

  retVal = 1;

 } else apCtx -> m_pPos = apCtx -> m_pEnd;

 return retVal;

}  /* end CDDA_Fill */

static int CDDA_Seek ( FileContext* apCtx, unsigned int aPos ) {

 CDDAFilePrivate* lpPriv = ( CDDAFilePrivate* )apCtx -> m_pData;
 int              lOffset;
 int              lSector;

 if ( aPos > apCtx -> m_Size ) return -1;

 lOffset = aPos - (   apCtx -> m_Pos - ( apCtx -> m_pEnd - apCtx -> m_pBuff[ apCtx -> m_CurBuf ] )  );

 if (  lOffset >= 0 && lOffset <= ( apCtx -> m_pEnd - apCtx -> m_pBuff[ apCtx -> m_CurBuf ] )  ) {

  apCtx -> m_pPos = apCtx -> m_pBuff[ apCtx -> m_CurBuf ] + lOffset;

 } else {

  lOffset                   = aPos + lpPriv -> m_pCtx -> m_Offset;
  lpPriv -> m_CurrentSector = lSector = lpPriv -> m_StartSector + lOffset / 2352;

  apCtx -> m_Pos = 0;
  apCtx -> Fill ( apCtx );

  if ( lSector != lpPriv -> m_StartSector ) {

   apCtx -> m_Pos  += ( lSector - lpPriv -> m_StartSector - 1 ) * 2352 + 2352 - lpPriv -> m_pCtx -> m_Offset;
   apCtx -> m_pPos += lOffset % 2352;

  } else apCtx -> m_pPos += aPos % 2352;

 }  /* end else */

 return apCtx -> m_CurPos = aPos;

}  /* end CDDA_Seek */

static int CDDA_FillStm ( FileContext* apCtx ) {

 CDDAFilePrivate* lpPriv = ( CDDAFilePrivate* )apCtx -> m_pData;
 int              lOldBuf;

 if (  apCtx -> m_Pos == apCtx -> m_Size || !CDDA_Sync ( lpPriv -> m_pCtx )  ) return 0;

 lOldBuf                    =  apCtx -> m_CurBuf;
 apCtx -> m_CurBuf          = !apCtx -> m_CurBuf;
 lpPriv -> m_CurrentSector += lpPriv -> m_nSectorsRead;

 apCtx -> m_pPos = apCtx -> m_pBuff[ apCtx -> m_CurBuf ];

 if ( lpPriv -> m_CurrentSector >= lpPriv -> m_EndSector ) {

  apCtx -> m_Pos  = apCtx -> m_Size;
  apCtx -> m_pEnd = apCtx -> m_pPos + lpPriv -> m_Offset;

 } else {

  int lnSectors = lpPriv -> m_EndSector - lpPriv -> m_CurrentSector;

  apCtx -> m_pEnd = apCtx -> m_pBuff[ apCtx -> m_CurBuf ] + apCtx -> m_BufSize;
  apCtx -> m_Pos += apCtx -> m_BufSize;

  if ( lnSectors == 0 )

   lnSectors = 1;

  else if ( lnSectors > lpPriv -> m_nSectors ) lnSectors = lpPriv -> m_nSectors;

  CDDA_ReadSectors ( lpPriv -> m_pCtx, lpPriv -> m_CurrentSector, lnSectors, apCtx -> m_pBuff[ lOldBuf ] );

  lpPriv -> m_nSectorsRead = lnSectors;

 }  /* end else */

 return 1;

}  /* end CDDA_FillStm */

static int CDDA_SeekStm ( FileContext* apCtx, unsigned int aPos ) {

 CDDAFilePrivate* lpPriv = ( CDDAFilePrivate* )apCtx -> m_pData;

 if (  aPos > apCtx -> m_Size || !CDDA_Sync ( lpPriv -> m_pCtx )  ) return -1;

 apCtx -> Stream ( apCtx, aPos, apCtx -> m_BufSize / 2352 );

 return 1;

}  /* end CDDA_SeekStm */

static int CDDA_Stream ( FileContext* apCtx, unsigned int aStartPos, unsigned int anSectors ) {

 CDDAFilePrivate* lpPriv = ( CDDAFilePrivate* )apCtx -> m_pData;
 int              lOffset;
 int              lnSectors;

 if ( anSectors == 0 ) {

  CDDA_Sync ( lpPriv -> m_pCtx );
  apCtx -> m_CurBuf = 0;
  apCtx -> Seek     = CDDA_Seek;
  apCtx -> Fill     = CDDA_Fill;

  if ( aStartPos >= apCtx -> m_Size ) return 0;

  apCtx -> Seek ( apCtx, aStartPos );

 } else {

  if ( aStartPos >= apCtx -> m_Size ) return 0;

  apCtx -> m_BufSize = anSectors * 2352;

  apCtx -> m_pBase[ 0 ] = realloc ( apCtx -> m_pBase[ 0 ], apCtx -> m_BufSize + 63 );
  apCtx -> m_pBase[ 1 ] = realloc ( apCtx -> m_pBase[ 1 ], apCtx -> m_BufSize + 63 );

  if ( apCtx -> m_pBase[ 0 ] == NULL || apCtx -> m_pBase[ 1 ] == NULL ) return 0;

  apCtx -> m_pBuff[ 0 ] = UNCACHED_SEG(    (   (  ( unsigned int )apCtx -> m_pBase[ 0 ]  ) + 63   ) & 0xFFFFFFC0    );
  apCtx -> m_pBuff[ 1 ] = UNCACHED_SEG(    (   (  ( unsigned int )apCtx -> m_pBase[ 1 ]  ) + 63   ) & 0xFFFFFFC0    );

  lOffset                   = aStartPos + lpPriv -> m_pCtx -> m_Offset;
  apCtx -> m_CurBuf         = 0;
  lpPriv -> m_nSectors      = anSectors;
  lpPriv -> m_CurrentSector = lpPriv -> m_StartSector + lOffset / 2352;
  lnSectors                 = lpPriv -> m_EndSector - lpPriv -> m_CurrentSector;

  if ( lnSectors == 0 )

   lnSectors = 1;

  else if ( lnSectors > lpPriv -> m_nSectors ) lnSectors = lpPriv -> m_nSectors;

  if (  CDDA_ReadSectors ( lpPriv -> m_pCtx, lpPriv -> m_CurrentSector, lnSectors, apCtx -> m_pBuff[ 0 ] ) &&
        CDDA_Sync ( lpPriv -> m_pCtx )
  ) {

   apCtx -> m_pPos = apCtx -> m_pBuff[ 0 ];

   if ( lpPriv -> m_CurrentSector == lpPriv -> m_StartSector ) apCtx -> m_pPos += lpPriv -> m_pCtx -> m_Offset + aStartPos % 2352;

   if ( lpPriv -> m_CurrentSector == lpPriv -> m_EndSector ) {

    apCtx -> m_Pos  = apCtx -> m_Size;
    apCtx -> m_pEnd = lpPriv -> m_CurrentSector == lpPriv -> m_StartSector ?
                       apCtx -> m_pPos + apCtx -> m_Size                   :
                       apCtx -> m_pBuff[ 0 ] + apCtx -> m_BufSize;

   } else {

    lpPriv -> m_CurrentSector += lnSectors;

    apCtx -> m_pPos  = apCtx -> m_pBuff[ 0 ];
    apCtx -> m_pPos += lOffset % 2352;

    if ( lpPriv -> m_CurrentSector >= lpPriv -> m_EndSector ) {

     apCtx -> m_Pos  = apCtx -> m_Size;
     apCtx -> m_pEnd = apCtx -> m_pPos + apCtx -> m_Size - aStartPos;

    } else {

     int lnSectors = lpPriv -> m_EndSector - lpPriv -> m_CurrentSector;

     if ( lnSectors == 0 )

      lnSectors = 1;

     else if ( lnSectors > lpPriv -> m_nSectors ) lnSectors = lpPriv -> m_nSectors;

     apCtx -> m_pEnd    = apCtx -> m_pBuff[ apCtx -> m_CurBuf ] + apCtx -> m_BufSize;
     apCtx -> m_Pos    += apCtx -> m_BufSize;
     lpPriv -> m_Offset = ( apCtx -> m_Size - aStartPos + lOffset ) % apCtx -> m_BufSize;

     CDDA_ReadSectors ( lpPriv -> m_pCtx, lpPriv -> m_CurrentSector, lnSectors, apCtx -> m_pBuff[ 1 ] );

     lpPriv -> m_nSectorsRead = lnSectors;

    }  /* end else */

   }  /* end else */

   apCtx -> Fill     = CDDA_FillStm;
   apCtx -> Seek     = CDDA_SeekStm;
   apCtx -> m_CurPos = aStartPos;

  } else return 0;

 }  /* end if */

 return 1;

}  /* end CDDA_Stream */

static void CDDA_DestroyFileContext ( FileContext* apCtx ) {

 if ( apCtx != NULL ) {

  free ( apCtx -> m_pData      );
  free ( apCtx -> m_pBase[ 0 ] );

  if ( apCtx -> m_pBase[ 1 ] != NULL ) free ( apCtx -> m_pBase[ 1 ] );

  free ( apCtx );

 }  /* end if */

}  /* end CDDA_DestroyFileContext */

static FileContext* CDDA_InitFileContextInternal ( CDDAContext* apCtx, int aStart, int aSize, int anImgIdx ) {

 FileContext* retVal    = malloc (  sizeof ( FileContext )  );
 int          lfSuccess = 0;

 if ( retVal != NULL ) {

  retVal -> m_pBase[ 0 ] = malloc ( 2352 + 63 );

  if ( retVal -> m_pBase[ 0 ] != NULL ) {

   retVal -> m_pBuff[ 0 ] = UNCACHED_SEG(    (   (  ( unsigned int )retVal -> m_pBase[ 0 ]  ) + 63   ) & 0xFFFFFFC0    );
   retVal -> m_pBase[ 1 ] = NULL;
   retVal -> m_pBuff[ 1 ] = NULL;
   retVal -> m_CurBuf     = 0;
   retVal -> m_Size       = aSize;
   retVal -> m_BufSize    = 2352;
   retVal -> m_Pos        = 0;
   retVal -> m_CurPos     = 0;
   retVal -> m_pEnd       = retVal -> m_pBuff[ 0 ] + 2352;
   retVal -> m_pData      = malloc (  sizeof ( CDDAFilePrivate )  );

   if ( retVal -> m_pData ) {

    aSize += apCtx -> m_Offset;

    (  ( CDDAFilePrivate* )retVal -> m_pData  ) -> m_StartSector   = aStart;
    (  ( CDDAFilePrivate* )retVal -> m_pData  ) -> m_EndSector     = aStart + aSize / 2352 + (  ( aSize % 2352 ) != 0  );
    (  ( CDDAFilePrivate* )retVal -> m_pData  ) -> m_CurrentSector = aStart;
    (  ( CDDAFilePrivate* )retVal -> m_pData  ) -> m_ImgIdx        = anImgIdx;
    (  ( CDDAFilePrivate* )retVal -> m_pData  ) -> m_pCtx          = apCtx;

    retVal -> Read    = CDDA_Read;
    retVal -> Seek    = CDDA_Seek;
    retVal -> Fill    = CDDA_Fill;
    retVal -> Stream  = CDDA_Stream;
    retVal -> Destroy = CDDA_DestroyFileContext;

    if (  retVal -> Fill ( retVal )  ) lfSuccess = 1;
    
   }  /* end if */

   if ( !lfSuccess ) free ( retVal -> m_pBase[ 0 ] );

  }  /* end if */

  if ( !lfSuccess ) {

   free ( retVal );
   retVal = NULL;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end CDDA_InitFileContextInternal */

static void _read_string ( FileContext* apCtx, char** apBuf, int aMaxLen ) {
#ifdef _WIN32
 char* lpBuf = _alloca ( aMaxLen );
#else  /* PS2 */
 char  lBuf[ aMaxLen ];
 char* lpBuf = lBuf;
#endif  /* _WIN32 */
 if (  apCtx -> Read ( apCtx, lpBuf, aMaxLen )  ) {

  int lLen = strlen ( lpBuf );

  if (   lLen && (  *apBuf = malloc ( lLen + 1 )  )   ) strcpy ( *apBuf, lpBuf );

 } else *apBuf = NULL;

}  /* end _read_string */

static void CDDA_LoadDirectoryList ( CDDAContext* apCtx ) {

 FileContext* lpCtx = CDDA_InitFileContextInternal (
  apCtx, apCtx -> m_StartSector[ 0 ], ( apCtx -> m_EndSector[ 0 ] + 1 ) * 2352 + ( apCtx -> m_Offset ? 2352 : 0 ), -1
 );

 apCtx -> m_pName        = NULL;
 apCtx -> m_pDescription = NULL;
 apCtx -> m_pData        = NULL;

 if ( lpCtx ) {

  apCtx -> m_pData = malloc (  sizeof ( CDDAFileContext )  );

  if ( apCtx -> m_pData != NULL ) {

   short         lCount;
   unsigned char lBuf[ 30 ];

   lpCtx -> Read ( lpCtx, lBuf, 16 );

   (  ( CDDAFileContext* )apCtx -> m_pData  ) -> m_pCtx   = lpCtx;
   (  ( CDDAFileContext* )apCtx -> m_pData  ) -> m_pNodes = NULL;

   _read_string ( lpCtx, &apCtx -> m_pName,         64 );
   _read_string ( lpCtx, &apCtx -> m_pDescription, 512 );

   lpCtx -> Seek ( lpCtx, 16 + 64 + 512 + 9216 );

   if (   lpCtx -> Read (  lpCtx, ( unsigned char* )&lCount, 2  )   ) {

    unsigned int  i;

    lpCtx -> Seek ( lpCtx, 16 + 64 + 512 + 9216 + 2 + 4096 * lCount );

    for ( i = 1; i < apCtx -> m_nTracks; ++i ) {

     CDDADirectory* lpNode = ( CDDADirectory* )malloc (  sizeof ( CDDADirectory )  );

     if ( lpNode != NULL ) {

      _read_string ( lpCtx, &lpNode -> m_pName, 32 );

      lpNode -> m_Idx    =  i;
      lpNode -> m_ImgIdx = -1;

      if (   lpCtx -> Read (  lpCtx, ( unsigned char* )&lCount, 2  )   ) lpNode -> m_ImgIdx = lCount;

      lpCtx -> Read ( lpCtx, lBuf, 30 );

      lpNode -> m_pNext = (  ( CDDAFileContext* )apCtx -> m_pData  ) -> m_pNodes;
      (  ( CDDAFileContext* )apCtx -> m_pData  ) -> m_pNodes = lpNode;

     }  /* end if */

    }  /* end for */

   }  /* end if */

  }  /* end if */

 } else apCtx -> m_pData = NULL;

}  /* end CDDA_LoadDirectoryList */

int CDDA_GetPicture ( CDDAContext* apCtx, int anIndex, void* apData ) {

 int          retVal = 0;
 FileContext* lpCtx  = (  ( CDDAFileContext* )apCtx -> m_pData  ) -> m_pCtx;

 if (  lpCtx != NULL && lpCtx -> Seek ( lpCtx, 16 + 64 + 512 + 9216 ) != -1  ) {

  unsigned short lnImages;

  if (  lpCtx -> Read ( lpCtx, ( unsigned char* )&lnImages, 2 ) == 2 &&
        lnImages > ( unsigned short )anIndex                         &&
        lpCtx -> Seek ( lpCtx, 16 + 64 + 512 + 9216 + 2 + 4096 * anIndex ) != -1
  ) retVal = lpCtx -> Read ( lpCtx, apData, 4096 );

 }  /* end if */

 return retVal;

}  /* end CDDA_GetPicture */

int CDDA_GetDiskPicture ( CDDAContext* apCtx, void* apData ) {

 int          retVal = 0;
 FileContext* lpCtx  = (  ( CDDAFileContext* )apCtx -> m_pData  ) -> m_pCtx;

 if (  lpCtx != NULL && lpCtx -> Seek ( lpCtx, 16 + 64 + 512 ) != -1  )

  retVal = lpCtx -> Read ( lpCtx, apData, 9216 );

 return retVal;

}  /* end CDDA_GetDiskPicture */

CDDAContext* CDDA_InitContext ( unsigned long aDrive ) {

 int          lfSuccess = 0;
 CDDAContext* retVal    = NULL;
#ifdef _WIN32
 unsigned char lDevName[ 7 ] = {
  '\\', '\\', '.', '\\', ( unsigned char )aDrive, ':', '\x00'
 };
 HANDLE lhDevice = CreateFileA (
  lDevName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL
 );
#ifdef _WIN32
 Sleep ( 2000 );
#endif  /* _WIN32 */
 if ( lhDevice != INVALID_HANDLE_VALUE ) {

  CDROMToc   lTOC;
  DWORD      lnRead;
  OVERLAPPED lOvrlp;

  lOvrlp.Offset     = 0;
  lOvrlp.OffsetHigh = 0;
  lOvrlp.hEvent     = CreateEvent ( NULL, TRUE, FALSE, NULL );

  if ( lOvrlp.hEvent != NULL ) {

   if (  DeviceIoControl (
          lhDevice, IOCTL_CDROM_READ_TOC, NULL, 0,
          &lTOC, sizeof ( CDROMToc ), &lnRead, &lOvrlp
         ) || (  GetLastError () == ERROR_IO_PENDING &&
                 GetOverlappedResult ( lhDevice, &lOvrlp, &lnRead, TRUE )
              )
   ) {

    int i;

    retVal = ( CDDAContext* )malloc (  sizeof ( CDDAContext ) + sizeof ( CDDAWin32Private )  );
    retVal -> m_nTracks = 0;
    HCDROM( retVal )    = lhDevice;
    OVERLP( retVal )    = lOvrlp;

    for ( i = lTOC.m_FirstTrack; i <= lTOC.m_LastTrack; ++i )

     if (  _is_audio ( &lTOC, i )  ) {

      retVal -> m_StartSector[ retVal -> m_nTracks ] = _start_sector ( &lTOC, i );
      retVal -> m_EndSector  [ retVal -> m_nTracks ] = _end_sector   ( &lTOC, i );
      ++retVal -> m_nTracks;

     }  /* end if */

    if (  retVal -> m_nTracks && _find_signature ( retVal )  ) {

     lfSuccess = TRUE;

    } else {

     free ( retVal );
     retVal = NULL;

    }  /* end else */

   }  /* end if */

  }  /* end if */

  if ( !lfSuccess ) {

   if ( lOvrlp.hEvent != NULL ) CloseHandle ( lOvrlp.hEvent );

   CloseHandle ( lhDevice );

  }  /* end if */

 }  /* end if */
#else  /* PS2 */
 CDDA_TOC lToc;
 s32      lStat = CDDA_Init ();

 if ( lStat ) {

  if (  CDDA_DiskType () == DiskType_CDDA && CDDA_ReadTOC ( &lToc )  ) {

   int i;

   lToc.m_StartTrack = btoi( lToc.m_StartTrack );
   lToc.m_EndTrack   = btoi( lToc.m_EndTrack   );

   retVal = ( CDDAContext* )malloc (  sizeof ( CDDAContext )  );
   retVal -> m_nTracks = 0;

   for ( i = lToc.m_StartTrack; i <= lToc.m_EndTrack; ++i ) {

    retVal -> m_StartSector[ retVal -> m_nTracks ] = _start_sector ( &lToc, retVal -> m_nTracks );
    retVal -> m_EndSector  [ retVal -> m_nTracks ] = _end_sector   ( &lToc, retVal -> m_nTracks );

    ++retVal -> m_nTracks;

   }  /* end for */

   if (  retVal -> m_nTracks && _find_signature ( retVal )  ) {

    lfSuccess = 1;

   } else {

    free ( retVal );
    retVal = NULL;

   }  /* end else */

  }  /* end if */

  if ( !lfSuccess ) {

   CDDA_Stop        ();
   CDDA_Synchronize ();

  }  /* end if */

 }  /* end if */
#endif  /* _WIN32 */
 if ( retVal ) CDDA_LoadDirectoryList ( retVal );

 return retVal;

}  /* end CDDA_InitContext */

void CDDA_DestroyContext ( CDDAContext* apCtx ) {

 if ( apCtx != NULL ) {

  CDDA_Sync ( apCtx );
#ifdef _WIN32
  CloseHandle (  OVERLP( apCtx ).hEvent  );
  CloseHandle (  HCDROM( apCtx )         );
#else  /* PS2 */
  CDDA_Stop        ();
  CDDA_Synchronize ();
#endif  /* _WIN32 */
  if ( apCtx -> m_pData != NULL ) {

   CDDADirectory* lpNode = (  ( CDDAFileContext* )apCtx -> m_pData  ) -> m_pNodes;

   while ( lpNode != NULL ) {

    CDDADirectory* lpNext = lpNode -> m_pNext;

    if ( lpNode -> m_pName != NULL ) free ( lpNode -> m_pName );

    free ( lpNode );

    lpNode = lpNext;

   }  /* end while */

   CDDA_DestroyFileContext (   (  ( CDDAFileContext* )apCtx -> m_pData  ) -> m_pCtx  );

   free ( apCtx -> m_pData );

  }  /* end if */

  if ( apCtx -> m_pDescription != NULL ) free ( apCtx -> m_pDescription );
  if ( apCtx -> m_pName        != NULL ) free ( apCtx -> m_pName        );

  free ( apCtx );

 }  /* end if */

}  /* end CDDA_DestroyContext */

const CDDADirectory* CDDA_DirectoryList ( CDDAContext* apCtx ) {

 return apCtx != NULL ? (  ( CDDAFileContext* )apCtx -> m_pData  ) -> m_pNodes : NULL;

}  /* end CDDA_DirectoryList */

void CDDA_DestroyFileList ( CDDAFile* apFileList ) {

 if ( apFileList != NULL ) {

  CDDAFile* lpNode = apFileList;

  while ( lpNode != NULL ) {

   CDDAFile* lpNext = lpNode -> m_pNext;

   if ( lpNode -> m_pName != NULL ) free ( lpNode -> m_pName );

   free ( lpNode );

   lpNode = lpNext;

  }  /* end while */

 }  /* end if */

}  /* end CDDA_DestroyFileList */

CDDAFile* CDDA_GetFileList ( CDDAContext* apDiskCtx, const CDDADirectory* apDirCtx ) {

 CDDAFile*    retVal = NULL;
 FileContext* lpCtx  = CDDA_InitFileContextInternal (
                        apDiskCtx,
                        apDiskCtx -> m_StartSector[ apDirCtx -> m_Idx ],
                        apDiskCtx -> m_EndSector  [ apDirCtx -> m_Idx ] * 2352 + apDiskCtx -> m_Offset, -1
                       );

 if ( lpCtx != NULL ) {

  short lnFiles = 0;

  if (   lpCtx -> Read (  lpCtx, ( unsigned char* )&lnFiles, 2  )   ) {

   int            i;
   char           lBuf[ 30 ];
   unsigned short lIdx;

   for ( i = 0; i < lnFiles; ++i ) {

    CDDAFile* lpFile = malloc (  sizeof ( CDDAFile )  );

    lpFile -> m_Size   =  0;
    lpFile -> m_ImgIdx = -1;
    lpFile -> m_pNext  = retVal;
    retVal = lpFile;

    _read_string ( lpCtx, &lpFile -> m_pName, 32 );

    if (  lpCtx -> Read (   lpCtx, ( unsigned char* )&lIdx, 2  )  ) lpFile -> m_ImgIdx = lIdx;

    lpCtx -> Read ( lpCtx, lBuf, 30 );
    lpCtx -> Read (  lpCtx, ( unsigned char* )&lpFile -> m_Offset, 4  );
    lpCtx -> Read (  lpCtx, ( unsigned char* )&lpFile -> m_Size,   4  );

   }  /* end for */

  }  /* end if */

  CDDA_DestroyFileContext ( lpCtx );

 }  /* end if */

 return retVal;

}  /* end CDDA_GetFileList */

FileContext* CDDA_InitFileContext ( CDDAContext* apCtx, const char* apFileName ) {

 FileContext*         retVal  = NULL;
 const CDDADirectory* lpDirs  = CDDA_DirectoryList ( apCtx );
 const char*          lpStart = strpbrk ( apFileName, "\\/" );
 const char*          lpEnd;
 const char*          lpDirName   = ".";
 int                  lDirNameLen = 0;

 if ( lpStart != NULL ) {

  if ( lpStart != apFileName ) {

   lpEnd   = lpStart;
   lpStart = apFileName;

   while ( *lpStart == '\\' || *lpStart == '/' ) ++lpStart;

  } else {

   while ( *lpStart == '\\' || *lpStart == '/' ) ++lpStart;

   lpEnd = strpbrk ( lpStart, "\\/" );

  }  /* end else */

  if ( lpEnd != NULL ) {

   lpDirName   = lpStart;
   lDirNameLen = lpEnd - lpStart;
   lpStart     = ++lpEnd;

   while ( *lpStart == '\\' || *lpStart == '/' ) ++lpStart;

  }  /* end if */

 } else lpStart = apFileName;

 if ( lDirNameLen == 0 ) lDirNameLen =   1;

 while ( lpDirs != NULL ) {

  if (  lpDirs -> m_pName && !strncmp ( lpDirs -> m_pName, lpDirName, lDirNameLen )  ) break;

  lpDirs = lpDirs -> m_pNext;

 }  /* end while */

 if ( lpDirs != NULL ) {

  CDDAFile* lpFiles = CDDA_GetFileList ( apCtx, lpDirs );

  if ( lpFiles != NULL ) {

   CDDAFile* lpFile = lpFiles;

   while ( lpFile ) {

    if (  lpFile -> m_pName && !strcmp ( lpFile -> m_pName, lpStart )  ) {

     retVal = CDDA_InitFileContextInternal (
      apCtx, apCtx -> m_StartSector[ lpDirs -> m_Idx ] + lpFile -> m_Offset,
      lpFile -> m_Size, lpFile -> m_ImgIdx
     );
     retVal -> m_StreamSize = 432;

     break;

    }  /* end if */

    lpFile = lpFile -> m_pNext;

   }  /* end while */

   CDDA_DestroyFileList ( lpFiles );

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end CDDA_InitFileContext */
#ifdef _WIN32
typedef struct STIOFilePrivate {

 HANDLE     m_hFile;
 OVERLAPPED m_Ovlp;

} STIOFilePrivate;
#else  /* PS2 */
typedef struct STIOFilePrivate {

 int m_FD;

} STIOFilePrivate;
#endif  /* _WIN32 */
static void STIO_DestroyFileContext ( FileContext* apCtx ) {

 if ( apCtx != NULL ) {
#ifdef _WIN32
  CloseHandle (   (  ( STIOFilePrivate* )apCtx -> m_pData  ) -> m_hFile         );
  CloseHandle (   (  ( STIOFilePrivate* )apCtx -> m_pData  ) -> m_Ovlp.hEvent   );
#else  /* PS2 */
  IO_Close (   (  ( STIOFilePrivate* )apCtx -> m_pData  ) -> m_FD   );
  IO_SetBlockMode ( 0 );
#endif  /* _WIN32 */
  free ( apCtx -> m_pData      );
  free ( apCtx -> m_pBase[ 0 ] );

  if ( apCtx -> m_pBase[ 1 ] != NULL ) free ( apCtx -> m_pBase[ 1 ] );

  free ( apCtx );

 }  /* end if */

}  /* end STIO_DestroyFileContext */

static int STIO_Read ( FileContext* apCtx, void* apBuf, unsigned int aSize ) {

 unsigned int lLen, lSize = aSize;

 while ( aSize > 0 ) {

  lLen = apCtx -> m_pEnd - apCtx -> m_pPos;

  if ( lLen > aSize ) lLen = aSize;

  if ( lLen == 0 ) {

   apCtx -> Fill ( apCtx );

   lLen = apCtx -> m_pEnd - apCtx -> m_pPos;

   if ( lLen == 0 ) break;

  } else {

   memcpy ( apBuf, apCtx -> m_pPos, lLen );

   (  ( char* )apBuf  ) += lLen;
   apCtx -> m_pPos      += lLen;
   apCtx -> m_Pos       += lLen;
   apCtx -> m_CurPos    += lLen;
   aSize                -= lLen;

  }  /* end else */

 }  /* end while */

 return lSize - aSize;

}  /* end STIO_Read */

static int STIO_Seek ( FileContext* apCtx, unsigned int aPos ) {

 int lOffset;

 if ( aPos > apCtx -> m_Size ) return -1;

 lOffset = aPos - (   apCtx -> m_Pos - ( apCtx -> m_pEnd - apCtx -> m_pBuff[ apCtx -> m_CurBuf ] )  );

 if (  lOffset >= 0 && lOffset <= ( apCtx -> m_pEnd - apCtx -> m_pBuff[ apCtx -> m_CurBuf ] )  )

  apCtx -> m_pPos = apCtx -> m_pBuff[ apCtx -> m_CurBuf ] + lOffset;

 else {
#ifndef _WIN32
  STIOFilePrivate* lpPriv  = ( STIOFilePrivate* )apCtx -> m_pData;
#endif  /* _WIN32 */
  apCtx -> m_pPos   =
  apCtx -> m_pEnd   = apCtx -> m_pBuff[ apCtx -> m_CurBuf ];
  apCtx -> m_CurPos = apCtx -> m_Pos = aPos;
#ifndef _WIN32
  aPos = IO_LSeek ( lpPriv -> m_FD, aPos, 0 );
#endif  /* _WIN32 */
 }  /* end else */

 return aPos;

}  /* end STIO_Seek */

static int STIO_Fill ( FileContext* apCtx ) {

 int              lLen;
 STIOFilePrivate* lpPriv = ( STIOFilePrivate* )apCtx -> m_pData;
#ifdef _WIN32
 DWORD lnRead;

 lLen = 0;

 lpPriv -> m_Ovlp.Offset     = apCtx -> m_CurPos;
 lpPriv -> m_Ovlp.OffsetHigh = 0;

 if (  ReadFile (
        lpPriv -> m_hFile, apCtx -> m_pBuff[ apCtx -> m_CurBuf ],
        apCtx -> m_BufSize, &lnRead, &lpPriv -> m_Ovlp
       ) || (  GetLastError () == ERROR_IO_PENDING &&
                GetOverlappedResult (
                 lpPriv -> m_hFile, &lpPriv -> m_Ovlp, &lnRead, TRUE
                )
            )
 ) lLen = lnRead;
#else  /* PS2 */
 lLen = IO_Read ( lpPriv -> m_FD, apCtx -> m_pBuff[ apCtx -> m_CurBuf ], apCtx -> m_BufSize );

 if ( lLen < 0 ) lLen = 0;
#endif  /* _WIN32 */
 apCtx -> m_pPos = apCtx -> m_pBuff[ apCtx -> m_CurBuf ];
 apCtx -> m_pEnd = apCtx -> m_pPos + lLen;

 return lLen;

}  /* end STIO_Fill */

static int STIO_FillStm ( FileContext* apCtx ) {

 int              retVal = 0;
 unsigned int     lnRead = 0;
 STIOFilePrivate* lpPriv = ( STIOFilePrivate* )apCtx -> m_pData;

 if (  apCtx -> m_Pos < apCtx -> m_Size ) {

   int lOldBuf;
#ifdef _WIN32
  if (  GetOverlappedResult (
         lpPriv -> m_hFile, &lpPriv -> m_Ovlp, &lnRead, TRUE
        )
  ) {
#else  /* PS2 */
  if (  IO_Wait ( 0, &lnRead )  ) {
#endif  /* _WIN32 */
   lOldBuf            = apCtx -> m_CurBuf;
   apCtx -> m_CurBuf  = !apCtx -> m_CurBuf;
   apCtx -> m_pPos    = apCtx -> m_pBuff[ apCtx -> m_CurBuf ];
   apCtx -> m_pEnd    = apCtx -> m_pPos + lnRead;

   if (  apCtx -> m_CurPos + lnRead < apCtx -> m_Size ) {
#ifdef _WIN32
    lpPriv -> m_Ovlp.Offset     = apCtx -> m_CurPos + lnRead;
    lpPriv -> m_Ovlp.OffsetHigh = 0;

    ReadFile (
     lpPriv -> m_hFile, apCtx -> m_pBuff[ lOldBuf ], apCtx -> m_BufSize,
     &lnRead, &lpPriv -> m_Ovlp
    );
#else
    IO_Read ( lpPriv -> m_FD, apCtx -> m_pBuff[ lOldBuf ], apCtx -> m_BufSize );
#endif  /* _WIN32 */
   }  /* end if */

   retVal = 1;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end STIO_FillStm */

static int STIO_SeekStm ( FileContext* apCtx, unsigned int aPos ) {
#ifdef _WIN32
 DWORD            lnRead;
 STIOFilePrivate* lpPriv = ( STIOFilePrivate* )apCtx -> m_pData;

 if (  aPos > apCtx -> m_Size ||
       !GetOverlappedResult (
         lpPriv -> m_hFile, &lpPriv -> m_Ovlp, &lnRead, TRUE
        )
 ) return -1;
#else  /* PS2 */
 if (  aPos > apCtx -> m_Size || IO_Wait ( 0, NULL ) < 0  ) return -1;
#endif  /* _WIN32 */
 apCtx -> Stream ( apCtx, aPos, apCtx -> m_BufSize / 4096 );

 return 1;

}  /* end CDDA_SeekStm */

static int STIO_Stream ( FileContext* apCtx, unsigned int aStartPos, unsigned int anBlocks ) {

 STIOFilePrivate* lpPriv = ( STIOFilePrivate* )apCtx -> m_pData;
 void*            lpData;
 int              retVal = 0;
 unsigned int     lnRead = 0;

 if ( anBlocks == 0 ) {
#ifdef _WIN32
  GetOverlappedResult (
   lpPriv -> m_hFile, &lpPriv -> m_Ovlp, &lnRead, TRUE
  );
#else  /* PS2 */
  IO_Wait ( 0, &lnRead );
  IO_SetBlockMode ( 0 );
#endif  /* _WIN32 */
  apCtx -> m_CurBuf = 0;
  apCtx -> Seek     = STIO_Seek;
  apCtx -> Fill     = STIO_Fill;

  if ( aStartPos >= apCtx -> m_Size ) return 0;

  apCtx -> Seek ( apCtx, aStartPos );

 } else {

  if ( aStartPos >= apCtx -> m_Size ) return 0;

  apCtx -> m_BufSize = anBlocks * 4096;

  lpData                = realloc ( apCtx -> m_pBase[ 0 ], apCtx -> m_BufSize + 63 );
  apCtx -> m_pBase[ 1 ] = realloc ( apCtx -> m_pBase[ 1 ], apCtx -> m_BufSize + 63 );

  if ( lpData == NULL || apCtx -> m_pBase[ 1 ] == NULL ) return 0;

  apCtx -> m_pBase[ 0 ] = lpData;
  apCtx -> m_pBuff[ 0 ] = UNCACHED_SEG(    (   (  ( unsigned int )apCtx -> m_pBase[ 0 ]  ) + 63   ) & 0xFFFFFFC0    );
  apCtx -> m_pBuff[ 1 ] = UNCACHED_SEG(    (   (  ( unsigned int )apCtx -> m_pBase[ 1 ]  ) + 63   ) & 0xFFFFFFC0    );
  apCtx -> m_CurPos     = aStartPos;

  apCtx -> Seek = STIO_SeekStm;
  apCtx -> Fill = STIO_FillStm;

#ifdef _WIN32
  lpPriv -> m_Ovlp.Offset     = apCtx -> m_CurPos;
  lpPriv -> m_Ovlp.OffsetHigh = 0;

  if (  ReadFile (
         lpPriv -> m_hFile, apCtx -> m_pBuff[ 0 ], apCtx -> m_BufSize,
         &lnRead, &lpPriv -> m_Ovlp
        ) || (  GetLastError () == ERROR_IO_PENDING &&
                GetOverlappedResult (
                 lpPriv -> m_hFile, &lpPriv -> m_Ovlp, &lnRead, TRUE
                )
             )
  ) retVal = lnRead;
#else  /* PS2 */
  IO_LSeek ( lpPriv -> m_FD, apCtx -> m_CurPos, 0 );
  retVal = lnRead = IO_Read ( lpPriv -> m_FD, apCtx -> m_pBuff[ 0 ], apCtx -> m_BufSize );
#endif  /* _WIN32 */
  apCtx -> m_pPos = apCtx -> m_pBuff[ 0 ];
  apCtx -> m_pEnd = apCtx -> m_pPos + retVal;

  if ( lnRead == apCtx -> m_BufSize ) {
#ifdef _WIN32
   lpPriv -> m_Ovlp.Offset     = apCtx -> m_CurPos + lnRead;
   lpPriv -> m_Ovlp.OffsetHigh = 0;

   ReadFile (
    lpPriv -> m_hFile, apCtx -> m_pBuff[ 1 ],
    apCtx -> m_BufSize, &lnRead, &lpPriv -> m_Ovlp
   );
#else  /* PS2 */
   IO_SetBlockMode ( 1 );
   IO_Read ( lpPriv -> m_FD, apCtx -> m_pBuff[ 1 ], apCtx -> m_BufSize );
#endif  /* _WIN32 */
  }  /* end if */

 }  /* end else */

 return retVal;

}  /* end STIO_Stream */

FileContext* STIO_InitFileContext ( const char* aFileName ) {

 int          lfSuccess = 0;
 FileContext* retVal    = NULL;
#ifdef _WIN32
 HANDLE lhFile = CreateFileA (
  aFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
  FILE_FLAG_OVERLAPPED, NULL
 );

 if ( lhFile != INVALID_HANDLE_VALUE ) {

  HANDLE lhEvent = CreateEvent ( NULL, TRUE, FALSE, NULL );

  if ( lhEvent != NULL ) {

   retVal = malloc (  sizeof ( FileContext )  );

   if ( retVal != NULL ) {

    retVal -> m_pBase[ 0 ] = malloc ( 4096 + 63 );
    retVal -> m_pData      = malloc (  sizeof ( STIOFilePrivate )  );

    if ( retVal -> m_pData != NULL && retVal -> m_pBase[ 0 ] != NULL ) {

     retVal -> m_pBase[ 1 ] = NULL;
     retVal -> m_pBuff[ 0 ] = UNCACHED_SEG(    (   (  ( unsigned int )retVal -> m_pBase[ 0 ]  ) + 63   ) & 0xFFFFFFC0    );
     retVal -> m_pBuff[ 1 ] = NULL;
     retVal -> m_CurBuf     = 0;
     retVal -> m_Size       = GetFileSize ( lhFile, NULL );
     retVal -> m_BufSize    = 4096;
     retVal -> m_Pos        = 0;
     retVal -> m_CurPos     = 0;
     retVal -> m_pPos       = retVal -> m_pBuff[ 0 ];
     retVal -> m_pEnd       = retVal -> m_pPos;

     (  ( STIOFilePrivate* )retVal -> m_pData  ) -> m_hFile       = lhFile;
     (  ( STIOFilePrivate* )retVal -> m_pData  ) -> m_Ovlp.hEvent = lhEvent;

     retVal -> Fill    = STIO_Fill;
     retVal -> Destroy = STIO_DestroyFileContext;
     retVal -> Read    = STIO_Read;
     retVal -> Seek    = STIO_Seek;
     retVal -> Stream  = STIO_Stream;

     retVal -> m_StreamSize = 384;

     lfSuccess = 1;

    }  /* end if */

   }  /* end if */

  }  /* end if */

  if ( !lfSuccess ) {

   if ( retVal != NULL ) {

    if ( retVal -> m_pData      != NULL ) free ( retVal -> m_pData      );
    if ( retVal -> m_pBase[ 0 ] != NULL ) free ( retVal -> m_pBase[ 0 ] );

    free ( retVal );
    retVal = NULL;

   }  /* end if */

   if ( lhEvent != NULL ) CloseHandle ( lhEvent );

   CloseHandle ( lhFile );

  }  /* end if */

 }  /* end if */
#else  /* PS2 */
 int lFD = IO_Open ( aFileName );

 if ( lFD >= 0 ) {

  retVal = malloc (  sizeof ( FileContext )  );

  if ( retVal != NULL ) {

   retVal -> m_pBase[ 0 ] = malloc ( 4096 + 63 );
   retVal -> m_pData      = malloc (  sizeof ( STIOFilePrivate )  );

   if ( retVal -> m_pData != NULL && retVal -> m_pBase[ 0 ] != NULL ) {

    retVal -> m_pBase[ 1 ] = NULL;
    retVal -> m_pBuff[ 0 ] = UNCACHED_SEG(    (   (  ( unsigned int )retVal -> m_pBase[ 0 ]  ) + 63   ) & 0xFFFFFFC0    );
    retVal -> m_pBuff[ 1 ] = NULL;
    retVal -> m_CurBuf     = 0;
    retVal -> m_Size       = IO_LSeek ( lFD, 0, 2 );
    retVal -> m_BufSize    = 4096;
    retVal -> m_Pos        = 0;
    retVal -> m_CurPos     = 0;
    retVal -> m_pPos       = retVal -> m_pBuff[ 0 ];
    retVal -> m_pEnd       = retVal -> m_pPos;
    retVal -> m_StreamSize = 384;

    (  ( STIOFilePrivate* )retVal -> m_pData  ) -> m_FD = lFD;

    retVal -> Fill    = STIO_Fill;
    retVal -> Destroy = STIO_DestroyFileContext;
    retVal -> Read    = STIO_Read;
    retVal -> Seek    = STIO_Seek;
    retVal -> Stream  = STIO_Stream;
 
    IO_LSeek ( lFD, 0, 0 );

    lfSuccess = 1;

   }  /* end if */

  }  /* end if */

 }  /* end if */

 if ( !lfSuccess ) {

  if ( retVal != NULL ) {

   if ( retVal -> m_pData      != NULL ) free ( retVal -> m_pData      );
   if ( retVal -> m_pBase[ 0 ] != NULL ) free ( retVal -> m_pBase[ 0 ] );

   free ( retVal );
   retVal = NULL;

  }  /* end if */

  if ( lFD >= 0 ) IO_Close ( lFD );

 }  /* end if */
#endif  /* _WIN32 */
 return retVal;

}  /* end STIO_InitFileContext */

void File_Skip ( FileContext* apFileCtx, unsigned int aCount ) {

 static char s_lBuff[ 512 ];

 unsigned int lnBlocks = aCount / 512;
 unsigned int lnRem    = aCount % 512;
 unsigned int i;

 for ( i = 0; i < lnBlocks; ++i ) apFileCtx -> Read ( apFileCtx, s_lBuff, 512 );

 if ( lnRem ) apFileCtx -> Read ( apFileCtx, s_lBuff, lnRem );

}  /* end File_Skip */
