#include "ums_cache.h"
#include "ums_driver.h"

static int inline _read_index ( USBMDevice* apDev, unsigned int aSector ) {
 int i, lIdx = -1;
 int lIdxLimit = apDev -> m_IdxLimit;
 for ( i = 0; i < 4; ++i ) {
  if ( aSector >= apDev -> m_Cache[ i ].m_Sector &&
       aSector < ( apDev -> m_Cache[ i ].m_Sector + lIdxLimit )
  ) {
   if ( apDev -> m_Cache[ i ].m_Tax < 0 ) apDev -> m_Cache[ i ].m_Tax = 0;
   apDev -> m_Cache[ i ].m_Tax += 2;
   lIdx = i;
  }  /* end if */
  apDev -> m_Cache[ i ].m_Tax -= 1;
 }  /* end for */
 if ( lIdx < 0 ) return lIdx;
 return ( lIdx * lIdxLimit ) + ( aSector - apDev -> m_Cache[ lIdx ].m_Sector );
}  /* end _read_index */

static int inline _write_index ( USBMDevice* apDev, unsigned int aSector, int anIdxLimit ) {
 int i;
 int lMinTax = 0x0FFFFFFF;
 int lIdx    = 0;
 for ( i = 0; i < 4; ++i ) {
  if ( apDev -> m_Cache[ i ].m_Tax < lMinTax ) {
   lIdx    = i;
   lMinTax = apDev -> m_Cache[ i ].m_Tax;
  }  /* end if */
 }  /* end for */
 apDev -> m_Cache[ lIdx ].m_Tax   += 2;
 apDev -> m_Cache[ lIdx ].m_Sector = aSector;
 return lIdx * anIdxLimit;
}  /* end _write_index */

void CacheInit ( USBMDevice* apDev ) {

 int i;

 apDev -> m_IdxLimit = ( 32768 + apDev -> m_SectorSize - 1 ) / apDev -> m_SectorSize;

 for ( i = 0; i < 4; ++i ) {
  apDev -> m_Cache[ i ].m_Sector = 0xFFFFFFF0;
  apDev -> m_Cache[ i ].m_Tax    = 0;
 }  /* end for */

}  /* end CacheInit */

void* CacheRead ( USBMDevice* apDev, unsigned int aSector ) {

 int          lIdx;
 unsigned int lAlignedSector;
 unsigned int lSectorSize = apDev -> m_SectorSize;
 unsigned int lIdxLimit   = apDev -> m_IdxLimit;
 unsigned int lQuot, lRem;
 char*        lpPtr;

 lIdx = _read_index ( apDev, aSector );

 if ( lIdx >= 0 ) return apDev -> m_pCacheBuf + ( lIdx * lSectorSize );

 lQuot = aSector / lIdxLimit;
 lRem  = aSector % lIdxLimit;

 lAlignedSector = lQuot * lIdxLimit;
 lIdx           = _write_index ( apDev, lAlignedSector, lIdxLimit );
 lpPtr          = apDev -> m_pCacheBuf + lIdx * lSectorSize;

 if (  UmsRead (
        apDev, lAlignedSector, lpPtr, 32768
       ) >= lSectorSize
 ) return lpPtr + lRem * lSectorSize;

 return NULL;

}  /* end CacheRead */
