/*
 * scache.c - USB Mass storage driver for PS2
 * (C) 2004, Marek Olejnik (ole00@post.cz)
 * (C) 2004  Hermes (support for sector sizes from 512 to 4096 bytes)
 * (C) 2006  Eugene Plotnikov (SMS version)
 * Sector cache 
 * See the file LICENSE included with this distribution for licensing terms.
 */
#include "mass_stor.h"

#define READ_SECTOR( a, b ) mass_stor_readSector (  ( a ), ( b )  ) 
#define CACHE_SIZE          32

typedef struct _cache_record {

 unsigned int m_Sector;
 int          m_Tax;

} cache_record;

int sectorSize;

static int            s_IndexLimit;
static unsigned char* s_pSectorBuf;
static cache_record   s_Rec[ CACHE_SIZE ];

static void initRecords ( void ) {

 int i;

 for ( i = 0; i < CACHE_SIZE; ++i ) {

  s_Rec[ i ].m_Sector = 0xFFFFFFF0;
  s_Rec[ i ].m_Tax    = 0;

 }  /* end for */

}  /* end initRecords */

static int getSlot ( unsigned int aSector ) {

 int i;

 for ( i = 0; i < CACHE_SIZE; ++i )

  if (  aSector >= s_Rec[ i ].m_Sector && aSector < ( s_Rec[ i ].m_Sector + s_IndexLimit )  ) return i;

 return -1;

}  /* end getSlot */

static int getIndexRead ( unsigned int aSector ) {

 int i;
 int lIndex = -1;

 for ( i = 0; i < CACHE_SIZE; ++i ) {

  if (  aSector >= s_Rec[ i ].m_Sector && aSector < ( s_Rec[ i ].m_Sector + s_IndexLimit )  ) {

   if (  s_Rec[ i ].m_Tax < 0 ) s_Rec[ i ].m_Tax = 0;

   s_Rec[ i ].m_Tax += 2;
   lIndex            = i;

  }  /* end if */

  --s_Rec[ i ].m_Tax;

 }  /* end for */

 if ( lIndex < 0 ) 

  return lIndex;

 else return lIndex * s_IndexLimit + ( aSector - s_Rec[ lIndex ].m_Sector );

}  /* end getIndexRead */

static int getIndexWrite ( unsigned int aSector ) {

 int i, retVal;
 int lMinTax = 0x0FFFFFFF;
 int lIndex  = 0;

 for ( i = 0; i < CACHE_SIZE; ++i ) {

  if ( s_Rec[ i ].m_Tax < lMinTax ) {

   lIndex  = i;
   lMinTax = s_Rec[ i ].m_Tax;

  }  /* end if */

 }  /* end for */
	
 s_Rec[ lIndex ].m_Tax   += 2; 
 s_Rec[ lIndex ].m_Sector = aSector;

 return lIndex * s_IndexLimit;

}  /* end getIndexWrite */

int scache_readSector ( unsigned int aSec, void** appBuf ) {

 int          lIndex;
 int          retVal;
 unsigned int lAlignedSec;

 lIndex = getIndexRead ( aSec );

 if ( lIndex >= 0 ) {

  *appBuf = s_pSectorBuf + ( lIndex * sectorSize );

  return sectorSize;

 }  /* end if */

 lAlignedSec = ( aSec / s_IndexLimit ) * s_IndexLimit;
 lIndex      = getIndexWrite ( lAlignedSec );

 retVal = READ_SECTOR(  lAlignedSec, s_pSectorBuf + ( lIndex * sectorSize )  );

 if ( retVal < 0 ) return retVal;

 *appBuf = s_pSectorBuf + ( lIndex * sectorSize ) + (  ( aSec % s_IndexLimit ) * sectorSize  );

 return sectorSize;

}  /* end scache_readSector */

int scache_init ( int aSecSize ) {

 sectorSize   = aSecSize;
 s_IndexLimit = 4096 / aSecSize;

 if ( !s_pSectorBuf ) s_pSectorBuf = ( unsigned char* )AllocSysMemory ( 0, 4096 * CACHE_SIZE, 0 );

 initRecords ();

 return 1;

}  /* end scache_init */

void scache_close ( void ) {

 if ( s_pSectorBuf ) {

  FreeSysMemory ( s_pSectorBuf );
  s_pSectorBuf = 0;

 }  /* end if */

}  /* end scache_close */
