#include "PbPart1.h"
#include "PbDma.h"
#include "PbVif.h"
#include "PbMeshData.h"
#include "PbGlobal.h"
#include "PbDma.h"
#include "PbVu1.h"
#include "PbGs.h"
#include <kernel.h>
#include <tamtypes.h>

static u64 test[1024*400];

/////////////////////////////////////////////////////////////////////////////////////////
// void* PbPart1_DrawObject
/////////////////////////////////////////////////////////////////////////////////////////

void* PbPart1_DrawObject( PbMatrix* pCameraToScreen,PbMatrix* pWorlToCamera,
                          float* pAngles, void* pChain )
{
  u16 i = 0;
  s32 num_coords;
  void* p_store_chain = pChain = (void*)&test;
  u32* p_object = (u32*)PbMeshData_Get( 0 );

  /////////////////////////////////////////////////////////////////////////////////////
  // Setup double buffering

  *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

  *((u32*)pChain)++ = VIF_CODE( VIF_STMOD,0,0 );  // normalmode
  *((u32*)pChain)++ = VIF_CODE( VIF_BASE,0,9 );
  *((u32*)pChain)++ = VIF_CODE( VIF_OFFSET,0,512-9 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

  ////////////////////////////////////////////////////////////////////////////////////
  // Add CameraToScreen & WorldToCamera & angles to the list

  *((u64*)pChain)++ = DMA_REF_TAG( (u32)pCameraToScreen, 4 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,4,0 );

  *((u64*)pChain)++ = DMA_REF_TAG( (u32)pWorlToCamera, 4 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,4,4 );

  *((u64*)pChain)++ = DMA_REF_TAG( (u32)pAngles, 1 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,1,8 );
  
  // add object to the list, max 240 coords for each buffer

  u32 num_sections = *(p_object);
  p_object += 4;  // pad

  num_sections = 1; // just 1 for now.

  for( i = 0; i < num_sections; i++ )
  {
    ////////////////////////////////////////////////////////////////////////////////
    // skip forward so we end up on a qword aligned adress.

    num_coords = (*p_object);
    p_object += 4; // pad

    //num_coords = 340;

    int set_gif = 1;
  
    //p_object += 110*4;

    ////////////////////////////////////////////////////////////////////////////////////
    // Add giftag we need for the drawing.

    while( num_coords > 0 )
    {
      s32 current_count = num_coords > 112 ? 112 : num_coords; 
      //s32 current_count = 220; 
    
      ////////////////////////////////////////////////////////////////////////////////////
      // Add GifTag to list

      *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
      *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
      *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,1,VIF_UNPACK_DBLBUF | 0 );

      if( set_gif == 1 )
      {
        *((u64*)pChain)++ = GS_GIF_TAG( 2, 0, GS_PRIM_TRIANGLE_STRIP, 1, 1, current_count );
        *((u64*)pChain)++ = 0x51; // registers to set
        set_gif = 0;
      }
      else
      {
        *((u64*)pChain)++ = GS_GIF_TAG( 2, 0, GS_PRIM_TRIANGLE_STRIP, 0, 1, current_count );
        *((u64*)pChain)++ = 0x51; // pad
      }

      ////////////////////////////////////////////////////////////////////////////////////
      // Add Coordinates to list

      *((u64*)pChain)++ = DMA_REF_TAG( (u32)p_object, current_count );
      *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
      *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,current_count,VIF_UNPACK_DBLBUF | 1 );

      ////////////////////////////////////////////////////////////////////////////////////
      // Add Call the program

      *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
      *((u32*)pChain)++ = VIF_CODE( VIF_MSCAL,0,0 );
      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

      p_object += 112*4;
      num_coords -= 112;
    }
  }

  *((u64*)pChain)++ = DMA_END_TAG( 0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

  FlushCache(0);

  PbDma_Wait01(); // lets make sure the dma is ready.
  PbDma_Send01Chain( p_store_chain, 0 );

  return NULL;
}


//    *((u32*)pChain)++ = VIF_CODE( VIF_STMOD,0,VIF_MODE_DIFFERENCE );  
//    *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_8,140,5 );

