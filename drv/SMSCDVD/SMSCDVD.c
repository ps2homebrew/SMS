/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2002, A.Lee & Nicholas Van Veen
# Copyright (c) 2005, Eugene Plotnikov (SMS project)
# All rights reserved.
# 
# Redistribution and use of this software, in source and binary forms, with or
# without modification, are permitted provided that the following conditions are
# met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this 
#    list of conditions and the following disclaimer.
#     
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation 
#    and/or other materials provided with the distribution.
#     
# 3. You are granted a license to use this software for academic, research and
#    non-commercial purposes only.
# 
# 4. The copyright holder imposes no restrictions on any code developed using
#    this software. However, the copyright holder retains a non-exclusive
#    royalty-free license to any modifications to the distribution made by the
#    licensee.
# 
# 5. Any licensee wishing to make commercial use of this software should contact
#    the copyright holder to execute the appropriate license for such commercial
#    use. Commercial use includes:
#  
#    -  Integration of all or part of the source code into a product for sale 
#       or commercial license by or on behalf of Licensee to third parties, or
# 
#    -  Distribution of the binary code or source code to third parties that 
#       need it to utilize a commercial product sold or licensed by or on 
#       behalf of Licensee.
#        
#  
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
# OF SUCH DAMAGE.
*/
#include "SMSCDVD.h"

#define TRUE	1
#define FALSE	0

enum PathMatch {
 NOT_MATCH = 0, MATCH, SUBDIR
};

#define MAX_DIR_CACHE_SECTORS 32

typedef struct rootDirTocHeader {

 u16 length         __attribute__(  ( packed )  );
 u32 tocLBA         __attribute__(  ( packed )  );
 u32 tocLBA_bigend  __attribute__(  ( packed )  );
 u32 tocSize        __attribute__(  ( packed )  );
 u32 tocSize_bigend __attribute__(  ( packed )  );
 u8  dateStamp[ 8 ] __attribute__(  ( packed )  );
 u8  reserved [ 6 ] __attribute__(  ( packed )  );
 u8  reserved2      __attribute__(  ( packed )  );
 u8  reserved3      __attribute__(  ( packed )  );

} rootDirTocHeader;

typedef struct asciiDate {

 char	year      [ 4 ] __attribute__(  ( packed )  );
 char	month     [ 2 ] __attribute__(  ( packed )  );
 char	day       [ 2 ] __attribute__(  ( packed )  );
 char	hours     [ 2 ] __attribute__(  ( packed )  );
 char	minutes   [ 2 ] __attribute__(  ( packed )  );
 char	seconds   [ 2 ] __attribute__(  ( packed )  );
 char	hundreths [ 2 ] __attribute__(  ( packed )  );
 char	terminator[ 1 ] __attribute__(  ( packed )  );

} asciiDate;

typedef struct cdVolDesc {

 u8               m_FSType                      __attribute__(  ( packed )  );
 u8               m_VolID[ 5 ]                  __attribute__(  ( packed )  );
 u8               m_Reserved2                   __attribute__(  ( packed )  );
 u8               m_Reserved3                   __attribute__(  ( packed )  );
 u8               m_SysIDName[ 32 ]             __attribute__(  ( packed )  );
 u8               m_VolName  [ 32 ]             __attribute__(  ( packed )  );
 u8               m_Reserved5[  8 ]             __attribute__(  ( packed )  );
 u32              m_VolSize                     __attribute__(  ( packed )  );
 u32              m_VolSizeBE                   __attribute__(  ( packed )  );
 u8               m_Reserved6[ 32 ]             __attribute__(  ( packed )  );
 u32              m_Unknown1                    __attribute__(  ( packed )  );
 u32              m_Uunknown1BE                 __attribute__(  ( packed )  );
 u16              m_VolDescSize                 __attribute__(  ( packed )  );
 u16              m_VolDescSizeBE               __attribute__(  ( packed )  );
 u32              m_Unknown3                    __attribute__(  ( packed )  );
 u32              m_Unknown3BE                  __attribute__(  ( packed )  );
 u32              m_PriDirTblLBA                __attribute__(  ( packed )  );
 u32              m_Reserved7                   __attribute__(  ( packed )  );
 u32              m_SecDirTblLBA                __attribute__(  ( packed )  );
 u32              m_Reserved8                   __attribute__(  ( packed )  );
 rootDirTocHeader m_RootToc                     __attribute__(  ( packed )  );
 u8               m_VolSetName          [ 128 ] __attribute__(  ( packed )  );
 u8               m_PublisherName       [ 128 ] __attribute__(  ( packed )  );
 u8               m_PreparerName        [ 128 ] __attribute__(  ( packed )  );
 u8               m_ApplicationName     [ 128 ] __attribute__(  ( packed )  );
 u8               m_CopyrightFileName   [  37 ] __attribute__(  ( packed )  );
 u8               m_AbstractFileName    [  37 ] __attribute__(  ( packed )  );
 u8               m_BibliographyFileName[  37 ] __attribute__(  ( packed )  );
 asciiDate        m_CreationDate                __attribute__(  ( packed )  );
 asciiDate        m_ModificationDate            __attribute__(  ( packed )  );
 asciiDate        m_EffectiveDate               __attribute__(  ( packed )  );
 asciiDate        m_ExpirationDate              __attribute__(  ( packed )  );
 u8               m_Reserved10                  __attribute__(  ( packed )  );
 u8               m_Reserved11[ 1166 ]          __attribute__(  ( packed )  );

} cdVolDesc;

typedef struct dirTableEntry {

 u8  m_DirNameLength __attribute__(  ( packed )  );
 u8  m_Reserved      __attribute__(  ( packed )  );
 u32 m_DirTOCLBA     __attribute__(  ( packed )  );
 u16 m_DirDepth      __attribute__(  ( packed )  );
 u8  m_DirName[ 32 ] __attribute__(  ( packed )  );

} dirTableEntry;

typedef struct dirTocEntry {

 short         m_Length          __attribute__(  ( packed )  );
 unsigned int  m_FileLBA         __attribute__(  ( packed )  );
 unsigned int  m_FileLBABE       __attribute__(  ( packed )  );
 unsigned int  m_FileSize        __attribute__(  ( packed )  );
 unsigned int  m_FileSizeBE      __attribute__(  ( packed )  );
 unsigned char m_DateStamp[ 6 ]  __attribute__(  ( packed )  );
 unsigned char m_Reserved1       __attribute__(  ( packed )  );
 unsigned char m_FileProperties  __attribute__(  ( packed )  );
 unsigned char m_Reserved2[ 6 ]  __attribute__(  ( packed )  );
 unsigned char m_FilenameLength  __attribute__(  ( packed )  );
 unsigned char m_Filename[ 128 ] __attribute__(  ( packed )  );

} dirTocEntry;

typedef struct fdtable {

 int          m_FD;
 unsigned int m_FileSize;
 unsigned int m_LBA;
 unsigned int m_FilePos;

} fdtable;

typedef struct dir_cache_info {

 char         m_Pathname[ 1024 ];  // The pathname of the cached directory
 unsigned int m_Valid;             // TRUE if cache data is valid, FALSE if not
 unsigned int m_PathDepth;         // The path depth of the cached directory (0 = root)
 unsigned int m_SectorStart;       // The start sector (LBA) of the cached directory
 unsigned int m_SectorNum;         // The total size of the directory (in sectors)
 unsigned int m_CacheOffset;       // The offset from sector_start of the cached area
 unsigned int m_CacheSize;         // The size of the cached directory area (in sectors)
 char*        cache;               // The actual cached data

} dir_cache_info;

enum Cache_getMode { CACHE_START = 0, CACHE_NEXT = 1 };

static struct cdVolDesc   CDVolDesc;
static unsigned int*      buffer;
static SifRpcDataQueue_t  qd;
static SifRpcServerData_t sd0;
static fdtable            s_FDTable[ 16 ];
static int                s_FDUsed [ 16 ];
static int                s_FilesOpen;
static iop_io_device_t    file_driver;
static dir_cache_info     CachedDirInfo;
static cd_read_mode_t     s_CDReadMode;
static unsigned int       s_LastSector;
static unsigned int       s_LastBk;
static cd_read_mode_t     s_DVDReadMode;
static int                s_DVDVSupport;
static char               s_DirName[ 1024 ];
static char               s_Buffer[ 9 * 2064 ];

static int ( *Func_Open   ) ( iop_io_file_t*, const char* );
static int ( *Func_Read   ) ( iop_io_file_t*, void*, int  );
static int ( *Func_DOpen  ) ( iop_io_file_t*, const char* );
static int ( *Func_DRead  ) ( iop_io_file_t*, void*       );
static int ( *Func_DClose ) ( iop_io_file_t*              );

static int CDVD_init  ( iop_io_device_t*                      );
static int CDVD_open  ( iop_io_file_t*, const char*, int, ... );
static int CDVD_lseek ( iop_io_file_t*, unsigned long, int    );
static int CDVD_read  ( iop_io_file_t*, void*, int            );
static int CDVD_write ( iop_io_file_t*, void*, int            );
static int CDVD_close ( iop_io_file_t*                        );

static int   CDVD_findfile      ( const char*, TocEntry* );
static void* CDVDRpc_Stop       ( void                   );
static void* CDVDRpc_FlushCache ( void                   );

static int strcasecmp ( const char*, const char* );

static void _splitpath   ( const char*, char*, char* );
static void TocEntryCopy ( TocEntry*, dirTocEntry*   );

static enum PathMatch ComparePath ( const char* );

static int CDVD_Cache_Dir ( const char*, enum Cache_getMode );
static int FindPath       ( char*                           );

static void* CDVD_rpc_server ( int, void*, int );
static void  CDVD_Thread     ( void*           );

static int ReadSect ( u32 aStartSector, u32 anSectors, void* apBuf, cd_read_mode_t* aMode ) {

 int lRetry;
 int lResult = 0;

 s_CDReadMode.trycount = 32;
	
 for ( lRetry=0; lRetry < 32; ++lRetry ) {

  if ( lRetry <= 8 )

   s_CDReadMode.spindlctrl = 1;

  else s_CDReadMode.spindlctrl = 0;
	
  CdDiskReady ( 0 );

  if (  CdRead ( aStartSector, anSectors, apBuf, aMode ) != TRUE  ) {

   if ( lRetry == 31 ) {

    memset ( apBuf, 0, anSectors << 11 );
    return FALSE;

   }  /* end if */

  } else {

   CdSync ( 0 );
   break;

  }  /* end else */
	
  lResult = CdGetError ();

  if ( !lResult ) break;

 }  /* end for */

 s_CDReadMode.trycount   = 32;
 s_CDReadMode.spindlctrl =  1;

 if ( !lResult ) return TRUE;

 memset ( apBuf, 0, anSectors << 11 );

 return FALSE;

}  /* end ReadSect */

static int ReadDVDVSectors ( u32 aStartSector, u32 anSectors, void* apBuf ) {

 int retVal;

 sceCdDiskReady ( 0 );

 retVal = sceCdReadDVDV ( aStartSector, anSectors, apBuf, &s_DVDReadMode );

 sceCdSync ( 0 );

 return retVal;

}  /* end ReadDVDVSectors */
/*************************************************************
* The functions below are the normal file-system operations, *
* used to provide a standard filesystem interface. There is  *
* no need to export these functions for calling via RPC      *
*************************************************************/
int CDVD_init ( iop_io_device_t* apDriver ) {

 printf ( "CDVD: CDVD Filesystem v2.0\n"                             );
 printf ( "by A.Lee (aka Hiryu), Nicholas Van Veen (aka Sjeep)...\n" );
 printf ( "...and Eugene Plotnikov (aka EEUG)\n"                     );
 printf ( "CDVD: Initializing '%s' file driver.\n", apDriver -> name );

 CdInit ( 1 );

 memset (  s_FDTable, 0, sizeof ( s_FDTable )  );
 memset (  s_FDUsed,  0, 16 * 4                );

 s_DVDVSupport             =  1;
 s_DVDReadMode.trycount    =  5;
 s_DVDReadMode.spindlctrl  = 11;
 s_DVDReadMode.datapattern = CdSecS2048;
 s_DVDReadMode.pad         = 0x00;

 return 0;

}  /* end CDVD_init */

static int _AllocFD ( unsigned int aPos, unsigned int aSize ) {

 int i;

 for ( i = 0; i < 16; ++i ) if ( s_FDUsed[ i ] == 0 ) break;

 if ( i >= 16 ) return -ENFILE;

 s_FDUsed[ i ] = 1;
 ++s_FilesOpen;

 s_FDTable[ i ].m_FD       = i;
 s_FDTable[ i ].m_FileSize = aSize;
 s_FDTable[ i ].m_LBA      = aPos;
 s_FDTable[ i ].m_FilePos  = 0;

 return i;

}  /* end _AllocFD */

static int _LookupFD ( int aFD ) {

 int i;

 for ( i = 0; i < 16; ++i ) if ( s_FDTable[ i ].m_FD == aFD ) break;

 return i < 16 ? i : -EBADF;

}  /* end _LookupFD */

static int ISO_Open ( iop_io_file_t* apFile, const char* name ) {

 static struct TocEntry tocEntry;

 int i;

 if (  CDVD_findfile ( name, &tocEntry ) != TRUE ) return -ENOENT;

 i = _AllocFD ( tocEntry.m_FileLBA, tocEntry.m_FileSize );

 apFile -> privdata = ( void* )i;

 return i;

}  /* end ISO_Open */

static int CDVD_open ( iop_io_file_t* apFile, const char* apName, int aMode, ... ) {

 if ( aMode != O_RDONLY ) return -EACCES;

 return Func_Open ( apFile, apName );

}  /* end CDVD_open */

static int CDVD_lseek ( iop_io_file_t* apFile, unsigned long offset, int whence ) {

 int i = _LookupFD (  ( int )apFile -> privdata  );

 if ( i < 0 ) return i;

 switch ( whence ) {

  case SEEK_SET:

   s_FDTable[ i ].m_FilePos = offset;

  break;

  case SEEK_CUR:

   s_FDTable[ i ].m_FilePos += offset;

  break;

  case SEEK_END:

   s_FDTable[ i ].m_FilePos = s_FDTable[ i ].m_FileSize + offset;

  break;

  default: return -EINVAL;

 }  /* end switch */

 if ( s_FDTable[ i ].m_FilePos < 0                         ) s_FDTable[ i ].m_FilePos = 0;
 if ( s_FDTable[ i ].m_FilePos > s_FDTable[ i ].m_FileSize ) s_FDTable[ i ].m_FilePos = s_FDTable[ i ].m_FileSize;

 return s_FDTable[ i ].m_FilePos;

}  /* end CDVD_lseek */

static int ISO_Read (  iop_io_file_t* apFile, void* buffer, int size ) {

 int          i;
 unsigned int start_sector;
 unsigned int off_sector;
 unsigned int num_sectors;
 unsigned int read = 0;

 i = _LookupFD (  ( int )apFile -> privdata  );

 if ( i < 0 ) return i;

 if ( s_FDTable[ i ].m_FilePos > s_FDTable[ i ].m_FileSize ) return 0;

 if (  ( s_FDTable[ i ].m_FilePos + size ) > s_FDTable[ i ].m_FileSize )

  size = s_FDTable[ i ].m_FileSize - s_FDTable[ i ].m_FilePos;

 if ( size <= 0 ) return 0;

 if ( size > 16384 ) size = 16384; 

 start_sector = s_FDTable[ i ].m_LBA + ( s_FDTable[ i ].m_FilePos >> 11 );
 off_sector   = s_FDTable[ i ].m_FilePos & 0x7FF;

 num_sectors = off_sector + size;
 num_sectors = ( num_sectors >> 11 ) + (  ( num_sectors & 2047 ) != 0  );

 if ( start_sector == s_LastSector ) {

  read = 1;

  if ( s_LastBk > 0 ) memcpy ( s_Buffer, s_Buffer + 2048 * s_LastBk, 2048 );

  s_LastBk = 0;

 }  /* end if */
	
 s_LastSector = start_sector + num_sectors - 1;

 if (  read == 0 || ( read == 1 && num_sectors > 1 )  ) {

  ReadSect (
   start_sector + read, num_sectors - read, s_Buffer + ( read << 11 ),
   &s_CDReadMode
  );
	 
  s_LastBk = num_sectors - 1;

 }  // end if

 memcpy ( buffer, s_Buffer + off_sector, size );

 s_FDTable[ i ].m_FilePos += size;

 return size;

}  /* end ISO_Read */

static int CDVD_read ( iop_io_file_t* apFile, void* apBuff, int aSize ) {

 return Func_Read ( apFile, apBuff, aSize );

}  /* end CDVD_read */

int CDVD_write ( iop_io_file_t* apDile, void* apBuffer, int aSize ) {

 return -EACCES;

}  /* end CDVD_write */

int CDVD_close ( iop_io_file_t* apFile ) {

 int i = _LookupFD (  ( int )apFile -> privdata  );

 if ( i < 0 ) return i;

 s_FDUsed[ i ] = 0;
 --s_FilesOpen;

 return 0;

}  /* end CDVD_close */
/**************************************************************
* The functions below are not exported for normal file-system *
* operations, but are used by the file-system operations, and *
* may also be exported  for use via RPC                       *
**************************************************************/
void CDVD_GetVolumeDescriptor ( void ) {

 struct cdVolDesc localVolDesc;

 int volDescSector;

 for ( volDescSector = 16; volDescSector < 20; ++volDescSector ) {

  ReadSect ( volDescSector, 1, &localVolDesc, &s_CDReadMode );

  if (  strncmp ( localVolDesc.m_VolID, "CD001", 5 ) == 0  ) {

   if ( localVolDesc.m_FSType == 1 ||
        localVolDesc.m_FSType == 2
   ) memcpy (  &CDVolDesc, &localVolDesc, sizeof ( cdVolDesc )  );

  } else break;

 }  /* end for */

}  /* end CDVD_GetVolumeDescriptor */

static int CDVD_findfile ( const char* fname, struct TocEntry* tocEntry ) {

 static char filename[  128 + 1 ];
 static char pathname[ 1024 + 1 ];

 dirTocEntry* tocEntryPointer;

 _splitpath ( fname, pathname, filename );

 if (  CachedDirInfo.m_Valid  && ComparePath ( pathname ) == MATCH  ) {

  ( char* )tocEntryPointer = CachedDirInfo.cache;

  for (  ; ( char* )tocEntryPointer < (  CachedDirInfo.cache + ( CachedDirInfo.m_CacheSize * 2048 )  );
           ( char* )tocEntryPointer += tocEntryPointer -> m_Length
  ) {

   if ( !tocEntryPointer -> m_Length )

    ( char* )tocEntryPointer = CachedDirInfo.cache + (     (    (   (  ( char* )tocEntryPointer - CachedDirInfo.cache  ) / 2048   ) + 1    ) * 2048     );

   if (   ( char* )tocEntryPointer >= (  CachedDirInfo.cache + ( CachedDirInfo.m_CacheSize * 2048 )  )   ) break;

   if (  !( tocEntryPointer -> m_FileProperties & 0x02 )  ) {

    TocEntryCopy ( tocEntry, tocEntryPointer );

    if (  strcasecmp ( tocEntry -> m_Filename, filename ) == 0  ) return TRUE;

   }  /* end if */

  }  /* end for */

  if ( CachedDirInfo.m_CacheSize == CachedDirInfo.m_SectorNum ) return FALSE;

  if ( !CachedDirInfo.m_CacheOffset ) {

   if (  !CDVD_Cache_Dir ( pathname, CACHE_NEXT )  ) return FALSE;

  } else if (  !CDVD_Cache_Dir ( pathname, CACHE_START )  ) return FALSE;

 } else {

  if (  !CDVD_Cache_Dir ( pathname, CACHE_START )   ) return FALSE;

 }  /* end else */

 while ( CachedDirInfo.m_CacheSize > 0 ) {

  ( char* )tocEntryPointer = CachedDirInfo.cache;
		
  if ( !CachedDirInfo.m_CacheOffset ) ( char* )tocEntryPointer += tocEntryPointer -> m_Length;

  for (  ; ( char* )tocEntryPointer < (  CachedDirInfo.cache + ( CachedDirInfo.m_CacheSize * 2048 )  );
           ( char* )tocEntryPointer += tocEntryPointer -> m_Length
  ) {

   if ( !tocEntryPointer -> m_Length )

    ( char* )tocEntryPointer = CachedDirInfo.cache + (     (    (   (  ( char* )tocEntryPointer - CachedDirInfo.cache  ) / 2048   ) + 1    ) * 2048     );

   if (   ( char* )tocEntryPointer >= (  CachedDirInfo.cache + ( CachedDirInfo.m_CacheSize * 2048 )  )   ) break;

   TocEntryCopy ( tocEntry, tocEntryPointer );

   if (  !strcasecmp ( tocEntry -> m_Filename, filename )  ) return TRUE;

  }  /* end for */
		
  CDVD_Cache_Dir ( pathname, CACHE_NEXT );

 }  /* end while */

 return FALSE;

}  /* end CDVD_findfile */

int CDVD_Cache_Dir ( const char* apPath, enum Cache_getMode aMode ) {

 int lPathLen;

 if ( CachedDirInfo.m_Valid ) {

  if (  ComparePath ( apPath ) == MATCH  ) {

   if ( aMode == CACHE_START ) {

    if ( !CachedDirInfo.m_CacheOffset )
					
     return CachedDirInfo.m_Valid = TRUE;

    else {

     CachedDirInfo.m_CacheOffset = 0;
     CachedDirInfo.m_CacheSize   = CachedDirInfo.m_SectorNum;

     if ( CachedDirInfo.m_CacheSize > MAX_DIR_CACHE_SECTORS ) CachedDirInfo.m_CacheSize = MAX_DIR_CACHE_SECTORS;

     if (  !ReadSect (
             CachedDirInfo.m_SectorStart + CachedDirInfo.m_CacheOffset,
             CachedDirInfo.m_CacheSize, CachedDirInfo.cache, &s_CDReadMode
            )
     ) return CachedDirInfo.m_Valid = FALSE;

     return CachedDirInfo.m_Valid = TRUE;

    }  /* end else */

   } else {

    CachedDirInfo.m_CacheOffset += CachedDirInfo.m_CacheSize;
    CachedDirInfo.m_CacheSize    = CachedDirInfo.m_SectorNum - CachedDirInfo.m_CacheOffset;

    if ( CachedDirInfo.m_CacheSize > MAX_DIR_CACHE_SECTORS ) CachedDirInfo.m_CacheSize = MAX_DIR_CACHE_SECTORS;

    if (  !ReadSect (
            CachedDirInfo.m_SectorStart + CachedDirInfo.m_CacheOffset,
            CachedDirInfo.m_CacheSize, CachedDirInfo.cache, &s_CDReadMode
           )
    ) return CachedDirInfo.m_Valid = FALSE;

    return CachedDirInfo.m_Valid = TRUE;

   }  /* end else */

  } else {

   if (  ComparePath ( apPath ) == SUBDIR  ) {

    if ( CachedDirInfo.m_CacheOffset ) {

     CachedDirInfo.m_CacheOffset = 0;
     CachedDirInfo.m_CacheSize   = CachedDirInfo.m_SectorNum;

     if ( CachedDirInfo.m_CacheSize > MAX_DIR_CACHE_SECTORS ) CachedDirInfo.m_CacheSize = MAX_DIR_CACHE_SECTORS;

     if (  !ReadSect (
             CachedDirInfo.m_SectorStart + CachedDirInfo.m_CacheOffset,
             CachedDirInfo.m_CacheSize, CachedDirInfo.cache, &s_CDReadMode
            )
     ) return CachedDirInfo.m_Valid = FALSE;

    }  /* end if */

    lPathLen = strlen ( CachedDirInfo.m_Pathname );
    strcpy ( s_DirName, apPath + lPathLen );

    return FindPath ( s_DirName );

   }  /* end if */

  }  /* end else */

 }  /* end if */

 CdDiskReady ( 0 );
 CDVD_GetVolumeDescriptor ();

 CachedDirInfo.m_PathDepth     = 0;
 CachedDirInfo.m_Pathname[ 0 ] = '\x00';
 CachedDirInfo.m_CacheOffset   = 0;
 CachedDirInfo.m_SectorStart   = CDVolDesc.m_RootToc.tocLBA;
 CachedDirInfo.m_SectorNum     = ( CDVolDesc.m_RootToc.tocSize >> 11 ) + (  ( CDVolDesc.m_RootToc.tocSize & 2047 ) != 0  );
 CachedDirInfo.m_CacheSize     = CachedDirInfo.m_SectorNum;

 if ( CachedDirInfo.m_CacheSize > MAX_DIR_CACHE_SECTORS ) CachedDirInfo.m_CacheSize = MAX_DIR_CACHE_SECTORS;

 if (  !ReadSect (
         CachedDirInfo.m_SectorStart + CachedDirInfo.m_CacheOffset,
         CachedDirInfo.m_CacheSize, CachedDirInfo.cache, &s_CDReadMode
        )
 ) return CachedDirInfo.m_Valid = FALSE;

 strcpy ( s_DirName, apPath );

 return FindPath ( s_DirName );

}  /* end CDVD_Cache_Dir */

static int FindPath ( char* pathname ) {

 struct TocEntry     localTocEntry;
 char*               lDirName;
 char*               seperator;
 int                 dir_entry;
 int                 found_dir;
 struct dirTocEntry* tocEntryPointer;

 lDirName = strtok ( pathname, "\\/" );

 CdDiskReady ( 0 );

 while ( lDirName ) {

  found_dir = FALSE;

  ( char* )tocEntryPointer  = CachedDirInfo.cache;
  ( char* )tocEntryPointer += tocEntryPointer -> m_Length;

  dir_entry = 0;

  for (  ; ( char* )tocEntryPointer < (  CachedDirInfo.cache + ( CachedDirInfo.m_CacheSize * 2048 )  );
           ( char* )tocEntryPointer += tocEntryPointer -> m_Length
  ) {

   if ( !tocEntryPointer -> m_Length )

    ( char* )tocEntryPointer = CachedDirInfo.cache + (     (    (   (  ( char* )tocEntryPointer - CachedDirInfo.cache  ) / 2048   ) + 1    ) * 2048     );

   if (   ( char* )tocEntryPointer >= (  CachedDirInfo.cache + ( CachedDirInfo.m_CacheSize * 2048 )  )   ) {

    if (  ( CachedDirInfo.m_CacheOffset + CachedDirInfo.m_CacheSize ) < CachedDirInfo.m_SectorNum  ) {

     CachedDirInfo.m_CacheOffset += CachedDirInfo.m_CacheSize;
     CachedDirInfo.m_CacheSize    = CachedDirInfo.m_SectorNum - CachedDirInfo.m_CacheOffset;

     if ( CachedDirInfo.m_CacheSize > MAX_DIR_CACHE_SECTORS )

      CachedDirInfo.m_CacheSize = MAX_DIR_CACHE_SECTORS;

     if (  !ReadSect (
             CachedDirInfo.m_SectorStart + CachedDirInfo.m_CacheOffset,
             CachedDirInfo.m_CacheSize, CachedDirInfo.cache, &s_CDReadMode
            )
     ) return CachedDirInfo.m_Valid = FALSE;

     ( char* )tocEntryPointer = CachedDirInfo.cache;

    } else return CachedDirInfo.m_Valid = FALSE;

   }  /* end if */

   if ( tocEntryPointer -> m_FileProperties & 0x02 ) {

    TocEntryCopy ( &localTocEntry, tocEntryPointer );

    if ( !dir_entry && CachedDirInfo.m_PathDepth ) strcpy ( localTocEntry.m_Filename, ".." );

    if (  !strcasecmp ( lDirName, localTocEntry.m_Filename )  ) {

     found_dir = TRUE;

     if ( !dir_entry ) {

      if ( CachedDirInfo.m_PathDepth > 0 ) --CachedDirInfo.m_PathDepth;
						
      if ( !CachedDirInfo.m_PathDepth )

       CachedDirInfo.m_Pathname[ 0 ] = '\x00';

      else {

       seperator = strrchr ( CachedDirInfo.m_Pathname, '/' );

       if ( seperator ) *seperator = '\x00';

      }  /* end else */

     } else {

      strcat ( CachedDirInfo.m_Pathname, "/"      );
      strcat ( CachedDirInfo.m_Pathname, lDirName );

      ++CachedDirInfo.m_PathDepth;

     }  /* end else */

     break;

    }  /* end if */

   }  /* end if */

   ++dir_entry;

  }  /* end for */

  if ( !found_dir ) return CachedDirInfo.m_Valid = FALSE;

  lDirName = strtok ( NULL,"\\/" );

  CachedDirInfo.m_SectorStart = localTocEntry.m_FileLBA;
  CachedDirInfo.m_SectorNum   = ( localTocEntry.m_FileSize >> 11 ) + (  ( CDVolDesc.m_RootToc.tocSize & 2047 ) != 0  );
  CachedDirInfo.m_CacheOffset = 0;
  CachedDirInfo.m_CacheSize   = CachedDirInfo.m_SectorNum;

  if ( CachedDirInfo.m_CacheSize > MAX_DIR_CACHE_SECTORS )

   CachedDirInfo.m_CacheSize = MAX_DIR_CACHE_SECTORS;

  if (  !ReadSect (
          CachedDirInfo.m_SectorStart+CachedDirInfo.m_CacheOffset,
          CachedDirInfo.m_CacheSize, CachedDirInfo.cache, &s_CDReadMode
         )
  ) return CachedDirInfo.m_Valid = FALSE;

 }  /* end while */

 return CachedDirInfo.m_Valid = TRUE;

}  /* end FindPath */

static void CdFlushCache ( void ) {

 strcpy ( CachedDirInfo.m_Pathname, "" );  // The pathname of the cached directory
 CachedDirInfo.m_Valid       = FALSE;      // Cache is not valid
 CachedDirInfo.m_PathDepth   = 0;          // 0 = root
 CachedDirInfo.m_SectorStart = 0;          // The start sector (LBA) of the cached directory
 CachedDirInfo.m_SectorNum   = 0;          // The total size of the directory (in sectors)
 CachedDirInfo.m_CacheOffset = 0;          // The offset from sector_start of the cached area
 CachedDirInfo.m_CacheSize   = 0;          // The size of the cached directory area (in sectors)

}  /* end CdFlushCache */

static void* CDVDRpc_FlushCache ( void ) {

 CdFlushCache ();

 return NULL;

}  /* end CDVDRpc_FlushCache */

static void* CDVDRpc_Stop ( void ) {

 CdStop ();

 return NULL;

}  /* end CDVDRpc_Stop */

static void inline _copy_name ( char* apName, struct dirTocEntry* apTOCEntry ) {

 int i, lLen;

 if ( CDVolDesc.m_FSType == 2 ) {
// This is a Joliet Filesystem, so use Unicode to ISO string copy
  lLen = apTOCEntry -> m_FilenameLength / 2;

  for ( i = 0; i < lLen; ++i ) apName[ i ] = apTOCEntry -> m_Filename[ ( i << 1 ) + 1 ];

 } else {

  lLen = apTOCEntry -> m_FilenameLength;
  strncpy ( apName, apTOCEntry -> m_Filename, 128 );

 }  /* end else */

 apName[ lLen ] = 0;

 if (  !( apTOCEntry -> m_FileProperties & 0x02 )  ) strtok ( apName, ";" );

}  /* end _copy_name */

struct dirTocEntry* s_tocEntryPointer = NULL;
static int          s_DirEntry;

static int ISO_DOpen (  iop_io_file_t* apFile, const char* apName ) {

 if (  s_tocEntryPointer                                          ) return -ENFILE;
 if (  !CDVD_Cache_Dir ( apName,                   CACHE_START )  ) return -ENOENT;
 if (  !CDVD_Cache_Dir ( CachedDirInfo.m_Pathname, CACHE_START )  ) return -ENOENT;

 ( char* )s_tocEntryPointer  = CachedDirInfo.cache;
 ( char* )s_tocEntryPointer += s_tocEntryPointer -> m_Length;

 if ( CachedDirInfo.m_PathDepth == 0 ) ( char* )s_tocEntryPointer += s_tocEntryPointer -> m_Length;

 s_DirEntry = 0;

 return 0;

}  /* end ISO_DOpen */

static int CDVD_dopen ( iop_io_file_t* apFile, const char* apName ) {

 return Func_DOpen ( apFile, apName );

}  /* end CDVD_dopen */

static int ISO_DRead ( iop_io_file_t* apFile, void* apRetVal ) {

 fio_dirent_t* lpBuf = ( fio_dirent_t* )apRetVal;

 if ( !s_tocEntryPointer ) return -EPROTO;

 while ( 1 ) {

  for (   ; ( char* )s_tocEntryPointer < (  CachedDirInfo.cache + ( CachedDirInfo.m_CacheSize * 2048 )  );   ) {

   if ( s_tocEntryPointer -> m_Length == 0 )

    ( char* )s_tocEntryPointer = CachedDirInfo.cache + (((((char*)s_tocEntryPointer - CachedDirInfo.cache)/2048)+1)*2048);

   if (  ( char* )s_tocEntryPointer >= CachedDirInfo.cache + ( CachedDirInfo.m_CacheSize * 2048 )  ) break;

    if ( s_tocEntryPointer -> m_FileProperties & 0x02 )

     lpBuf -> stat.mode = FIO_SO_IFDIR;

    else lpBuf -> stat.mode = FIO_SO_IFREG;

   if ( s_DirEntry == 0 && CachedDirInfo.m_PathDepth != 0 )

    strcpy ( lpBuf -> name, ".." );

   else _copy_name ( lpBuf -> name, s_tocEntryPointer );

   ++s_DirEntry;

   ( char* )s_tocEntryPointer += s_tocEntryPointer -> m_Length;

   return 1;

  }  /* end for */

  if (   ( CachedDirInfo.m_CacheOffset + CachedDirInfo.m_CacheSize ) < CachedDirInfo.m_SectorNum  ) {

   if (  CDVD_Cache_Dir ( CachedDirInfo.m_Pathname, CACHE_NEXT ) != TRUE  ) return -ENOENT;

  } else break;

  ( char* )s_tocEntryPointer = CachedDirInfo.cache;

 }  /* end while */

 return -ENOENT;

}  /* end ISO_DRead */

static int CDVD_dread ( iop_io_file_t* apFile, void* apRetVal ) {

 return Func_DRead ( apFile, apRetVal );

}  /* end CDVD_dread */

static int ISO_DClose ( iop_io_file_t* apFile ) {

 if ( !s_tocEntryPointer ) return -EPROTO;

 s_tocEntryPointer = NULL;

 return 0;

}  /* end ISO_DClose */

static int CDVD_dclose ( iop_io_file_t* apFile ) {

 return Func_DClose ( apFile );

}  /* end CDVD_dclose */
/*************************************************
* The functions below are for internal use only, *
* and are not to be exported                     *
*************************************************/
void CDVD_Thread ( void* param ) {

 SifInitRpc ( 0 );
// 0x4800 bytes for TocEntry structures (can fit 128 of them)
// 0x400  bytes for the filename string
 buffer = ( unsigned int* )AllocSysMemory ( 0, 0x4C00, NULL );

 if ( buffer == NULL ) SleepThread ();

 SifSetRpcQueue (  &qd, GetThreadId ()  );
 SifRegisterRpc (  &sd0, CDVD_IRX, CDVD_rpc_server, ( void* )buffer, 0, 0, &qd  );
 SifRpcLoop     (  &qd );

}  /* end CDVD_Thread */

#include "SMSCDVD_UDFS.c"

static void* CDVDRpc_SetDVDV ( unsigned int* afSet ) {

 if ( *afSet ) {

  *afSet = UDFInit ();

  Func_Open   = UDF_Open;
  Func_Read   = UDF_Read;
  Func_DOpen  = UDF_DOpen;
  Func_DRead  = UDF_DRead;
  Func_DClose = UDF_DClose;

 } else {

  Func_Open   = ISO_Open;
  Func_Read   = ISO_Read;
  Func_DOpen  = ISO_DOpen;
  Func_DRead  = ISO_DRead;
  Func_DClose = ISO_DClose;

  *afSet = 1;

 }  /* end else */

 return afSet;

}  /* end CDVDRpc_SetDVDV */

void* CDVD_rpc_server ( int aFunc, void* apData, int aSize ) {

 switch ( aFunc ) {

  case CDVD_STOP      : return CDVDRpc_Stop       ();
  case CDVD_FLUSHCACHE: return CDVDRpc_FlushCache ();
  case CDVD_SETDVDV   : return CDVDRpc_SetDVDV    (  ( unsigned int* )apData  );
  case CDVD_DVDV      : *(  ( unsigned int* )apData  ) = s_DVDVSupport; return apData;

 }  /* end switch */

 return NULL;

}  /* end CDVD_rpc_server */

static void _splitpath ( const char* apPath, char* apDir, char* apFName ) {

 static char s_Path[ 1024 + 1 ];

 char* lpSlash;

 strncpy ( s_Path, apPath, 1024 );

 lpSlash = strrchr ( s_Path, '/' );

 if ( !lpSlash ) lpSlash = strrchr (  s_Path, ( int )'\\'  );

 if ( lpSlash ) {

  lpSlash[ 0 ] = 0;
  strncpy ( apDir, s_Path, 1024 );
  apDir[ 255 ] = 0;

  strncpy ( apFName, lpSlash + 1, 128 );
  apFName[ 128 ] = 0;

 } else {

  apDir[ 0 ] = 0;
  strncpy ( apFName, s_Path, 128 );
  apFName[ 128 ] = 0;

 }  /* end else */

}  /* end _splitpath */

static void TocEntryCopy ( TocEntry* apTocEntry, dirTocEntry* apIntTocEntry ) {

 int i;
 int lFNameLen;

 apTocEntry -> m_FileSize       = apIntTocEntry -> m_FileSize;
 apTocEntry -> m_FileLBA        = apIntTocEntry -> m_FileLBA;
 apTocEntry -> m_FileProperties = apIntTocEntry -> m_FileProperties;

 if ( CDVolDesc.m_FSType == 2 ) {

  lFNameLen = apIntTocEntry -> m_FilenameLength / 2;

  for ( i = 0; i < lFNameLen; ++i )

   apTocEntry -> m_Filename[ i ] = apIntTocEntry -> m_Filename[ ( i <<1 ) + 1 ];

 } else {

  lFNameLen = apIntTocEntry -> m_FilenameLength;

  strncpy ( apTocEntry -> m_Filename, apIntTocEntry -> m_Filename,128 );

 }  /* end else */

 apTocEntry -> m_Filename[ lFNameLen ] = 0;

 if (  !( apTocEntry -> m_FileProperties & 0x02 )  ) strtok ( apTocEntry -> m_Filename, ";" );

}  /* end TocEntryCopy */

static int strcasecmp ( const char *s1, const char *s2 ) {

 while (  *s1 != '\0' && tolower ( *s1 ) == tolower ( *s2 )  ) {

  ++s1;
  ++s2;

 }  /* end while */

 return tolower (  *( unsigned char* )s1  ) -
        tolower (  *( unsigned char* )s2  );

}  /* end strcasecmp */

static enum PathMatch ComparePath ( const char* path ) {

 int i, length = strlen ( CachedDirInfo.m_Pathname );

 for ( i = 0; i < length; ++i ) {

  if ( path[ i ] != CachedDirInfo.m_Pathname[ i ] ) {

   if ( path[ i ] == '/' || path[ i ] == '\\' )

    if ( CachedDirInfo.m_Pathname[ i ] == '/' || CachedDirInfo.m_Pathname[ i ] == '\\' ) continue;

   return NOT_MATCH;

  }  /* end if */

 }  /* end for */

 if ( path[ length ] == 0 ) return MATCH;

 return path[ length ] == '/' || path[ length ] == '\\' ? SUBDIR : NOT_MATCH;

}  /* end ComparePath */

static int CDVD_deinit ( iop_io_device_t* apDev ) {

 return -ENOTSUP;

}  /* end CDVD_deinit */

static int CDVD_format ( iop_io_file_t* apFile, ... ) {

 return -ENOTSUP;

}  /* end CDVD_format */

static int CDVD_ioctl ( iop_io_file_t* apFile, unsigned long aCmd, void* apArg ) {

 return -ENOTSUP;

}  /* end CDVD_ioctl */

static int CDVD_dummy_file ( iop_io_file_t* apFile, const char* apName ) {

 return -EACCES;

}  /* end CDVD_dummy_file */

static int CDVD_getstat ( iop_io_file_t* apFile, const char* apName, void* apRetVal ) {

 return -ENOTSUP;

}  /* end CDVD_getstat */

static int CDVD_chstat ( iop_io_file_t* apFile, const char* apName, void* apPtr, unsigned int aVal ) {

 return -ENOTSUP;

}  /* end CDVD_chstat */

static iop_io_device_ops_t s_CDFSOps;

int _start ( int argc, char** argv ) {

 iop_thread_t param;
 int          th;

 strcpy ( CachedDirInfo.m_Pathname, "" );  // The pathname of the cached directory
 CachedDirInfo.m_Valid       = FALSE;      // Cache is not valid
 CachedDirInfo.m_PathDepth   = 0;          // 0 = root
 CachedDirInfo.m_SectorStart = 0;          // The start sector (LBA) of the cached directory
 CachedDirInfo.m_SectorNum   = 0;          // The total size of the directory (in sectors)
 CachedDirInfo.m_CacheOffset = 0;          // The offset from sector_start of the cached area
 CachedDirInfo.m_CacheSize   = 0;          // The size of the cached directory area (in sectors)

 if ( CachedDirInfo.cache == NULL ) CachedDirInfo.cache = ( char* )AllocSysMemory ( 0, MAX_DIR_CACHE_SECTORS * 2048, NULL );

 s_CDReadMode.trycount    = 0;
 s_CDReadMode.spindlctrl  = CdSpinStm;
 s_CDReadMode.datapattern = CdSecS2048;

 file_driver.name    = "cdfs";
 file_driver.type    = 16;
 file_driver.version = 1;
 file_driver.desc    = "CDVD Filedriver";
 file_driver.ops     = &s_CDFSOps;

 s_CDFSOps.io_init    = CDVD_init;
 s_CDFSOps.io_deinit  = CDVD_deinit;
 s_CDFSOps.io_format  = CDVD_format;
 s_CDFSOps.io_open    = CDVD_open;
 s_CDFSOps.io_close   = CDVD_close;
 s_CDFSOps.io_read    = CDVD_read;
 s_CDFSOps.io_write   = CDVD_write;
 s_CDFSOps.io_lseek   = CDVD_lseek;
 s_CDFSOps.io_ioctl   = CDVD_ioctl;
 s_CDFSOps.io_remove  = CDVD_dummy_file;
 s_CDFSOps.io_mkdir   = CDVD_dummy_file;
 s_CDFSOps.io_rmdir   = CDVD_dummy_file;
 s_CDFSOps.io_dopen   = CDVD_dopen;
 s_CDFSOps.io_dclose  = CDVD_dclose;
 s_CDFSOps.io_dread   = CDVD_dread;
 s_CDFSOps.io_getstat = CDVD_getstat;
 s_CDFSOps.io_chstat  = CDVD_chstat;

 io_DelDrv ( "cdfs"       );
 io_AddDrv ( &file_driver );

 param.attr      = TH_C;
 param.thread    = ( void* )CDVD_Thread;
 param.priority  = 40;
 param.stacksize = 0x8000;
 param.option    = 0;
 th = CreateThread ( &param );

 if ( th > 0 ) {

  StartThread ( th, 0 );

  CDVDRpc_SetDVDV ( 0 );

  return MODULE_RESIDENT_END;

 } else	return MODULE_NO_RESIDENT_END;

}  /* end _start */
