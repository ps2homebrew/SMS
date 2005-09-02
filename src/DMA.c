/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright 2004 - Chris "Neovanglist" Gilbert <Neovanglist@LainOS.org>
#           2005 - Adopted for SMS by Eugene Plotnikov
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "DMA.h"

#include <kernel.h>

const unsigned int DMA_CHCR[ 10 ] = {
 0x10008000, 0x10009000, 0x1000A000, 0x1000B000,
 0x1000B400, 0x1000C000, 0x1000C400, 0x1000C800,
 0x1000D000, 0x1000D400
};

const unsigned int DMA_MADR[ 10 ] = {
 0x10008010, 0x10009010, 0x1000A010, 0x1000B010,
 0x1000B410, 0x1000C010, 0x1000C410, 0x1000C810,
 0x1000D010, 0x1000D410
};

const unsigned int DMA_SIZE[ 10 ] = {
 0x10008020, 0x10009020, 0x00000000, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000,
 0x00000000, 0x00000000
};

const unsigned int DMA_TADR[ 10 ] = {
 0x10008030, 0x10009030, 0x1000A030, 0x00000000,
 0x1000B430, 0x00000000, 0x1000C430, 0x00000000,
 0x00000000, 0x1000D430
};

const unsigned int DMA_ASR0[ 10 ] = {
 0x10008040, 0x10009040, 0x1000A040, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000,
 0x00000000, 0x00000000
};

const unsigned int DMA_ASR1[ 10 ] = {
 0x10008050, 0x10009050, 0x1000A050, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000,
 0x00000000, 0x00000000
};

const unsigned int DMA_SADR[ 10 ] = {
 0x10008080, 0x10009080, 0x1000A080, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000,
 0x1000D080, 0x1000D480
};

const unsigned int DMA_QWC[ 10 ] =  {
 0x10008020, 0x10009020, 0x1000A020, 0x1000B020,
 0x1000B420, 0x1000C020, 0x1000C420, 0x1000C820,
 0x1000D020, 0x1000D420
};

void DMA_Wait ( int aChannel ) {

 while (   *DMA_ADDR(  DMA_CHCR[ aChannel ]  ) & 0x00000100   );

}  /* end DMA_Wait */

void DMA_Send ( int aChannel, unsigned long int* apData, int aSize ) {

 SyncDCache (  apData, apData + ( aSize << 2 )  );

 *DMA_ADDR( DMA_QWC [ aChannel ]  ) = aSize;
 *DMA_ADDR( DMA_MADR[ aChannel ]  ) = ( unsigned int )apData;
 *DMA_ADDR( DMA_CHCR[ aChannel ]  ) = DMA_SET_CHCR(
   DMADirection_From_Mem, DMATransferMode_Normal, 0, 0, 0, 1, 0
  );

}  /* end DMA_Send */

void DMA_Recv ( int aChannel, unsigned long int* apData, int aSize ) {

 *DMA_ADDR( DMA_QWC [ aChannel ]  ) = aSize;
 *DMA_ADDR( DMA_MADR[ aChannel ]  ) = ( unsigned int )apData;
 *DMA_ADDR( DMA_CHCR[ aChannel ]  ) = DMA_SET_CHCR(
   DMADirection_To_Mem, DMATransferMode_Normal, 0, 0, 0, 1, 0
  );

}  /* end DMA_Recv */

void DMA_SendChain ( int aChannel, unsigned long int* apData, int aSize ) {

 SyncDCache (  apData, apData + ( aSize << 2 )  );

 *DMA_ADDR( DMA_QWC [ aChannel ]  ) = 0;
 *DMA_ADDR( DMA_TADR[ aChannel ]  ) = ( unsigned int )apData;
 *DMA_ADDR( DMA_CHCR[ aChannel ]  ) = DMA_SET_CHCR(
   DMADirection_From_Mem, DMATransferMode_Chain, 0, 0, 0, 1, 0
  );

}  /* end DMA_SendChain */

void DMA_Init ( unsigned int aRELE, unsigned int aMFD, unsigned int aSTS, unsigned int aSTD, unsigned int aRCYC ) {
	
 *DMA_REG_CTRL = 0x00000000;
 *DMA_REG_STAT = 0x00000000;
 *DMA_REG_PCR  = 0x00000000;
 *DMA_REG_SQWC = 0x00000000;
 *DMA_REG_RBSR = 0x00000000;
 *DMA_REG_RBOR = 0x00000000;
 *DMA_REG_CTRL = DMA_SET_CTRL( 1, aRELE, aMFD, aSTS, aSTD, aRCYC );

}  /* end DMA_Init */

void DMA_ChannelInit ( int aChannel ) {

 *DMA_ADDR( DMA_CHCR[ aChannel ] ) = 0x00000000;
 *DMA_ADDR( DMA_MADR[ aChannel ] ) = 0x00000000;

 if ( DMA_SIZE[ aChannel ] != 0 ) *DMA_ADDR( DMA_SIZE[ aChannel ] ) = 0x00000000;
 if ( DMA_TADR[ aChannel ] != 0 ) *DMA_ADDR( DMA_TADR[ aChannel ] ) = 0x00000000;
 if ( DMA_ASR0[ aChannel ] != 0 ) *DMA_ADDR( DMA_ASR0[ aChannel ] ) = 0x00000000;
 if ( DMA_ASR1[ aChannel ] != 0 ) *DMA_ADDR( DMA_ASR1[ aChannel ] ) = 0x00000000;
 if ( DMA_SADR[ aChannel ] != 0 ) *DMA_ADDR( DMA_SADR[ aChannel ] ) = 0x00000000;

 *DMA_ADDR( DMA_QWC [ aChannel ] ) = 0x00000000;

}  /* end DMA_ChannelInit */

void DMA_SendSPR ( int aChannel, unsigned char* apData, int aQWC ) {

 *DMA_ADDR( DMA_MADR[ aChannel ] ) = DMA_SET_MADR(  ( unsigned int )apData, 1  );
 *DMA_ADDR( DMA_QWC [ aChannel ] ) = aQWC;
 *DMA_ADDR( DMA_CHCR[ aChannel ] ) = DMA_SET_CHCR( 0, 0, 0, 0, 0, 1, 0 );

}  /* end DMA_SendSPR */
