#include "PbPart1.h"
#include "PbDma.h"
#include "PbVif.h"
#include "PbMeshData.h"
#include "PbDma.h"
#include "PbVu1.h"
#include "PbGs.h"
#include "PbMatrix.h"
#include "PbTexture.h"
#include "PbGfx.h"
#include "vram_malloc.h"
#include <tamtypes.h>
#include <kernel.h>
#include "ps2gs.h"
#include "gs.h"
#include "shapes.h"
#include "PbGlobal.h"
#include "PbSpr.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////////////////

static u64 test[1024*400];
static float g_Time = 0.0f;
static float angle = 0.0f;
static int g_PbP1_State = PBP1_STATE_INIT_FLASH; // Begin Part with flash
static PbMatrix ViewScreenMatrix;
static PbMatrix CameraMatrix;
u32 RenderTarget;
extern u32 tex_pointer;

/////////////////////////////////////////////////////////////////////////////////////////
// PbPart1_Update( float DeltaTime )
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_Update( float DeltaTime )
{
  PbGs_SetZbufferTest( 1, GS_CONTEXT_1 );
  PbGfx_ClearScreen();

  switch( PbPart1_GetState() )
  {
    case PBP1_STATE_INIT_FLASH  : PbPart1_InitFlash();
    case PBP1_STATE_FLASH       : PbPart1_Flash( DeltaTime );
    case PBP1_STATE_INIT_NORMAL : PbPart1_InitNormal();
    case PBP1_STATE_NORMAL      : PbPart1_Normal( DeltaTime );
  }

  PbGfx_Update();
  g_Time += DeltaTime;
}

/////////////////////////////////////////////////////////////////////////////////////////
// void* PbPart1_Init
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_InitFlash()
{
  PbPart1_SetState( PBP1_STATE_FLASH );
}

/////////////////////////////////////////////////////////////////////////////////////////
// void PbPart1_Flash
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_Flash( float DeltaTime )
{
  PbPart1_SetState( PBP1_STATE_INIT_NORMAL );
}

/////////////////////////////////////////////////////////////////////////////////////////
// void PbPart1_InitNormal
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_InitNormal()
{
  RenderTarget = vram_malloc( 256*256 );
  out( "RenderTarget: 0x%x\n", RenderTarget );

  PbPart1_SetState( PBP1_STATE_NORMAL );
}

/////////////////////////////////////////////////////////////////////////////////////////
// void PbPart1_Normal
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_Normal( float DeltaTime )
{
  void* p_data;  
  void* p_store;  

  PbTexture_SetActive( tex_pointer, GS_CONTEXT_1 );

  p_data = p_store = PbSpr_Alloc( 20*16, TRUE );

  //////////////////////////////////////////////////////////////////////////////////////
  // Setup the render target

  *((u64*)p_data)++ = DMA_CNT_TAG( 3 );
  *((u32*)p_data)++ = VIF_CODE( VIF_NOP, 0, 0 );
  *((u32*)p_data)++ = VIF_CODE( VIF_DIRECT, 0, 3 );
  
  *((u64*)p_data)++ = GS_GIF_TAG( 1, 0, 0, 0, 1, 2 );
  *((u64*)p_data)++ = GS_AD;

  *((u64*)p_data)++ = GS_SETREG_FRAME_1( RenderTarget / 2048, 256 / 64, 0, 0 );
  *((u64*)p_data)++ = GS_REG_FRAME_1;

  *((u64*)p_data)++ = PS2_GS_SETREG_SCISSOR_1(0, 256-1, 0, 256-1);
  *((u64*)p_data)++ = PS2_GS_SCISSOR_1;

  *((u64*)p_data)++ = DMA_END_TAG( 0 );
  *((u32*)p_data)++ = VIF_CODE( VIF_FLUSHA, 0, 0 );
  *((u32*)p_data)++ = VIF_CODE( VIF_FLUSH, 0, 0 );
  
  PbDma_Wait01();
  PbDma_Send01Chain( p_store, TRUE );
  PbDma_Wait01();

  fill_rect( 0,0,256,256,0,127 );
  PbDma_Wait01();

  //PbGs_SetZbufferTest( 2, GS_CONTEXT_1 );

  PbGs_SetZbufferTest( 2, GS_CONTEXT_1 );

  // Set render target to texture

  PbPart1_Main( &ViewScreenMatrix, &CameraMatrix );

  // Ugly, but this is a demo ok? :)

  PbDma_Wait01();
  PbVu1_Wait();

  while( *GIF_STAT & 1 << 10 ) 
    ;


  PbGfx_SetActiveScreen();

  PbDma_Wait02();

  while( *GIF_STAT & 1 << 10 ) 
    ;

  rect r;

  r.col = 0;
  r.v[0].col = 0;
  r.v[0].lit = 255;
  r.v[0].u   = 0;
  r.v[0].v   = 0;
  r.v[0].x   = 0;
  r.v[0].y   = 0;
  r.v[0].z   = 0;

  r.col = 0;
  r.v[1].col = 0;
  r.v[1].lit = 255;
  r.v[1].u   = 256;
  r.v[1].v   = 256;
  r.v[1].x   = 640;
  r.v[1].y   = 256;

  PbGs_SetZbufferTest( 1, GS_CONTEXT_1 );
  
  fill_rect_tex( r, RenderTarget, 256, 256 );
    
  //p_data[0] = 
}

/////////////////////////////////////////////////////////////////////////////////////////
// void* PbPart1_Init
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_Main( PbMatrix* pScreenToView, PbMatrix* pCameraMatrix )
{
  PbMatrix CombindedMatrix;
  PbMatrix FinalMatrix;
  PbMatrix RotateMatrix;
  PbMatrix RotateMatrix2;

  angle += 0.01;

  PbMatrix_BuildPitch( &RotateMatrix2, angle );
  PbMatrix_BuildHeading( &RotateMatrix, angle );
  PbMatrix_Multiply( &RotateMatrix, &RotateMatrix, &RotateMatrix2 );
  PbMatrix_Multiply( &CombindedMatrix, &RotateMatrix, pCameraMatrix );
  PbMatrix_Multiply( &FinalMatrix, &CombindedMatrix, pScreenToView );

  PbPart1_DrawEnvmapped( &FinalMatrix,&RotateMatrix, &test );
}

/////////////////////////////////////////////////////////////////////////////////////////
// void* PbPart1_DrawObject
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_DrawEnvmapped( PbMatrix* pScreenToView,PbMatrix* pObjectToWorld, void* pChain )
{
  s32  num_coords     = 0;
  u32  num_sections   = 0;
  u16  i              = 0;
  u32* p_object       = NULL;
  void* p_store_chain = pChain;;

  p_object = (u32*)PbMeshData_Get( 0 );

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

  *((u64*)pChain)++ = DMA_REF_TAG( (u32)pScreenToView, 4 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,4,0 );

  *((u64*)pChain)++ = DMA_REF_TAG( (u32)pObjectToWorld, 4 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,4,4 );
  
  // add object to the list, max 120 coords for each buffer

  num_sections = *(p_object);
  p_object += 4;  // pad

  num_sections = 1; // just 1 for now.

  for( i = 0; i < num_sections; i++ )
  {
    int set_gif = 1;

    ////////////////////////////////////////////////////////////////////////////////
    // skip forward so we end up on a qword aligned adress.

    num_coords = (*p_object);
    p_object += 4; // pad

    ////////////////////////////////////////////////////////////////////////////////////
    // Add giftag we need for the drawing.

    while( num_coords > 0 )
    {
      s32 current_count = num_coords > 80 ? 80 : num_coords; 
    
      ////////////////////////////////////////////////////////////////////////////////////
      // Add GifTag to list

      *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
      *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
      *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,1,VIF_UNPACK_DBLBUF | 0 );

      if( set_gif == 1 )
      {
        *((u64*)pChain)++ = GS_GIF_TAG( 3, 0, GS_SET_PRIM( GS_PRIM_TRIANGLE_STRIP, 1, 1, 0, 0, 0, 1, 0, 0 ),
                                        1, 1, current_count );
        *((u64*)pChain)++ = 0x531; // registers to set
        set_gif = 0;
      }
      else
      {
        *((u64*)pChain)++ = GS_GIF_TAG( 3, 0, GS_SET_PRIM( GS_PRIM_TRIANGLE_STRIP, 1, 1, 0, 0, 0, 1, 0, 0 ), 
                                        0, 1, current_count );
        *((u64*)pChain)++ = 0x531; // pad
      }

      ////////////////////////////////////////////////////////////////////////////////////
      // Add Coordinates to list

      *((u64*)pChain)++ = DMA_REF_TAG( (u32)p_object, current_count*2 );
      *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
      *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,current_count*2,VIF_UNPACK_DBLBUF | 1 );

      ////////////////////////////////////////////////////////////////////////////////////
      // Add Call the program

      *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
      *((u32*)pChain)++ = VIF_CODE( VIF_MSCAL,0,0 );
      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
      *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

      p_object += 80*4*2;
      num_coords -= 80;
    }
  }

  *((u64*)pChain)++ = DMA_CNT_TAG( 2 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP, 0, 0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_DIRECT, 0, 2 );
  
  *((u64*)pChain)++ = GS_GIF_TAG( 1, 0, 0, 0, 1, 1 );
  *((u64*)pChain)++ = GS_AD;

  *((u64*)pChain)++ = 0x42;
  *((u64*)pChain)++ = PS2_GS_TEXFLUSH;

  *((u64*)pChain)++ = DMA_END_TAG( 0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_FLUSHA, 0, 0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_FLUSH, 0, 0 );

  FlushCache(0);

  PbDma_Wait01(); // lets make sure the dma is ready.
  PbDma_Send01Chain( p_store_chain, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// void* PbPart1_DrawObject
/////////////////////////////////////////////////////////////////////////////////////////

void* PbPart1_DrawObject( PbMatrix* pCameraToScreen,PbMatrix* pWorlToCamera,
                          float* pAngles, void* pChain )
{
  s32  num_coords   = 0;
  u32  num_sections = 0;
  u16  i            = 0;
  u32* p_object    = NULL;
  void* p_store_chain = pChain = (void*)&test;

  p_object = (u32*)PbMeshData_Get( 0 );

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
  
  // add object to the list, max 120 coords for each buffer

  num_sections = *(p_object);
  p_object += 4;  // pad

  num_sections = 1; // just 1 for now.

  for( i = 0; i < num_sections; i++ )
  {
    int set_gif = 1;

    ////////////////////////////////////////////////////////////////////////////////
    // skip forward so we end up on a qword aligned adress.

    num_coords = (*p_object);
    p_object += 4; // pad

    ////////////////////////////////////////////////////////////////////////////////////
    // Add giftag we need for the drawing.

    while( num_coords > 0 )
    {
      s32 current_count = num_coords > 112 ? 112 : num_coords; 
    
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

//########################################################################################
// Init functions and State functions
//########################################################################################

/////////////////////////////////////////////////////////////////////////////////////////
// void PbPart1_SetupVu1
// Adds the vu1 programs needed
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_SetupVu1()
{
  PbVu1_AddToChain( 1, (void*)0x70000000, 0 );
  PbDma_Wait01();
  PbDma_Send01Chain( (void*)0x70000000, 1 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// void PbPart1_SetupTextures()
// Setup the textures needed
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_SetupTextures()
{
  PbTexture_Send( NULL, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// void PbPart1_SetupTextures()
// Setup the textures needed
/////////////////////////////////////////////////////////////////////////////////////////

void PbPart1_SetupGeneral()
{
  PbMatrix_CreateViewScreen( &ViewScreenMatrix, 
                             212.0f,1.0f,1.0f, 1024+256/2,(1024+256/2),
                             1.0f, 6777215.0f,64.0f, 5536.0f );

  PbMatrix_MakeIdentity( &CameraMatrix );
  PbMatrix_Translate( &CameraMatrix, 0, 0, 400  );

}

///////////////////////////////////////////////////////////////////////////////////////
// int PbPart1_SetInitalTime()
// Should be inline really
///////////////////////////////////////////////////////////////////////////////////////

void PbPart1_SetInitalTime( float Time )
{
  g_Time = Time;
}

///////////////////////////////////////////////////////////////////////////////////////
// int PbPart1_GetState()
// Should be inline really
///////////////////////////////////////////////////////////////////////////////////////

int PbPart1_GetState()
{
  return g_PbP1_State;
}

///////////////////////////////////////////////////////////////////////////////////////
// void PbPart1_SetState
// Should be inline really
///////////////////////////////////////////////////////////////////////////////////////

void PbPart1_SetState( int State )
{
  g_PbP1_State = State;
}


