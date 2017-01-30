#include "ums_fat.h"
#include "ums_driver.h"
#include "ums_cache.h"

#include <errno.h>
#include <io_common.h>
#include <smsutils.h>
#include <sys/stat.h>
#include <sysmem.h>
#include <sysclib.h>
#include <stdio.h>

#define _SIZE_( a )  (   sizeof (  ( a )  ) / sizeof (  ( a )[ 0 ]  )   )

#define FAT12 0
#define FAT16 1
#define FAT32 2

#define MAX_FILE          4
#define MAX_DIR_CLUSTER 512
#define FAT_MAX_NAME    128
#define DIR_CHAIN_SIZE   32

typedef struct part_raw_record {
 unsigned char m_Active                __attribute__(  ( packed )  );
 unsigned char m_StartHead             __attribute__(  ( packed )  );
 unsigned char m_StartSectorTrack[ 2 ] __attribute__(  ( packed )  );
 unsigned char m_SID                   __attribute__(  ( packed )  );
 unsigned char m_EndHead               __attribute__(  ( packed )  );
 unsigned char m_EndSectorTrack[ 2 ]   __attribute__(  ( packed )  );
 unsigned int  m_StartLBA              __attribute__(  ( packed )  );
 unsigned int  m_Size                  __attribute__(  ( packed )  );
} part_raw_record __attribute__(  ( packed )  );

typedef struct fat_raw_bpb {
 unsigned char  m_JMP[ 3 ]       __attribute__(  ( packed )  );
 unsigned char  m_OEM[ 8 ]       __attribute__(  ( packed )  );
 unsigned short m_SectorSize     __attribute__(  ( packed )  );
 unsigned char  m_ClusterSize    __attribute__(  ( packed )  );
 unsigned short m_nResSectors    __attribute__(  ( packed )  );
 unsigned char  m_nFATs          __attribute__(  ( packed )  );
 unsigned short m_RootSize       __attribute__(  ( packed )  );
 unsigned short m_nTotalSectors  __attribute__(  ( packed )  );
 unsigned char  m_MediaDesc      __attribute__(  ( packed )  );
 unsigned short m_FATSize        __attribute__(  ( packed )  );
 unsigned short m_TrackSize      __attribute__(  ( packed )  );
 unsigned short m_nHeads         __attribute__(  ( packed )  );
 unsigned int   m_nHidden        __attribute__(  ( packed )  );
 unsigned int   m_nSectors       __attribute__(  ( packed )  );
 unsigned char  m_DriveNr        __attribute__(  ( packed )  );
 unsigned char  m_Res            __attribute__(  ( packed )  );
 unsigned char  m_Sign           __attribute__(  ( packed )  );
 unsigned int   m_Serial         __attribute__(  ( packed )  );
 unsigned char  m_VolLabel[ 11 ] __attribute__(  ( packed )  );
 unsigned char  m_FATID[ 8 ]     __attribute__(  ( packed )  );
} fat_raw_bpb __attribute__(  ( packed )  );

typedef struct fat32_raw_bpb {
 unsigned char  m_Jmp[ 3 ]         __attribute__(  ( packed )  );
 unsigned char  m_OEM[ 8 ]         __attribute__(  ( packed )  );
 unsigned short m_SectorSize       __attribute__(  ( packed )  );
 unsigned char  m_ClusterSize      __attribute__(  ( packed )  );
 unsigned short m_nResSectors      __attribute__(  ( packed )  );
 unsigned char  m_nFATs            __attribute__(  ( packed )  );
 unsigned short m_RootSize         __attribute__(  ( packed )  );
 unsigned short m_nTotalSectors    __attribute__(  ( packed )  );
 unsigned char  m_MediaDesc        __attribute__(  ( packed )  );
 unsigned short m_FATSize          __attribute__(  ( packed )  );
 unsigned short m_TrackSize        __attribute__(  ( packed )  );
 unsigned short m_nHeads           __attribute__(  ( packed )  );
 unsigned int   m_nHidden          __attribute__(  ( packed )  );
 unsigned int   m_nSectors         __attribute__(  ( packed )  );
 unsigned int   m_FATSize32        __attribute__(  ( packed )  );
 unsigned short m_FATStatus        __attribute__(  ( packed )  );
 unsigned short m_Revision         __attribute__(  ( packed )  );
 unsigned int   m_RootDirCluster   __attribute__(  ( packed )  );
 unsigned short m_FSInfoSector     __attribute__(  ( packed )  );
 unsigned short m_BootSectorCopy   __attribute__(  ( packed )  );
 unsigned char  m_Res1[ 12 ]       __attribute__(  ( packed )  );
 unsigned char  m_PDN              __attribute__(  ( packed )  );
 unsigned char  m_Res2             __attribute__(  ( packed )  );
 unsigned char  m_Sign             __attribute__(  ( packed )  );
 unsigned int   m_Serial           __attribute__(  ( packed )  );
 unsigned char  m_VolLabel[ 11 ]   __attribute__(  ( packed )  );
 unsigned char  m_FATID[ 8 ]       __attribute__(  ( packed )  );
 unsigned char  m_MachineCode[ 8 ] __attribute__(  ( packed )  );
 unsigned short m_BootSign         __attribute__(  ( packed )  );
} fat32_raw_bpb __attribute__(  ( packed )  );

typedef struct _part_record {
 unsigned int m_Start;
 unsigned int m_Count;
} part_record;

typedef struct fat_direntry_short {
 unsigned char  m_Name[ 8 ]   __attribute__(  ( packed )  );
 unsigned char  m_Ext [ 3 ]   __attribute__(  ( packed )  );
 unsigned char  m_Attr        __attribute__(  ( packed )  );
 unsigned char  m_ResNT       __attribute__(  ( packed )  );
 unsigned char  m_Sec         __attribute__(  ( packed )  );
 unsigned char  m_CTime[ 2 ]  __attribute__(  ( packed )  );
 unsigned char  m_CDate[ 2 ]  __attribute__(  ( packed )  );
 unsigned char  m_ADate[ 2 ]  __attribute__(  ( packed )  );
 unsigned short m_ClusterHigh __attribute__(  ( packed )  );
 unsigned char  m_WTime[ 2 ]  __attribute__(  ( packed )  );
 unsigned char  m_WDate[ 2 ]  __attribute__(  ( packed )  );
 unsigned short m_ClusterLow  __attribute__(  ( packed )  );
 unsigned int   m_Size        __attribute__(  ( packed )  );
} fat_direntry_short __attribute__(  ( packed )  );

typedef struct fat_direntry_long {
 unsigned char m_EntrySeq;
 unsigned char m_Name1[ 10 ];
 unsigned char m_RSHV;
 unsigned char m_Res1;
 unsigned char m_Cks;
 unsigned char m_Name2[ 12 ];
 unsigned char m_Res2[ 2 ];
 unsigned char m_Name3[ 4 ];
} fat_direntry_long;

typedef struct fat_direntry {
 unsigned char m_Attr;
 unsigned char m_Name[ FAT_MAX_NAME ];
 unsigned char m_SName[ 13 ];
 unsigned int  m_Size;
 unsigned int  m_Cluster;
} fat_direntry;

typedef struct fat_dir_chain_record {
 unsigned int m_Cluster;
 unsigned int m_Index;
} fat_dir_chain_record;

typedef struct fat_dir {
 unsigned char        m_Attr;
 unsigned char        m_Name[ FAT_MAX_NAME ];
 unsigned char        m_CDate[ 4 ];
 unsigned char        m_CTime[ 3 ];
 unsigned char        m_ADate[ 4 ];
 unsigned char        m_ATime[ 3 ];
 unsigned char        m_MDate[ 4 ];
 unsigned char        m_MTime[ 3 ];
 unsigned int         m_Size;
 unsigned int         m_LastCluster;
 fat_dir_chain_record m_Chain[ DIR_CHAIN_SIZE ];
} fat_dir;

typedef struct file_record {
 int          m_FD;
 unsigned int m_Pos;
 int          m_Flag;
 int          m_Mode;
 fat_dir      m_Dir;
} file_record;

typedef struct find_file_info {
 int         m_FileFlag;
 int         m_Status;
 USBMDevice* m_pDev;
 fat_dir     m_Dir;
} find_file_info;

typedef struct FATFS {
 unsigned int  m_Start;
 unsigned int  m_LastChainCluster;
          int  m_LastChainResult;
 unsigned int  m_DirentryCluster;
          int  m_DirentryIndex;
 part_record   m_PartRec;
 unsigned int  m_ClusterBuf[ MAX_DIR_CLUSTER ];
 unsigned char m_FATID[ 9 ];
 unsigned char m_FAType;
 unsigned char m_ClusterSize;
 unsigned char m_nFATs;
 unsigned int  m_nResSectors;
 unsigned int  m_RootSize;
 unsigned int  m_FATSize;
 unsigned int  m_TrackSize;
 unsigned int  m_nHeads;
 unsigned int  m_nHidden;
 unsigned int  m_nSectors;
 unsigned int  m_RootDirStart;
 unsigned int  m_RootDirCluster;
 unsigned int  m_ActiveFAT;
 file_record   m_File[ MAX_FILE ];
 unsigned int  m_Counter;

 int          ( *get_cluster_chain ) ( USBMDevice*, unsigned int, unsigned int*, int, int );
 unsigned int ( *cluster2sector    ) ( USBMDevice*, unsigned int                          ); 

 unsigned int  m_Serial;

} FATFS;

static int strcmpi ( const char* apLeft, const char* apRight ) {
 int lLeft,lRight;
 do {
  lLeft  = tolower (  ( unsigned char )*apLeft++  );
  lRight = tolower (  ( unsigned char )*apRight++ );
 } while ( lLeft && ( lLeft == lRight )  );
 return lLeft - lRight;
}  /* end strcmpi */

static int _check_media_change ( USBMDevice* apDev, FATFS* apFS ) {

 int   retVal = 0;
 char* lpBuf       = (  ( char* )apFS  ) + sizeof ( *apFS );
 int   lSectorSize = apDev -> m_SectorSize;

 if (  !apDev -> m_Status && UmsRead ( apDev, apFS -> m_PartRec.m_Start, lpBuf, lSectorSize ) == lSectorSize && *( unsigned short* )&lpBuf[ 510 ] == 0xAA55  ) {

  int lSerial = apFS -> m_FAType != FAT32 ? (  ( fat_raw_bpb*   )lpBuf  ) -> m_Serial
                                          : (  ( fat32_raw_bpb* )lpBuf  ) -> m_Serial;

  if ( lSerial == apFS -> m_Serial ) retVal = 1;

 }  /* end if */

 return retVal;

}  /* end _check_media_change */

static unsigned int inline _root_dir_sectors ( USBMDevice* apDev, FATFS* apFS ) {
 return (  ( apFS -> m_RootSize * 32  ) +
           ( apDev -> m_SectorSize - 1 )
        ) / apDev -> m_SectorSize;
}  /* end _root_dir_sectors */

static unsigned int inline _data_clusters ( USBMDevice* apDev, FATFS* apFS ) {
 return (   apFS -> m_nSectors - (  apFS -> m_nResSectors + ( apFS -> m_nFATs * apFS -> m_FATSize ) + _root_dir_sectors ( apDev, apFS )  )   ) / apFS -> m_ClusterSize;
}  /* end _data_clusters */

static unsigned int _cluster2sector1216 ( USBMDevice* apDev, unsigned int aCluster ) {
 FATFS* lpFS = ( FATFS* )apDev -> m_FS.m_pData;
 return lpFS -> m_RootDirStart + (  lpFS -> m_RootSize / ( apDev -> m_SectorSize >> 5 )  ) + (  lpFS -> m_ClusterSize * ( aCluster - 2 )  );
}  /* end _cluster2sector1216 */

static unsigned int _cluster2sector32 ( USBMDevice* apDev, unsigned int aCluster ) {
 FATFS* lpFS = ( FATFS* )apDev -> m_FS.m_pData;
 return lpFS -> m_RootDirStart + (  lpFS -> m_ClusterSize * ( aCluster - 2 )  );
}  /* end _cluster2sector32 */

static unsigned int inline _get_cluster_record12 ( unsigned char* apBuf, int aType ) {
 if ( aType ) return ( apBuf[ 0 ] >> 4 ) + ( apBuf[ 1 ] << 4 );
 return apBuf[ 0 ] + (  ( apBuf[ 1 ] & 0x0F ) << 8  );
}  /* end _get_cluster_record12 */

static int _get_cluster_chain12 ( USBMDevice* apDev, unsigned int aCluster, unsigned int* apBuf, int aBufSize, int aStart ) {

 int            i, lRecOff, lSecSpan, lFATSec, lLastFATSec;
 FATFS*         lpFS     = ( FATFS* )apDev -> m_FS.m_pData;
 unsigned int   lSecSize = apDev -> m_SectorSize;
 unsigned int   lStart   = lpFS -> m_Start;
 unsigned char  lXBuf[ 4 ];
 unsigned char* lpBuf;

 i           =  0;
 lLastFATSec = -1;

 if ( aStart ) apBuf[ i++ ] = aCluster;

 while ( i < aBufSize ) {
  lRecOff  = ( aCluster * 3 ) / 2;
  lFATSec  = lRecOff / lSecSize;
  lSecSpan = 0;
  if (  ( lRecOff % lSecSize ) == ( lSecSize - 1 )  ) lSecSpan = 1;
  if ( lLastFATSec != lFATSec || lSecSpan ) {
   apDev -> m_pSector = CacheRead ( apDev, lStart + lFATSec );
   if ( !apDev -> m_pSector ) return -1;
   lLastFATSec = lFATSec;
   if ( lSecSpan ) {
    lXBuf[ 0 ] = apDev -> m_pSector[ lSecSize - 2 ];
    lXBuf[ 1 ] = apDev -> m_pSector[ lSecSize - 1 ];
    apDev -> m_pSector = CacheRead ( apDev, lStart + lFATSec + 1 );
    if ( !apDev -> m_pSector ) return -1;
    lXBuf[ 2 ] = apDev -> m_pSector[ 0 ];
    lXBuf[ 3 ] = apDev -> m_pSector[ 1 ];
   }  /* end if */
  }  /* end if */
  if ( lSecSpan )
   lpBuf = lXBuf + ( lRecOff % lSecSize ) - ( lSecSize - 2 );
  else lpBuf = apDev -> m_pSector + lRecOff % lSecSize;
  aCluster = _get_cluster_record12 ( lpBuf, aCluster % 2 );
  if ( aCluster >= 0x0FF8 ) break;
  apBuf[ i++ ] = aCluster;
 }  /* end while */

 if ( i >= aBufSize ) return 0;

 return i;

}  /* end _get_cluster_chain12 */

static int _get_cluster_chain16 ( USBMDevice* apDev, unsigned int aCluster, unsigned int* apBuf, int aBufSize, int aStart ) {

 int          i, lnIdx, lFATSec, lLastFATSec;
 FATFS*       lpFS     = ( FATFS* )apDev -> m_FS.m_pData;
 unsigned int lSecSize = apDev -> m_SectorSize;
 unsigned int lStart   = lpFS -> m_Start;

 i           = 0;
 lnIdx       = lSecSize / 2;
 lLastFATSec = -1;

 if ( aStart ) apBuf[ i++ ] = aCluster;

 while ( i < aBufSize ) {
  lFATSec = aCluster / lnIdx;
  if ( lLastFATSec != lFATSec ) {
   apDev -> m_pSector = CacheRead ( apDev, lStart + lFATSec );
   if ( !apDev -> m_pSector ) return -1;
   lLastFATSec = lFATSec;
  }  /* end if */
  aCluster = (    ( _u16* )(  apDev -> m_pSector + (  ( aCluster % lnIdx ) * 2  )   )    ) -> m_Val;
  if ( aCluster >= 0xFFF8 ) break;
  apBuf[ i++ ] = aCluster;
 }  /* end while */

 if ( i >= aBufSize ) return 0;

 return i;

}  /* end _get_cluster_chain16 */

static int _get_cluster_chain32 ( USBMDevice* apDev, unsigned int aCluster, unsigned int* apBuf, int aBufSize, int aStart ) {

 int          i, lnIdx, lFATSec, lLastFATSec;
 FATFS*       lpFS     = ( FATFS* )apDev -> m_FS.m_pData;
 unsigned int lSecSize = apDev -> m_SectorSize;
 unsigned int lStart   = lpFS -> m_Start;

 i           = 0;
 lnIdx       = lSecSize / 4;
 lLastFATSec = -1;

 if ( aStart ) apBuf[ i++ ] = aCluster;

 while ( i < aBufSize ) {
  lFATSec = aCluster / lnIdx;
  if ( lLastFATSec !=  lFATSec ) {
   apDev -> m_pSector = CacheRead ( apDev, lStart + lFATSec );
   if ( !apDev -> m_pSector ) return -1;
   lLastFATSec = lFATSec;
  }  /* end if */
  aCluster = (    ( _u32* )(   apDev -> m_pSector + (  ( aCluster % lnIdx ) * 4  )   )    ) -> m_Val;
  if (  ( aCluster & 0xFFFFFFF ) >= 0xFFFFFF8  ) break;
  apBuf[ i++ ] = aCluster & 0xFFFFFFF;
 }  /* end while */

 if ( i >= aBufSize ) return 0;

 return i;

}  /* end _get_cluster_chain32 */

static void inline _get_FAT_type ( USBMDevice* apDev, FATFS* apFS ) {
 unsigned int lnClusters = _data_clusters ( apDev, apFS );
 if ( lnClusters < 4085 ) {
  apFS -> m_FAType          = FAT12;
  apFS -> get_cluster_chain = _get_cluster_chain12;
  apFS -> cluster2sector    = _cluster2sector1216;
 } else if ( lnClusters < 65525 ) {
  apFS -> m_FAType = FAT16;
  apFS -> get_cluster_chain = _get_cluster_chain16;
  apFS -> cluster2sector    = _cluster2sector1216;
 } else {
  apFS -> m_FAType = FAT32;
  apFS -> get_cluster_chain = _get_cluster_chain32;
  apFS -> cluster2sector    = _cluster2sector32;
 }  /* end else */
}  /* end _get_FAT_type */

static int _get_cluster_chain ( USBMDevice* apDev, unsigned int aCluster, unsigned int* apBuf, int aBufSize, int aStart ) {

 FATFS* lpFS = ( FATFS* )apDev -> m_FS.m_pData;

 if ( aCluster == lpFS -> m_LastChainCluster ) return lpFS -> m_LastChainResult;

 lpFS -> m_LastChainResult  = lpFS -> get_cluster_chain ( apDev, aCluster, apBuf, aBufSize, aStart );
 lpFS -> m_LastChainCluster = aCluster;

 return lpFS -> m_LastChainResult;

}  /* end _get_cluster_chain */

static int _get_direntry_sector_data ( USBMDevice* apDev, unsigned int* apStartCluster, unsigned int* apStartSec, int* apDirSec ) {

 FATFS* lpFS = ( FATFS* )apDev -> m_FS.m_pData;
 int    lChainSize;

 if ( apStartCluster[ 0 ] == 0 ) {
  if ( lpFS -> m_FAType != FAT32 ) {
   *apStartSec = lpFS -> m_RootDirStart;
   *apDirSec   = lpFS -> m_RootSize / ( apDev -> m_SectorSize / 32 );
   return 0;
  } else apStartCluster[ 0 ] = lpFS -> m_RootDirCluster;
 }  /* end if */

 apStartSec[ 0 ] = lpFS -> cluster2sector ( apDev, apStartCluster[ 0 ] );
 lChainSize      = _get_cluster_chain ( apDev, apStartCluster[ 0 ], lpFS -> m_ClusterBuf, MAX_DIR_CLUSTER, 1 );

 if ( lChainSize <= 0 ) return -1;

 apDirSec[ 0 ] = lChainSize * lpFS -> m_ClusterSize;

 return lChainSize;

}  /* end _get_direntry_sector_data */

static int _get_direntry ( fat_direntry_short* apShort, fat_direntry_long* apLong, fat_direntry* apDir ) {

 int i, j;
 int lOff;
 int lCont;

 if ( apShort -> m_Name[ 0 ] == 0x00 ) return 0;
 if ( apShort -> m_Name[ 0 ] == 0xE5 ) return 3;

 if ( apLong -> m_RSHV == 0x0F && apLong -> m_Res1 == 0x00 && apLong -> m_Res2[ 0 ] == 0x00 ) {
  lOff  = (  ( apLong -> m_EntrySeq & 0x3F ) - 1  ) * 13;
  lCont = 1;
  for ( i = 0; i < 10 && lCont; i += 2 ) {
   if ( apLong -> m_Name1[ i ] == 0 && apLong -> m_Name1[ i + 1 ] == 0 ) {
    apDir -> m_Name[ lOff ] = 0;
    lCont                   = 0;
   } else apDir -> m_Name[ lOff++ ] = apLong -> m_Name1[ i ];
  }  /* end for */
  for ( i = 0; i < 12 && lCont; i += 2 ) {
   if ( apLong -> m_Name2[ i ]==0 && apLong -> m_Name2[ i + 1 ] == 0 ) {
    apDir -> m_Name[ lOff ] = 0;
    lCont                   = 0;
   } else apDir -> m_Name[ lOff++ ] = apLong -> m_Name2[ i ];
  }  /* end for */
  for ( i = 0; i < 4 && lCont; i += 2 ) {
   if ( apLong -> m_Name3[ i ] == 0 && apLong -> m_Name3[ i + 1 ] == 0 ) {
    apDir -> m_Name[ lOff ] = 0;
    lCont                   = 0;
   } else apDir -> m_Name[ lOff++ ] = apLong -> m_Name3[ i ];
  }  /* end for */
  if ( apLong -> m_EntrySeq & 0x40 ) apDir -> m_Name[ lOff ] = 0;
  return 2;
 } else {
  for ( i = 0; i < 8 && apShort -> m_Name[ i ] != ' '; ++i ) {
   apDir -> m_SName[ i ] = apShort -> m_Name[ i ];
   if ( apShort -> m_ResNT & 0x08 && apDir -> m_SName[ i ] >= 'A' && apDir -> m_SName[ i ] <= 'Z' ) apDir -> m_SName[ i ] += ' ';
  }  /* end for */
  for ( j = 0; j < 3 && apShort -> m_Ext[ j ] != ' '; ++j ) {
   if ( j == 0 ) apDir -> m_SName[ i++ ] = '.';
   apDir -> m_SName[ i + j ] = apShort -> m_Ext[ j ];
   if ( apShort -> m_ResNT & 0x10 && apDir -> m_SName[ i + j ] >= 'A' && apDir -> m_SName[ i + j ] <= 'Z' ) apDir -> m_SName[ i + j ] += ' ';
  }  /* end for */
  apDir -> m_SName[ i + j ] = 0;
  if ( apDir -> m_Name[ 0 ] == 0 ) strcpy ( apDir -> m_Name, apDir -> m_SName );
  apDir -> m_Attr    = apShort -> m_Attr;
  apDir -> m_Size    = apShort -> m_Size;
  apDir -> m_Cluster = (  ( unsigned int )apShort -> m_ClusterHigh << 16 ) | apShort -> m_ClusterLow;
  return 1;
 } /* end else */

}  /* end _get_direntry */

static void _set_dir_chain ( USBMDevice* apDev, fat_dir* apDir ) {

 int          i, j, lIdx, lChainSize, lNextChain, lStartChain;
 FATFS*       lpFS = ( FATFS* )apDev -> m_FS.m_pData;
 unsigned int lFileCluster, lFileSize, lBlockSize;
 unsigned int lClusterSize = lpFS -> m_ClusterSize * apDev -> m_SectorSize;

 lFileCluster = apDir -> m_Chain[ 0 ].m_Cluster;

 if ( lFileCluster < 2 ) return;

 j           = 1;
 lFileSize   = apDir -> m_Size;
 lBlockSize  = lFileSize / DIR_CHAIN_SIZE;
 lNextChain  = 1;
 lStartChain = 0;
 lFileSize   = 0;
 lIdx        = 0;

 while ( lNextChain ) {
  lChainSize = _get_cluster_chain ( apDev, lFileCluster, lpFS -> m_ClusterBuf, MAX_DIR_CLUSTER, 1 );
  if ( lChainSize == 0 ) {
   lChainSize   = MAX_DIR_CLUSTER;
   lFileCluster = lpFS -> m_ClusterBuf[ MAX_DIR_CLUSTER - 1 ];
  } else lNextChain = 0;
  for ( i = lStartChain; i < lChainSize; ++i ) {
   lFileSize += lClusterSize;
   while (  lFileSize >= ( j * lBlockSize ) && j < DIR_CHAIN_SIZE  ) {
    apDir -> m_Chain[ j ].m_Cluster = lpFS -> m_ClusterBuf[ i ];
    apDir -> m_Chain[ j ].m_Index   = lIdx;
    ++j;
   }  /* end while */
   ++lIdx;
  }  /* end for */
  lStartChain = 1;
 }  /* end while */

 apDir -> m_LastCluster = lpFS -> m_ClusterBuf[ i - 1 ];

}  /* end _set_dir_chain */

static void _set_dir ( USBMDevice* apDev, fat_dir* apDir, fat_direntry_short* apShort, fat_direntry* apEntry, int afClusterInfo ) {

 int            i;
 unsigned char* lpSrcName;

 lpSrcName = apEntry -> m_SName;

 if ( apEntry -> m_Name[ 0 ] ) lpSrcName = apEntry -> m_Name;

 strcpy ( apDir -> m_Name, lpSrcName );

 apDir -> m_Attr = apShort -> m_Attr;
 apDir -> m_Size = apEntry -> m_Size;

 apDir -> m_CDate[ 0 ] = apShort -> m_CDate[ 0 ] & 0x1F;
 apDir -> m_CDate[ 1 ] = ( apShort -> m_CDate[ 0 ] >> 5 ) + (  ( apShort -> m_CDate[ 1 ] & 0x01 ) << 3  );
 i = 1980 + ( apShort -> m_CDate[ 1 ] >> 1 );
 apDir -> m_CDate[ 2 ] = i & 0xFF;
 apDir -> m_CDate[ 3 ] = ( i & 0xFF00 ) >> 8;

 apDir -> m_CTime[ 0 ] = (  ( apShort -> m_CTime[ 1 ] & 0xF8 ) >> 3  );
 apDir -> m_CTime[ 1 ] = (  ( apShort -> m_CTime[ 1 ] & 0x07 ) << 3  ) + (  ( apShort -> m_CTime[ 0 ] & 0xE0 ) >> 5  );
 apDir -> m_CTime[ 6 ] = (  ( apShort -> m_CTime[ 0 ] & 0x1F ) << 1  );

 apDir -> m_ADate[ 0 ] = apShort -> m_ADate[ 0 ] & 0x1F;
 apDir -> m_ADate[ 1 ] = ( apShort -> m_ADate[ 0 ] >> 5 ) + (  ( apShort -> m_ADate[ 1 ] & 0x01 ) << 3  );
 i = 1980 + ( apShort -> m_ADate[ 1 ] >> 1 );
 apDir -> m_ADate[ 2 ] = i & 0xFF;
 apDir -> m_ADate[ 3 ] = ( i & 0xFF00 ) >> 8;

 apDir -> m_MDate[ 0 ] = apShort -> m_WDate[ 0 ] & 0x1F;
 apDir -> m_MDate[ 1 ] = ( apShort -> m_WDate[ 0 ] >> 5 ) + (  ( apShort -> m_WDate[ 1 ] & 0x01 ) << 3  );
 i = 1980 + ( apShort -> m_WDate[ 1 ] >> 1 );
 apDir -> m_MDate[ 2 ] = i & 0xFF;
 apDir -> m_MDate[ 3 ] = (i & 0xFF00 ) >> 8;

 apDir -> m_MTime[ 0 ] = (  ( apShort -> m_WTime[ 1 ] & 0xF8 ) >> 3  );
 apDir -> m_MTime[ 1 ] = (  ( apShort -> m_WTime[ 1 ] & 0x07 ) << 3  ) + (  ( apShort -> m_WTime[ 0 ] & 0xE0 ) >> 5  );
 apDir -> m_MTime[ 2 ] = (  ( apShort -> m_WTime[ 0 ] & 0x1F ) << 1  );

 apDir -> m_Chain[ 0 ].m_Cluster = apEntry -> m_Cluster;
 apDir -> m_Chain[ 0 ].m_Index   = 0;

 if ( afClusterInfo ) _set_dir_chain ( apDev, apDir );

}  /* end _set_dir */

static int _get_direntry_start_cluster ( USBMDevice* apDev, unsigned char* apName, unsigned int* apStartCluster, fat_dir* apDir ) {

 fat_direntry_short* lpShort;
 fat_direntry_long*  lpLong;
 fat_direntry        lDir;
 int                 i, lDirSec, lCont, lDirPos;
 unsigned int        lStartSec;
 FATFS*              lpFS         = ( FATFS* )apDev -> m_FS.m_pData;
 unsigned int        lClusterSize = lpFS -> m_ClusterSize;
 unsigned int        lSectorSize  = apDev -> m_SectorSize;

 lCont = 1;
 lDir.m_SName[ 0 ] = 0;
 lDir.m_Name [ 0 ] = 0;

 _get_direntry_sector_data ( apDev, apStartCluster, &lStartSec, &lDirSec );

 for ( i = 0; i < lDirSec && lCont; ++i ) {

  if (  ( apStartCluster[ 0 ] != 0 ) && ( i % lClusterSize == 0 )  )
   lStartSec = lpFS -> cluster2sector (  apDev, lpFS -> m_ClusterBuf[ ( i / lClusterSize ) ]  ) - i;

  apDev -> m_pSector = CacheRead ( apDev, lStartSec + i );

  if ( !apDev -> m_pSector ) return -1;

  lDirPos = 0;

  while ( lCont &&  lDirPos < lSectorSize ) {
   lpShort = ( fat_direntry_short* )( apDev -> m_pSector + lDirPos );
   lpLong  = ( fat_direntry_long*  )( apDev -> m_pSector + lDirPos );
   lCont   = _get_direntry ( lpShort, lpLong, &lDir );
   if ( lCont == 1 ) {
    if (  !( lDir.m_Attr & 0x08 )  ) {
     if ( !strcmpi ( lDir.m_SName, apName ) ||
          !strcmpi ( lDir.m_Name,  apName )
     ) {
      if ( apDir ) _set_dir ( apDev, apDir, lpShort, &lDir, 1 );
      apStartCluster[ 0 ] = lDir.m_Cluster;
      return lDir.m_Attr;
     }  /* end if */
    }  /* end if */
    lDir.m_SName[ 0 ] = 0;
    lDir.m_Name [ 0 ] = 0;
   }  /* end if */
   lDirPos += 32;
  }  /* end while */

 }  /* end  for */

 return -ENOENT;

}  /* end _get_direntry_start_cluster */

static int _file_start_cluster ( USBMDevice* apDev, const char* apName, unsigned int* apStartCluster, fat_dir* apDir ) {

 unsigned char lName[ 257 ];
 int           i, lOff, retVal = -1;

 i     = 0;
 lOff  = 0;

 while ( apName[ i ] == '/' ) ++i;

 while ( apName[ i ] ) {

  if ( apName[ i ] == '/' ) {
   lName[ lOff ] = 0;
   if (   (  retVal = _get_direntry_start_cluster ( apDev, lName, apStartCluster, apDir )  ) < 0   ) return -ENOENT;
   lOff = 0;
   ++i;
   while ( apName[ i ] && apName[ i ] == '/' ) ++i;
  } else lName[ lOff++ ] = apName[ i++ ];
 }  /* end for */

 lName[ lOff ] = 0;

 if (   lName[ 0 ] && (  retVal = _get_direntry_start_cluster ( apDev, lName, apStartCluster, apDir )  ) < 0   ) return -ENOENT;

 return retVal;

}  /* end _file_start_cluster */

static int _next_direntry ( USBMDevice* apDev, FATFS* apFS, fat_dir* apDir ) {

 fat_direntry_short* lpShort;
 fat_direntry_long*  lpLong;
 fat_direntry        lDir;
 unsigned int        i, lDirSector, lStartSector;
 unsigned int        lCont, lNewEntry, lDirPos, lDirCluster;
 unsigned int        lSectorSize, lClusterSize;

 if ( apFS -> m_DirentryCluster == 0xFFFFFFFF || !apDir ) return -ENOENT;

 lSectorSize  = apDev -> m_SectorSize;
 lClusterSize = apFS  -> m_ClusterSize;
 lDirCluster  = apFS  -> m_DirentryCluster;

 lDir.m_SName[ 0 ] = 0;
 lDir.m_Name [ 0 ] = 0;

 _get_direntry_sector_data ( apDev, &lDirCluster, &lStartSector, &lDirSector );

 lCont     = 1;
 lNewEntry = 1;
 lDirPos   = ( apFS -> m_DirentryIndex * 32 ) % lSectorSize;

 for (  i = ( apFS -> m_DirentryIndex * 32 ) / lSectorSize; ( i < lDirSector ) && lCont; ++i ) {
  if (   lDirCluster && (  lNewEntry || ( i % lClusterSize == 0 )  )   ) {
   lStartSector = apFS -> cluster2sector ( apDev, apFS -> m_ClusterBuf[ i / lClusterSize ] ) - i + ( i % lClusterSize );
   lNewEntry    = 0;
  }  /* end if */
  apDev -> m_pSector = CacheRead ( apDev, lStartSector + i );
  if ( !apDev -> m_pSector ) return -ENOENT;
  while ( lCont && lDirPos < lSectorSize ) {
   lpShort = ( fat_direntry_short* )( apDev -> m_pSector + lDirPos );
   lpLong  = ( fat_direntry_long*  )( apDev -> m_pSector + lDirPos );
   lCont = _get_direntry ( lpShort, lpLong, &lDir );
   apFS -> m_DirentryIndex += 1;
   if ( lCont == 1 ) {
    _set_dir ( apDev, apDir, lpShort, &lDir, 0 );
    return 1;
   }  /* end if */
   lDirPos += 32;
  }  /* end while */
  lDirPos = 0;
 }  /* end for */

 apFS -> m_DirentryCluster = 0xFFFFFFFF;

 return -ENOENT;

}  /* end _next_direntry */

static int _first_direntry ( USBMDevice* apDev, const char* apDirName, fat_dir* apDir ) {

 unsigned int lStartCluster = 0;
 FATFS*       lpFS          = ( FATFS* )apDev -> m_FS.m_pData;

 if (  apDirName == NULL || apDirName[ 0 ] == 0 || (
        apDirName[ 0 ] == '.' && (
         apDirName[ 1 ] == 0 || (
          apDirName[ 1 ] == '.' && apDirName[ 2 ] == 0
         )
        )
       ) ||
       ( apDirName[ 0 ] == '/' && (
          apDirName[ 1 ] == 0 || (
           apDirName[ 0 ] == '.' && (
            apDirName[ 1 ] == 0 || (
             apDirName[ 1 ] == '.' && apDirName[ 2 ] == 0
            )
           )
          )
         )
       )
 ) {
  lpFS -> m_DirentryCluster = 0;
 } else {
  int lSts = _file_start_cluster ( apDev, apDirName, &lStartCluster, apDir );
  if ( lSts < 0 ) return -ENOENT;
  if (  !( apDir -> m_Attr & 0x10 )  ) return -ENOTDIR;
  lpFS -> m_DirentryCluster = lStartCluster;
 }  /* end else */

 lpFS -> m_DirentryIndex = 0;

 return _next_direntry ( apDev, lpFS, apDir );

}  /* end _first_direntry */

static unsigned int _cluster_at_pos ( unsigned int aBlockSize, fat_dir* apDir, unsigned int aFilePos, unsigned int* apClusterPos ) {

 int i;
 int j = DIR_CHAIN_SIZE - 1;

 for ( i = 0; i < j; ++i ) if ( apDir -> m_Chain[ i + 0 ].m_Index * aBlockSize <= aFilePos &&
                                apDir -> m_Chain[ i + 1 ].m_Index * aBlockSize >  aFilePos
 ) {
  j = i;
  break;
 }  /* end for */

 apClusterPos[ 0 ] = apDir -> m_Chain[ j ].m_Index * aBlockSize;

 return apDir -> m_Chain[ j ].m_Cluster;

}  /* end _cluster_at_pos */

static int _fat_open ( USBMDevice* apDev, iop_file_t* apFile, const char* apName, int aMode ) {

 int           lSts;
 unsigned int  lCluster;
 file_record*  lpFile;
 FATFS*        lpFS = ( FATFS* )apDev -> m_FS.m_pData;

 if (  !_check_media_change ( apDev, lpFS )  ) return -ENODEV;

 lpFile = &lpFS -> m_File[ 0 ];

 while ( lpFile != &lpFS -> m_File[ _SIZE_( lpFS -> m_File ) ] ) {
  if ( lpFile -> m_FD < 0 ) break;
  ++lpFile;
 }  /* end while */

 if ( lpFile == &lpFS -> m_File[ _SIZE_( lpFS -> m_File ) ] ) return -EMFILE;

 lCluster = 0;
 lSts     = _file_start_cluster ( apDev, apName, &lCluster, &lpFile -> m_Dir );

 if ( lSts < 0    ) return lSts;
 if ( lSts & 0x10 ) return -EISDIR;

 lpFile -> m_FD   = ( int )apDev;
 lpFile -> m_Pos  = 0;
 lpFile -> m_Flag = 1;
 lpFile -> m_Mode = aMode;

 apFile -> privdata = lpFile;

 return lpFile - &lpFS -> m_File[ 0 ];

}  /* end _fat_open */

static int _fat_close ( iop_file_t* apFile ) {

 file_record* lpFile = ( file_record* )apFile -> privdata;

 lpFile -> m_FD = -1;

 return 0;

}  /* end _fat_close */

static int _fat_read ( iop_file_t* apFile, void* apBuf, int aSize ) {

 file_record* lpFile = ( file_record* )apFile -> privdata;
 USBMDevice*  lpDev  = ( USBMDevice*  )lpFile -> m_FD;
 FATFS*       lpFS   = ( FATFS*       )lpDev -> m_FS.m_pData;

 unsigned int i, j, lBufferPos, lFileCluster, lClusterPos;
 unsigned int lStartSector, lSectorSkip, lClusterSkip, lDataSkip;
 unsigned int lBufSize, lNextChain, lClusterChainStart, lChainSize;
 unsigned int lFilePos     = lpFile -> m_Pos;
 unsigned int lSectorSize  = lpDev -> m_SectorSize; 
 unsigned int lClusterSize = lpFS  -> m_ClusterSize;
 unsigned int lBlockSize   = lSectorSize * lClusterSize;

 lFileCluster = _cluster_at_pos ( lBlockSize, &lpFile -> m_Dir, lFilePos, &lClusterPos );
 lSectorSkip  = ( lFilePos - lClusterPos ) / lSectorSize;
 lClusterSkip = lSectorSkip / lClusterSize;
 lSectorSkip  = lSectorSkip % lClusterSize;
 lDataSkip    = lFilePos    % lSectorSize;
 lBufferPos   = 0;

 if ( lFileCluster < 2 ) return 0;

 if ( lFilePos + aSize > lpFile -> m_Dir.m_Size ) aSize = lpFile -> m_Dir.m_Size - lpFile -> m_Pos;

 lBufSize           = lSectorSize;
 lNextChain         = 1;
 lClusterChainStart = 1;

 while ( lNextChain && aSize > 0 ) {
  lChainSize         = _get_cluster_chain ( lpDev, lFileCluster, lpFS -> m_ClusterBuf, MAX_DIR_CLUSTER, lClusterChainStart );
  lClusterChainStart = 0;
  if ( lChainSize == 0 ) {
   lChainSize   = MAX_DIR_CLUSTER;
   lFileCluster = lpFS -> m_ClusterBuf[ MAX_DIR_CLUSTER - 1 ];
  } else lNextChain = 0;
  while ( lClusterSkip >= MAX_DIR_CLUSTER ) {
   lChainSize         = _get_cluster_chain ( lpDev, lFileCluster, lpFS -> m_ClusterBuf, MAX_DIR_CLUSTER, lClusterChainStart );
   lClusterChainStart = 0;
   if ( lChainSize == 0 ) {
    lChainSize   = MAX_DIR_CLUSTER;
    lFileCluster = lpFS -> m_ClusterBuf[ MAX_DIR_CLUSTER - 1 ];
   } else lNextChain = 0;
   lClusterSkip -= MAX_DIR_CLUSTER;
  }  /* end while */
  for ( i = lClusterSkip; i < lChainSize && aSize > 0; ++i ) {
   lStartSector = lpFS -> cluster2sector ( lpDev, lpFS -> m_ClusterBuf[ i ] );
   for ( j = lSectorSkip; j < lClusterSize && aSize > 0; ++j ) {
    lpDev -> m_pSector = CacheRead ( lpDev, lStartSector + j );
    if ( !lpDev -> m_pSector ) goto end;
    if ( aSize < lBufSize ) lBufSize = aSize + lDataSkip;
    if ( lBufSize > lSectorSize ) lBufSize = lSectorSize;
    mips_memcpy ( apBuf, lpDev -> m_pSector + lDataSkip, lBufSize - lDataSkip );
    aSize      -= ( lBufSize - lDataSkip );
    lBufferPos += ( lBufSize - lDataSkip );
    apBuf       = (  ( char* )apBuf  ) + ( lBufSize - lDataSkip );
    lDataSkip   = 0;
    lBufSize    = lSectorSize;
   }  /* end for */
   lSectorSkip = 0;
  }  /* end for */
  lClusterSkip = 0;
 }  /* end while */
end:
 lpFile -> m_Pos += lBufferPos;

 return lBufferPos;

}  /* end _fat_read */

static int _fat_seek ( iop_file_t* apFile, unsigned int aPos, int aDisp ) {

 file_record* lpFile = ( file_record* )apFile -> privdata;

 switch ( aDisp ) {
  case SEEK_SET:
   lpFile -> m_Pos  = aPos;
  break;
  case SEEK_CUR:
   lpFile -> m_Pos += aPos;
  break;
  case SEEK_END:
   lpFile -> m_Pos = lpFile -> m_Dir.m_Size + aPos;
  break;
  default: return -EINVAL;
 }  /* end switch */

 if ( lpFile -> m_Pos >= lpFile -> m_Dir.m_Size ) lpFile -> m_Pos = lpFile -> m_Dir.m_Size;

 return lpFile -> m_Pos;

}  /* end _fat_seek */

static int _fat_dopen ( USBMDevice* apDev, iop_file_t* apFile, const char* apName ) {

 int     lSts;
 fat_dir lDir;
 int     lfRoot = 0;

 if ( apName[ 0 ] == '/' && apName[ 1 ] == 0 ) {
  apName = "/";
  lfRoot = 1;
 }  /* end if */

 if (   (  lSts = _first_direntry ( apDev, apName, &lDir )  ) < 1 && !lfRoot   ) return -ENOENT;

 apFile -> privdata = AllocSysMemory (  0, sizeof ( find_file_info ), NULL  );

 if ( !apFile -> privdata ) return -ENOENT;

 (  ( find_file_info* )apFile -> privdata  ) -> m_FileFlag = 0;
 (  ( find_file_info* )apFile -> privdata  ) -> m_Status   = lSts < 1;
 (  ( find_file_info* )apFile -> privdata  ) -> m_pDev     = apDev;

 mips_memcpy (
  &(  ( find_file_info* )apFile -> privdata  ) -> m_Dir, &lDir, sizeof ( lDir )
 );

 return (  ( FATFS* )apDev -> m_FS.m_pData  ) -> m_Counter++;

}  /* end _fat_dopen */

static int _fat_dclose ( iop_file_t* apFile ) {

 return FreeSysMemory ( apFile -> privdata );

}  /* end _fat_dclose */

static int _fat_dread ( iop_file_t* apFile, void* apBuf ) {

 fio_dirent_t*   lpEntry  = ( fio_dirent_t*   )apBuf;
 find_file_info* lpFFInfo = ( find_file_info* )apFile -> privdata;
 USBMDevice*     lpDev;
 int             lfCont;

 do {

  int  lIn0, lIn1;
  char lCh0, lCh1, lCh2;

  lIn0 = lpFFInfo -> m_Status;
  lCh0 = lpFFInfo -> m_Dir.m_Attr;

  if ( lIn0 ) return 0;

  mips_memset (  lpEntry, 0, sizeof ( *lpEntry )  );

  if ( lCh0 & 8 )
   lfCont = 1;
  else {
   lfCont = 0;
   lIn0   = lpEntry -> stat.mode;
   if ( lpFFInfo -> m_Dir.m_Attr & 0x10 )
    lIn0 |= FIO_SO_IFDIR;
   else lIn0 |= FIO_SO_IFREG;
   lpEntry -> stat.mode = lIn0;
  }  /* end else */

  lpEntry -> stat.size = lpFFInfo -> m_Dir.m_Size;
  strcpy ( lpEntry -> name, lpFFInfo -> m_Dir.m_Name );

  lIn0 = (  ( _u32* )&lpFFInfo -> m_Dir.m_CDate[ 0 ]  ) -> m_Val;
  lCh0 = lpFFInfo -> m_Dir.m_CTime[ 0 ];
  lCh1 = lpFFInfo -> m_Dir.m_CTime[ 1 ];
  lCh2 = lpFFInfo -> m_Dir.m_CTime[ 2 ];

  (  ( _u32* )&lpEntry -> stat.ctime[ 4 ]  ) -> m_Val = lIn0;
  lpEntry -> stat.ctime[ 3 ] = lCh0;
  lpEntry -> stat.ctime[ 2 ] = lCh1;
  lpEntry -> stat.ctime[ 1 ] = lCh2;

  lIn0 = (  ( _u32* )&lpFFInfo -> m_Dir.m_ADate[ 0 ]  ) -> m_Val;
  lIn1 = (  ( _u32* )&lpFFInfo -> m_Dir.m_MDate[ 0 ]  ) -> m_Val;
  lCh0 = lpFFInfo -> m_Dir.m_MTime[ 0 ];
  lCh1 = lpFFInfo -> m_Dir.m_MTime[ 1 ];
  lCh2 = lpFFInfo -> m_Dir.m_MTime[ 2 ];

  lpDev = lpFFInfo -> m_pDev;

  (  ( _u32* )&lpEntry -> stat.atime[ 4 ]  ) -> m_Val = lIn0;
  (  ( _u32* )&lpEntry -> stat.mtime[ 4 ]  ) -> m_Val = lIn1;
  lpEntry -> stat.mtime[ 3 ] = lCh0;
  lpEntry -> stat.mtime[ 2 ] = lCh1;
  lpEntry -> stat.mtime[ 1 ] = lCh2;

  if (  _next_direntry (
         lpDev, ( FATFS* )lpDev -> m_FS.m_pData, &lpFFInfo -> m_Dir
        ) < 1
  ) lpFFInfo -> m_Status = 1;

 } while ( lfCont );

 return 1;

}  /* end _fat_dread */

#ifdef _DEBUG
char s_Buf[ 256 ];
#endif  /* _DEBUG */

int FATFS_Init ( USBMDevice* apDev ) {

 int              i, retVal = 0;
 part_raw_record* lpPartRec = ( part_raw_record* )( apDev -> m_pCacheBuf + 446 );
 int              lSecSize  = apDev -> m_SectorSize;
 FATFS*           lpFS;
#ifdef _DEBUG
 {  /* begin block */
   Log ( "FATFS_Init: begin\r\n" );
 }  /* end block */
#endif  /* _DEBUG */
 if (  UmsRead ( apDev, 0, apDev -> m_pCacheBuf, lSecSize ) != lSecSize  ) return 0;
#ifdef _DEBUG
 {  /* begin block */
   Log ( "FATFS_Init: read sector OK\r\n" );
 }  /* end block */
#endif  /* _DEBUG */
 lpFS = ( FATFS* )AllocSysMemory (  0, sizeof ( FATFS ) + lSecSize, NULL  );

 if ( !lpFS ) return 0;
#ifdef _DEBUG
 {  /* begin block */
   Log ( "FATFS_Init: memory allocation OK\r\n" );
 }  /* end block */
#endif  /* _DEBUG */
 lpFS -> m_PartRec.m_Start = 0;
 lpFS -> m_PartRec.m_Count = apDev -> m_MaxLBA;

 for ( i = 0; i < 4; ++i ) if ( lpPartRec -> m_SID == 6    ||
                                lpPartRec -> m_SID == 4    ||
                                lpPartRec -> m_SID == 1    ||
                                lpPartRec -> m_SID == 0x0B ||
                                lpPartRec -> m_SID == 0x0C ||
                                lpPartRec -> m_SID == 0x0E
                           ) {
  lpFS -> m_PartRec.m_Start = lpPartRec -> m_StartLBA;
  lpFS -> m_PartRec.m_Count = lpPartRec -> m_Size;
  break;
 } else lpPartRec = ( part_raw_record* )(  ( char* )lpPartRec + 16  );
#ifdef _DEBUG
 {  /* begin block */
   sprintf ( s_Buf, "FATFS_Init: partition start - %d, count: - %d\r\n", lpFS -> m_PartRec.m_Start, lpFS -> m_PartRec.m_Count );
   Log ( s_Buf );
 }  /* end block */
#endif  /* _DEBUG */
 if (  UmsRead (
        apDev, lpFS -> m_PartRec.m_Start, apDev -> m_pCacheBuf, lSecSize
       ) == lSecSize
 ) {

  fat_raw_bpb*   lpBPBRaw   = ( fat_raw_bpb*   )apDev -> m_pCacheBuf;
  fat32_raw_bpb* lpBPB32Raw = ( fat32_raw_bpb* )apDev -> m_pCacheBuf;
#ifdef _DEBUG
 {  /* begin block */
   sprintf ( s_Buf, "FATFS_Init: sector size - %d, cluster size - %d\r\n", lpBPBRaw -> m_SectorSize, lpBPBRaw -> m_ClusterSize );
   Log ( s_Buf );
 }  /* end block */
#endif  /* _DEBUG */
  if (  *( unsigned short* )&apDev -> m_pCacheBuf[ 510 ] != 0xAA55  ) return retVal;
  if (  lpBPBRaw -> m_nTotalSectors == 0 &&
        lpBPBRaw -> m_nSectors      == 0
  ) return retVal;

  if (  (  lpBPBRaw -> m_SectorSize ==  512 ||
           lpBPBRaw -> m_SectorSize == 1024 ||
           lpBPBRaw -> m_SectorSize == 2048 ||
           lpBPBRaw -> m_SectorSize == 4096
        ) &&
        (  lpBPBRaw -> m_ClusterSize ==   1 ||
           lpBPBRaw -> m_ClusterSize ==   2 ||
           lpBPBRaw -> m_ClusterSize ==   4 ||
           lpBPBRaw -> m_ClusterSize ==   8 ||
           lpBPBRaw -> m_ClusterSize ==  16 ||
           lpBPBRaw -> m_ClusterSize ==  32 ||
           lpBPBRaw -> m_ClusterSize ==  64 ||
           lpBPBRaw -> m_ClusterSize == 128
        )
  ) {

   char* lpID;

   lpFS -> m_ClusterSize = lpBPBRaw -> m_ClusterSize;
   lpFS -> m_nResSectors = lpBPBRaw -> m_nResSectors;
   lpFS -> m_nFATs       = lpBPBRaw -> m_nFATs;
   lpFS -> m_RootSize    = lpBPBRaw -> m_RootSize;
   lpFS -> m_FATSize     = lpBPBRaw -> m_FATSize;
   lpFS -> m_TrackSize   = lpBPBRaw -> m_TrackSize;
   lpFS -> m_nHeads      = lpBPBRaw -> m_nHeads;
   lpFS -> m_nHidden     = lpBPBRaw -> m_nHidden;
   lpFS -> m_nSectors    = lpBPBRaw -> m_nTotalSectors;

   if ( !lpFS -> m_nSectors ) lpFS -> m_nSectors = lpBPBRaw   -> m_nSectors;
   if ( !lpFS -> m_FATSize  ) lpFS -> m_FATSize  = lpBPB32Raw -> m_FATSize32;

   lpFS -> m_Start            = lpFS -> m_PartRec.m_Start + lpFS -> m_nResSectors;
   lpFS -> m_RootDirStart     = lpFS -> m_Start + lpFS -> m_nFATs * lpFS -> m_FATSize;
   lpFS -> m_RootDirCluster   = 0;
   lpFS -> m_LastChainCluster = 0;

   _get_FAT_type ( apDev, lpFS );
#ifdef _DEBUG
 {  /* begin block */
   sprintf ( s_Buf, "FATFS_Init: FAT type detected - %d\r\n", lpFS -> m_FAType );
   Log ( s_Buf );
 }  /* end block */
#endif  /* _DEBUG */
   if ( lpFS -> m_FAType == FAT32 ) {
    lpFS -> m_ActiveFAT = lpBPB32Raw -> m_FATStatus;
    if ( lpFS -> m_ActiveFAT & 0x80 )
     lpFS -> m_ActiveFAT = lpFS -> m_ActiveFAT & 0xF;
    else lpFS -> m_ActiveFAT = 0;
    lpFS -> m_RootDirCluster = lpBPB32Raw -> m_RootDirCluster;
    lpID = lpBPB32Raw -> m_FATID;
    i    = lpBPB32Raw -> m_Serial;
   } else {
    lpID = lpBPBRaw -> m_FATID;
    i    = lpBPBRaw -> m_Serial;
   }  /* end else */

   lpFS -> m_Serial     = i;
   mips_memcpy ( lpFS -> m_FATID, lpID, 8 );
   lpFS -> m_FATID[ 8 ] = '\x00';

   lpFS -> m_LastChainCluster = 0xFFFFFFFF;
   lpFS -> m_LastChainResult  = 0xFFFFFFFF;
   lpFS -> m_DirentryCluster  = 0xFFFFFFFF;
   lpFS -> m_Counter          = MAX_FILE;

   apDev -> m_FS.open    = _fat_open;
   apDev -> m_FS.close   = _fat_close;
   apDev -> m_FS.read    = _fat_read;
   apDev -> m_FS.seek    = _fat_seek;
   apDev -> m_FS.dopen   = _fat_dopen;
   apDev -> m_FS.dclose  = _fat_dclose;
   apDev -> m_FS.dread   = _fat_dread;
   apDev -> m_FS.Destroy = FreeSysMemory;
   apDev -> m_FS.m_pData = lpFS;

   for (  i = 0; i < _SIZE_( lpFS -> m_File ); ++i  ) lpFS -> m_File[ i ].m_FD = -1;

   retVal = 1;

  }  /* end if */

 }  /* end if */

 if ( !retVal ) FreeSysMemory ( lpFS );
#ifdef _DEBUG
 {  /* begin block */
   sprintf ( s_Buf, "FATFS_Init: return - %d\r\n", retVal );
   Log ( s_Buf );
 }  /* end block */
#endif  /* _DEBUG */
 return retVal;

}  /* end FATFS_Init */
