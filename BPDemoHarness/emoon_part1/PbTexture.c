#include "PbTexture.h"
#include "PbGs.h"
#include "PbDma.h"
#include "PbGlobal.h"
#include "PbSpr.h"
#include "ps2gs.h"
#include "gs.h"
#include "PbGs.h"
#include "gif.h"
#include "dma.h"

#define GIF_TAG_2(NLOOP,EOP,PRE,PRIM,FLG,NREG) \
		((u64)(NLOOP)<< 0)		| \
		((u64)(EOP)	 << 15)		| \
		((u64)(PRE)	 << 46)		| \
		((u64)(PRIM) << 47)	  | \
		((u64)(FLG)	 << 58)		| \
		((u64)(NREG) << 60);

static u32 test_texture[256*256] __attribute__((aligned(16)));
extern u32 tex_pointer; // highly temporary
extern u32 Envmap[];
extern u16 gs_texture_wh(u16 n);

static u64 temp[64] __attribute__((aligned(16)));

DECLARE_EXTERN_GS_PACKET(dma_buf);

///////////////////////////////////////////////////////////////////////////////
// PbTexture_Init
// Envmap, 
///////////////////////////////////////////////////////////////////////////////

void PbTexture_SetActive( u32 Texture,int Context )
{

  BEGIN_GS_PACKET(dma_buf);

  GIF_TAG_AD(dma_buf, 1, 1, 0, 0, 0);

//  GIF_DATA_AD(dma_buf, PS2_GS_TEXFLUSH, 0x42);
  
  GIF_DATA_AD(dma_buf, PS2_GS_TEX0_1,
	      PS2_GS_SETREG_TEX0_1(
				   tex_pointer / 64,	// base pointer
				   256  / 64,	// width
				   0,					// 32bit RGBA
				   gs_texture_wh(256),	// width
				   gs_texture_wh(256),	// height
				   1,					// RGBA
				   PS2_GS_TEX_TFX_DECAL,		       
				   0,0,0,0,0));

//  GIF_DATA_AD(dma_buf, PS2_GS_TEX1_1, PS2_GS_SETREG_TEX1_1(0, 0, 1, 1, 0, 0, 0));
//  GIF_DATA_AD(dma_buf, PS2_GS_CLAMP_1, PS2_GS_SETREG_CLAMP( 0, 0, 0, 256, 0, 256) );


  PbDma_Wait02();
  SEND_GS_PACKET(dma_buf);

  //out( "smurf\n" );

/*
  u64* p_data = PbSpr_Alloc( 4*16, TRUE );

  p_data[0] = GIF_TAG_2( 3, 1, 0, 0, 0, 1 );
  p_data[1] = GS_AD;
  
  p_data[2] = 0x42;
  p_data[3] = GS_REG_TEXFLUSH;

  p_data[4] =  PS2_GS_SETREG_TEX0_1(
                            		    0 / 256,	// base pointer
		                                256  / 64,	// width
		                                0,					// 32bit RGBA
		                                gs_texture_wh(256),	// width
		                                gs_texture_wh(256),	// height
		                                1,					// RGBA
		                                PS2_GS_TEX_TFX_DECAL,	0,0,0,0,0 );
  p_data[5] = PS2_GS_TEX0_1;

  p_data[6] = PS2_GS_SETREG_TEX1_1(0, 0, 1, 1, 0, 0, 0);
  p_data[7] = GS_REG_TEX1_1;

  PbDma_Wait02();
  PbDma_Send02( p_data, 4, 1 ); 
*/
}

///////////////////////////////////////////////////////////////////////////////
// PbTexture_Send
// Send a texture over to gs-ram, builds temporary list in SPR
///////////////////////////////////////////////////////////////////////////////

u32 PbTexture_Send( PbTexture* pTexture, u32 Dest )
{

  u64* p_data     = (u64*)0x70000000;
  int n_quads     = 0; 
  int curr_quads  = 0;
  u32*  p_texture = (u32*)Envmap;
  int x;
  int y;
  int i;
/*  
  for( i = 0; i < 256*256; i++ )
    test_texture[i] = 0;


  /////////////////////////////////////////////////////////////////////////////
  // Generate some funky texture.

  for( y = 0; y < 252; y++ )
  {
    for( x = 0; x < 255; x++ )
      test_texture[(y*256)+x] = *p_texture++;

    p_texture += 265-255;
  }
*/
  Dest = tex_pointer; // highly temporary

  load_image( &Envmap, Dest, 256, 0, 0, 0, 256, 256 );

/*     
  /////////////////////////////////////////////////////////////////////////////
  // temporary settings for now

  int xsize        = 256;
  int ysize        = 256;
  int buffer_width = 256;         // hardcoded 32bit
  int pixelformat  = GS_PSMCT32;

  /////////////////////////////////////////////////////////////////////////////
  // skip the header for now

  //pTexture += 1;
  p_texture = (char*)&test_texture;
  Dest = tex_pointer; // highly temporary

  /////////////////////////////////////////////////////////////////////////////
  // Setup registers

  *p_data++ = DMA_CNT_TAG( 5 );
  *p_data++ = 0;

  *p_data++ = GS_GIF_TAG( 1, 0, 0, 0, 1, 4 ); 
  *p_data++ = GS_AD;

  /////////////////////////////////////////////////////////////////////////////
  // Setup the blitting

  *p_data++ = (u64)( ((pixelformat & 0x3f) << 24) | (((buffer_width>>6) & 0x3f)<<16) | 
                      ((Dest/64) & 0x3fff) ) << 32;
  *p_data++ = GS_REG_BITBLTBUF;

  *p_data++ = (unsigned long)( ((0)<<16) | (0) ) << 32;  // x,y offsets 0 for now
  *p_data++ = GS_REG_TRXPOS;

  *p_data++= ((unsigned long)ysize << 32) | xsize;
  *p_data++ = GS_REG_TRXREG;

  *p_data++ = 0;
  *p_data++ = GS_REG_TRXDIR;

  /////////////////////////////////////////////////////////////////////////////
  // Calculate number of quads

  n_quads = xsize * ysize;  
  n_quads = (n_quads >> 2) + ((n_quads & 0x03 ) != 0 ? 1 : 0);

  /////////////////////////////////////////////////////////////////////////////
  // send the data

  while( n_quads )
  {
    if( n_quads > IMAGE_MAX_COUNT ) 
       curr_quads = IMAGE_MAX_COUNT;
    else 
      curr_quads = n_quads;

    *p_data++ = DMA_CNT_TAG( 1 );
    *p_data++ = 0; // pad

    *p_data++ = 0x0800000000000000 | curr_quads; // TODO: use proper macro
    *p_data++ = 0; // pad

    *p_data++ = DMA_REF_TAG( (u32)p_texture, curr_quads );
    *p_data++ = 0; // pad

    n_quads  -= curr_quads;
    p_texture += (curr_quads << 4);
  }

  /////////////////////////////////////////////////////////////////////////////
  // end the list

  *p_data++ = DMA_END_TAG( 0 );
  *p_data++ = 0;

  p_data = (u64*)0x70000000;

  /////////////////////////////////////////////////////////////////////////////
  // Make sure the dma is ready and send the chain (no flush as list is in SPR)

  PbDma_Wait02();
  PbDma_Send02Chain( (void*)0x70000000, 1 );
*/
  return 0;
}
/*
  for( y = 0; y < 255; y++ )
  {
    for( x = 0; x < 255; x++ )
    {
      int t_x = x - 128;
      int t_y = y - 128;

      float distance = sqrt( t_x*t_x + t_y*t_y );
      float f_color = 0.0f;
  
      if( distance != 0 )
        f_color = 1.0f / distance;

      f_color *= 111512;

      if( f_color > 255 )
        f_color = 255;

      unsigned int color = (unsigned int)f_color;   

     test_texture[(y*256)+x] = Envmap[(x*127)+y];
    }
  }
*/
