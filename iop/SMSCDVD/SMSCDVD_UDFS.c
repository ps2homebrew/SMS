/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Based on:
#  dvdudf: parse and read the UDF volume information of a DVD Video
#  Copyright (C) 1999 Christian Wolff for convergence integrated media GmbH
#
# Adopted for SMS in 2005 by Eugene Plotnikov <e-plotnikov@operamail.com>
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
# Or, point your browser to http://www.gnu.org/copyleft/gpl.html
# 
# The author can be reached at scarabaeus@convergence.de, 
# the project's page is at http://linuxtv.org/dvd/
*/
#define UDFADshort 1
#define UDFADlong  2
#define UDFADext   4

#define UDF_FILE_TYPE_DIRECTORY 4
#define UDF_FILE_TYPE_FILE      5

#define UDF_GETN1( p ) (  ( unsigned char  )apData[ p ]  )
#define UDF_GETN2( p ) (   ( unsigned short )apData[ p ] | (  ( unsigned short )apData[ ( p ) + 1 ] << 8  )   )
#define UDF_GETN4( p ) (   ( unsigned int   )apData[ p ] | (  ( unsigned int   )apData[ ( p ) + 1 ] << 8  ) | (  ( unsigned int )apData[ ( p ) + 2 ] << 16  ) | (  ( unsigned int )apData[ ( p ) + 3 ] << 24  )   )
#define UDF_GETN( p, n, target ) mips_memcpy ( target, &apData[ p ], n )

typedef struct UDFPartitionInfo {

 int            m_fValid;
 unsigned char  m_VolDesc[ 128 ];
 unsigned short m_Flags;
 unsigned short m_Number;
 unsigned char  m_Contents[ 32 ];
 unsigned int   m_AccessType;
 unsigned int   m_Start;
 unsigned int   m_Length;

} UDFPartitionInfo;

typedef struct UDFAddress {

 unsigned int   m_Location;
 unsigned int   m_Length;
 unsigned short m_Partition;
 unsigned char  m_Flags;
 unsigned char  m_Type;

} UDFAddress;

static UDFPartitionInfo s_PartInfo;
static UDFAddress       s_RootICB;
static UDFAddress       s_RootDir;
static unsigned char    s_ScanBuf [ 4096 ];
static char             s_ScanTemp[ 2048 ];
static unsigned char    s_ScanFileChar;
static unsigned short   s_ScanTagID;
static unsigned int     s_ScanLBA;
static unsigned int     s_ScanLen;
static unsigned int     s_ScanPos;
static UDFAddress       s_ScanDir;

static void UDFICB ( unsigned char* apData, unsigned char* apFileType, unsigned short* apFlags ) {

 apFileType[ 0 ] = UDF_GETN1( 11 );
 apFlags   [ 0 ] = UDF_GETN2( 18 );

}  /* end UDFICB */

static void UDFDecode ( unsigned char* apSrc, int aLen, char* apDst ) {

 int i = 0, p = 1;

 if (  !( apSrc[ 0 ] & 0x18 )  ) {

  apDst[ 0 ] = '\x00';
  return;

 }  /* end if */

 if ( apSrc[ 0 ] & 0x10 ) {

  ++p;

  while ( p < aLen ) apDst[ i++ ] = apSrc[ p += 2 ];

 } else while ( p < aLen ) apDst[ i++ ] = apSrc[ p++ ];

 apDst[ i ] = '\x00';

}  /* end UDFDecode */

static void inline UDFDescriptor ( unsigned char* apData, unsigned short* apTagID ) {

 apTagID[ 0 ] = UDF_GETN2( 0 );

}  /* end UDFDescriptor */

static void UDFExtentAD ( unsigned char* apData, unsigned int* apLen, unsigned int* apLoc ) {

 apLen[ 0 ] = UDF_GETN4( 0 );
 apLoc[ 0 ] = UDF_GETN4( 4 );

}  /* end UDFExtentAD */

static void UDFPartition ( unsigned char* apData, unsigned short* apFlags, unsigned short * apNb, char* apContents, unsigned int* apStart, unsigned int* apLen ) {

 apFlags[ 0 ] = UDF_GETN2( 20 );
 apNb   [ 0 ] = UDF_GETN2( 22 );

 UDF_GETN( 24, 32, apContents );

 apStart[ 0 ] = UDF_GETN4( 188 );
 apLen  [ 0 ] = UDF_GETN4( 192 );

}  /* end UDFPartition */

static int UDFLogVolume ( unsigned char* apData, char* apVolDesc ) {

 unsigned int lLBSize;
 unsigned int lMTL;
 unsigned int lnPM;

 UDFDecode ( &apData[ 84 ], 128, apVolDesc );

 lLBSize = UDF_GETN4( 212 );
 lMTL    = UDF_GETN4( 264 );
 lnPM    = UDF_GETN4( 268 );

 return lLBSize != 2048 ? 1 : 0;

}  /* end UDFLogVolume */

static int UDFFindPartition ( int aPartNr ) {

 unsigned char* lpLB     = &s_Buffer[        12 ];
 unsigned char* lpAnchor = &s_Buffer[ 2064 + 12 ];
 unsigned int   lLBA;
 unsigned int   lMVDSLoc;
 unsigned int   lMVDSLen;
 unsigned int   lLastSec;
 int            lfTerm;
 int            lfVolValid;
 int            i;
 unsigned short lTagID;

 lLastSec =   0;
 lLBA     = 256;
 lfTerm   =   0;

 while ( 1 ) {

  if (  ReadDVDVSectors ( lLBA, 1, &s_Buffer[ 2064 ] )  )

   UDFDescriptor ( lpAnchor, &lTagID );

  else lTagID = 0;

  if ( lTagID != 2 ) {

   if ( lfTerm ) return 0;

   if ( lLastSec ) {

    lLBA   = lLastSec;    
    lfTerm = 1;            

   } else {

    if ( lLastSec )

     lLBA = lLastSec - 256;                

    else return 0;

   }  /* end else */

  } else break;

 }  /* end while */

 UDFExtentAD ( lpAnchor + 16, &lMVDSLen, &lMVDSLoc );
  
 s_PartInfo.m_fValid       = 0;
 s_PartInfo.m_VolDesc[ 0 ] = '\x00';
 lfVolValid                = 0;

 i = 1;

 do {

  lLBA = lMVDSLoc;

  do {

   if (  !ReadDVDVSectors ( lLBA++, 1, s_Buffer )  )

    lTagID = 0;

   else UDFDescriptor ( lpLB, &lTagID );

   if (  lTagID == 5 && !s_PartInfo.m_fValid  ) {

    UDFPartition (
     lpLB, &s_PartInfo.m_Flags,
           &s_PartInfo.m_Number,
            s_PartInfo.m_Contents,
           &s_PartInfo.m_Start,
           &s_PartInfo.m_Length
    );

    s_PartInfo.m_fValid = ( aPartNr == s_PartInfo.m_Number );

   } else if ( lTagID == 6 && !lfVolValid ) {

    if (  UDFLogVolume ( lpLB, s_PartInfo.m_VolDesc )  ) {
/* wrong sector size */
    } else lfVolValid = 1;

   }  /* end if */

  } while (   (  lLBA <= lMVDSLoc + ( lMVDSLen - 1 ) / 2048  ) &&
              (  lTagID != 8                                 ) &&
              (  !s_PartInfo.m_fValid || !lfVolValid         )
    );
    
  if ( !s_PartInfo.m_fValid || !lfVolValid  )

   UDFExtentAD ( lpAnchor + 24, &lMVDSLen, &lMVDSLoc );

 } while (  i-- && ( !s_PartInfo.m_fValid || !lfVolValid )  );

 return s_PartInfo.m_fValid;

}  /* end UDFFindPartition */

static void UDFAD ( unsigned char* apData, UDFAddress* apAddr, unsigned char aType ) {

 apAddr -> m_Length  = UDF_GETN4( 0 );
 apAddr -> m_Flags   = apAddr -> m_Length >> 30;
 apAddr -> m_Length &= 0x3FFFFFFF;

 switch ( aType ) {

  case UDFADshort:

   apAddr -> m_Location  = UDF_GETN4( 4 );
   apAddr -> m_Partition = s_PartInfo.m_Number;

  break;

  case UDFADlong:

   apAddr -> m_Location  = UDF_GETN4( 4 );
   apAddr -> m_Partition = UDF_GETN2( 8 );

  break;

  case UDFADext:

   apAddr -> m_Location  = UDF_GETN4( 12 );
   apAddr -> m_Partition = UDF_GETN2( 16 );

  break;

 }  /* end switch */

}  /* end UDFAD */

static void UDFFileEntry ( unsigned char* apData, UDFAddress* apAddr ) {

 unsigned int   lLEA;
 unsigned int   lLAD;
 unsigned int   p;
 unsigned short lFlags;

 UDFICB ( &apData[ 16 ], &apAddr -> m_Type, &lFlags );

 lLEA = UDF_GETN4( 168 );
 lLAD = UDF_GETN4( 172 );
 p    = 176 + lLEA;

 while ( p < 176 + lLEA + lLAD ) {

  switch ( lFlags & 0x07 ) {

   case 0:

    UDFAD ( &apData[ p ], apAddr, UDFADshort );

   break;

   case 1:

    UDFAD ( &apData[ p ], apAddr, UDFADlong );

   break;

   case 2:

    UDFAD ( &apData[ p ], apAddr, UDFADext );

   break;

   case 3:

    switch ( lLAD ) {

     case 0x08:

      UDFAD ( &apData[ p ], apAddr, UDFADshort );

     break;

     case 0x10:

      UDFAD ( &apData[ p ], apAddr, UDFADlong );

     break;

     case 0x14:

      UDFAD ( &apData[ p ], apAddr, UDFADext );

     break;

    }  /* end switch */

  }  /* end switch */

  p += lLAD;

 }  /* end while */

 apAddr -> m_Length = UDF_GETN4( 56 );

}  /* end UDFFileEntry */

static int UDFMapICB ( UDFAddress* apICB, UDFAddress* apFile ) {

 unsigned char* lpLB = &s_Buffer[ 12 ];
 unsigned int   lLBA;
 unsigned short lTagID;

 lLBA = s_PartInfo.m_Start + apICB -> m_Location;

 do {

  if (  !ReadDVDVSectors ( lLBA++, 1, s_Buffer )  )

   lTagID = 0;

  else UDFDescriptor( lpLB, &lTagID );

  if ( lTagID == 261 ) {

   UDFFileEntry ( lpLB, apFile );
   return 1;

  }  /* end if */

 } while (   (  lLBA <= s_PartInfo.m_Start + apICB -> m_Location + ( apICB -> m_Length - 1 ) / 2048  ) && lTagID != 261   );

 return 0;

}  /* end UDFMapICB */

static int UDFFileIdentifier (
            unsigned char* apData, unsigned char* apFileChar,
            char* apFileName, UDFAddress* apFileICB
           ) {

 unsigned short lLIU;
 unsigned char  lLFI;
  
 apFileChar[ 0 ] = UDF_GETN1( 18 );
 lLFI            = UDF_GETN1( 19 );

 UDFAD ( &apData[ 20 ], apFileICB, UDFADlong );

 lLIU = UDF_GETN2( 36 );

 if ( lLFI )

  UDFDecode ( &apData[ 38 + lLIU ], lLFI, apFileName );

 else apFileName[ 0 ] = '\x00';

 return 4 * (  ( 38 + lLFI + lLIU + 3 ) / 4  );

}  /* end UDFFileIdentifier */

static int UDFScanDir ( UDFAddress* apScanDir, char* apFileName, UDFAddress* apFileICB ) {

 unsigned int   lScanPos;
 unsigned int   lScanLBA = s_PartInfo.m_Start + apScanDir -> m_Location;
 unsigned int   lScanLen = apScanDir -> m_Length;
 unsigned short lScanTagID;
 unsigned char  lScanFileChar;

 if (  ReadDVDVSectors ( lScanLBA, 2, s_Buffer ) <= 0  ) return 0;

 mips_memcpy ( &s_ScanBuf[    0 ], &s_Buffer[   12 ], 2048 );
 mips_memcpy ( &s_ScanBuf[ 2048 ], &s_Buffer[ 2076 ], 2048 );

 lScanPos = 0;

 while ( lScanPos < lScanLen ) {

  if ( lScanPos >= 2048 ) {

   ++lScanLBA;
   lScanPos -= 2048;
   lScanLen -= 2048;

   if (  ReadDVDVSectors ( lScanLBA, 2, s_Buffer ) <= 0  ) return 0;

   mips_memcpy ( &s_ScanBuf[    0 ], &s_Buffer[   12 ], 2048 );
   mips_memcpy ( &s_ScanBuf[ 2048 ], &s_Buffer[ 2076 ], 2048 );

  }  /* end if */

  UDFDescriptor ( &s_ScanBuf[ lScanPos ], &lScanTagID );

  if ( lScanTagID == 257 ) {

   lScanPos += UDFFileIdentifier (
    &s_ScanBuf[ lScanPos ], &lScanFileChar, s_ScanTemp, apFileICB
   );

   if (  !strcasecmp ( apFileName, s_ScanTemp )  ) return 1;

  } else return 0;

 }  /* end while */

 return 0;

}  /* end UDFScanDir */

static int UDFInit ( void ) {

 int            retVal = 0;
 unsigned char* lpLB   = &s_Buffer[ 12 ];
 unsigned short lTagID;

 if (  UDFFindPartition ( 0 )  ) {

  unsigned int lLBA = s_PartInfo.m_Start;

  do {

   if (  !ReadDVDVSectors ( lLBA++, 1, s_Buffer )  )

    lTagID = 0;

   else UDFDescriptor ( lpLB, &lTagID );

   if ( lTagID == 256 ) UDFAD ( lpLB + 400, &s_RootICB, UDFADlong );

  } while (  ( lLBA < s_PartInfo.m_Start + s_PartInfo.m_Length ) &&
             ( lTagID !=   8                                   ) &&
             ( lTagID != 256                                   )
    );

  if ( lTagID                == 256 &&  /* good descriptor */
       s_RootICB.m_Partition ==   0 &&  /* good partition  */
       UDFMapICB (                      /* root dir found  */
        &s_RootICB, &s_RootDir
       ) && s_RootDir.m_Type == 4
  ) retVal = 1;

 }  /* end if */

 s_ScanPos = 0xFFFFFFFF;

 return retVal;

}  /* end UDFInit */

static int UDF_FindFile ( const char* apPath, UDFAddress* apFile ) {

 UDFAddress lICB;
 char*      lpPos;
 int        lLen;

 *apFile = s_RootDir;
 apFile -> m_Type  = UDF_FILE_TYPE_DIRECTORY;

 strcpy ( s_DirName, apPath );
 lLen = strlen ( s_DirName );

 if ( s_DirName[ lLen - 1 ] == '/' ||
      s_DirName[ lLen - 1 ] == '\\'
 ) s_DirName[ lLen - 1 ] = '\x00';

 lpPos = strtok ( s_DirName, "\\/" );

 if ( lpPos ) {

  while ( lpPos ) {

   if (  !UDFScanDir ( apFile, lpPos, &lICB  )  ) return 0;
   if (  !UDFMapICB  ( &lICB, apFile         )  ) return 0;

   lpPos = strtok ( NULL, "\\/" );

  }  /* end while */

 }  /* end if */

 return 1;

}  /* end UDF_FindFile */

static int UDF_DOpen ( iop_io_file_t* apFile, const char* apPath ) {

 UDFAddress lDir;
 
 if ( s_ScanPos != 0xFFFFFFFF ) return -ENFILE;

 lDir = s_RootDir;

 if (  !UDF_FindFile ( apPath, &lDir ) ||
       lDir.m_Type != UDF_FILE_TYPE_DIRECTORY
 ) return -ENOENT;

 s_ScanDir = lDir;
 s_ScanPos = 0;
 s_ScanLBA = s_PartInfo.m_Start + s_ScanDir.m_Location;
 s_ScanLen = s_ScanDir.m_Length;

 ReadDVDVSectors ( s_ScanLBA++, 2, s_Buffer );

 mips_memcpy ( &s_ScanBuf[    0 ], &s_Buffer[   12 ], 2048 );
 mips_memcpy ( &s_ScanBuf[ 2048 ], &s_Buffer[ 2076 ], 2048 );

 return 1;

}  /* end UDF_DOpen */

static int UDF_DRead ( iop_io_file_t* apFile, void* apRetVal ) {

 fio_dirent_t* lpBuf = ( fio_dirent_t* )apRetVal;
 UDFAddress    lFileICB;

 if ( s_ScanPos == 0xFFFFFFFF ) return -EPROTO;

 while ( s_ScanPos < s_ScanLen ) {

  if ( s_ScanPos >= 2048 ) {

   s_ScanPos -= 2048;
   s_ScanLen -= 2048;

   if (  ReadDVDVSectors ( s_ScanLBA++, 2, s_Buffer ) <= 0  ) return 0;

   mips_memcpy ( &s_ScanBuf[    0 ], &s_Buffer[   12 ], 2048 );
   mips_memcpy ( &s_ScanBuf[ 2048 ], &s_Buffer[ 2076 ], 2048 );

  }  /* end if */

  UDFDescriptor ( &s_ScanBuf[ s_ScanPos ], &s_ScanTagID );

  if ( s_ScanTagID == 257 ) {

   s_ScanPos += UDFFileIdentifier (
    &s_ScanBuf[ s_ScanPos ], &s_ScanFileChar, s_ScanTemp, &lFileICB
   );

   if ( s_ScanTemp[ 0 ] ) {

    UDFAddress lFile;

    if (  !UDFMapICB ( &lFileICB, &lFile )  ) return -ENOENT;

    strcpy ( lpBuf -> name, s_ScanTemp );

    if ( lFile.m_Type == UDF_FILE_TYPE_DIRECTORY )

     lpBuf -> stat.mode = FIO_SO_IFDIR;

    else if ( lFile.m_Type == UDF_FILE_TYPE_FILE )

     lpBuf -> stat.mode = FIO_SO_IFREG;

    else return -ENOENT;

    return 1;

   }  /* end if */

  } else return -ENOENT;

 }  /* end while */

 return -ENOENT;

}  /* end UDF_DRead */

static int UDF_DClose ( iop_io_file_t* apFile ) {

 s_ScanPos = 0xFFFFFFFF;

 return 1L;

}  /* end UDF_DClose */

static int UDF_Open ( iop_io_file_t* apFile, const char* apName ) {

 UDFAddress lFile;

 int i;

 if (  !UDF_FindFile ( apName, &lFile ) || lFile.m_Type != UDF_FILE_TYPE_FILE  ) return -ENOENT;

 lFile.m_Location += s_PartInfo.m_Start;

 i = _AllocFD ( lFile.m_Location, lFile.m_Length );

 apFile -> privdata = ( void* )i;

 return i;

}  /* end UDF_Open */

static int UDF_Read ( iop_io_file_t* apFile, void* apBuff, int aSize ) {

 int          i;
 unsigned int lStartSector;
 unsigned int lOffSector;
 unsigned int lnSectors;
 unsigned int lfRead  = 0;
 unsigned int lnExtra = 0;
 char*        lpBuff;

 i = _LookupFD (  ( int )apFile -> privdata  );

 if ( i < 0 ) return i;

 if ( s_FDTable[ i ].m_FilePos > s_FDTable[ i ].m_FileSize ) return 0;

 if (  ( s_FDTable[ i ].m_FilePos + aSize ) > s_FDTable[ i ].m_FileSize )

  aSize = s_FDTable[ i ].m_FileSize - s_FDTable[ i ].m_FilePos;

 if ( aSize <= 0 ) return 0;

 if ( aSize > 16384 ) aSize = 16384; 

 lStartSector = s_FDTable[ i ].m_LBA + ( s_FDTable[ i ].m_FilePos >> 11 );
 lOffSector   = s_FDTable[ i ].m_FilePos & 0x7FF;

 lnSectors = lOffSector + aSize;
 lnSectors = ( lnSectors >> 11 ) + (  ( lnSectors & 2047 ) != 0  );

 if ( lStartSector == s_LastSector ) {

  lfRead  =    1;
  lnExtra = 2064;

  if ( s_LastBk > 0 ) mips_memcpy ( s_Buffer, s_Buffer + 2064 * s_LastBk, 2064 );

  s_LastBk = 0;

 }  /* end if */
	
 s_LastSector = lStartSector + lnSectors - 1;

 if (  lfRead == 0 || ( lfRead == 1 && lnSectors > 1 )  ) {

  ReadDVDVSectors (
   lStartSector + lfRead, lnSectors - lfRead, s_Buffer + lnExtra
  );
	 
  s_LastBk = lnSectors - 1;

 }  /* end if */

 lOffSector               +=    12;
 s_FDTable[ i ].m_FilePos += aSize;

 i         = aSize;
 lnSectors = 2060 - lOffSector;

 if (  ( int )lnSectors > aSize  ) lnSectors = aSize;

 mips_memcpy ( apBuff, s_Buffer + lOffSector, lnSectors );

 i -= lnSectors;

 if ( i > 0 ) {

  lpBuff = s_Buffer + 2064;
  apBuff = ( char* )apBuff + lnSectors;

  lnSectors  = i >> 11;
  lOffSector = i & 0x7FF;

  for (  i = 0; i < ( int )lnSectors; ++i  ) {

   mips_memcpy ( apBuff, lpBuff + 12, 2048 );
   apBuff  = ( char* )apBuff + 2048;
   lpBuff += 2064;

  }  /* end for */

  if ( lOffSector ) mips_memcpy ( apBuff, lpBuff + 12, lOffSector );

 }  /* end if */

 return aSize;

}  /* end UDF_Read */
