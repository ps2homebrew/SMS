/*
 * main.c - breakpoint demo part adresd.
 * This is a 3d distort/blur text effect for titles
 * Based on a routine by emoon, with changes.
 *
 * Copyright (c) 2004   adresd <adresd_ps2dev@yahoo.com>
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 * Makes use of PbDemoLib by emoon
 * Copyright (c) 2004   emoon <daniel@collin.com>
 */
#include <tamtypes.h>
#include <kernel.h>
#include <loadfile.h>
#include "../harness.h"
#include "../PbDemoLib/PbScreen.h"
#include "../PbDemoLib/PbScreen.h"
#include "../PbDemoLib/PbDma.h"
#include "../PbDemoLib/PbGs.h"
#include "../PbDemoLib/PbScreen.h"
#include "../PbDemoLib/PbMisc.h"
#include "../PbDemoLib/PbTexture.h"
#include "../PbDemoLib/PbSpr.h"
#include "../PbDemoLib/PbMatrix.h"
#include "../PbDemoLib/PbVif.h"
#include "../PbDemoLib/PbVu1.h"
#include "../PbDemoLib/PbPrim.h"
#include "../PbDemoLib/PbMath.h"
#include "../PbDemoLib/PbVec.h"


#define GIF_STAT        ((volatile unsigned int *)(0x10003020))

///////////////////////////////////////////////////////////////////////////////
// Data used in the part
///////////////////////////////////////////////////////////////////////////////

const demo_init_t* gp_Info;
PbTexture* gp_Envmap        = NULL;
PbTexture* gp_RenderTarget  = NULL;
PbTexture* gp_RenderTarget2 = NULL;
PbTexture* gp_RenderTarget3 = NULL;
extern u32 TestObj;
extern char binary_oddments_start[];;

extern u32 OddmentsObject;


extern u16 font_lookup[];

u16* gsw_charSizesPointer = font_lookup;


extern u32 ps2dev_tex[];

PbTexture* p_texture_ps2dev = NULL; // ps2dev logo tex

extern u32 presents_tex[];

PbTexture* p_texture_presents = NULL; 


extern u32 font_tex[];

PbTexture* p_texture_font = NULL; 
///////////////////////////////////////////////////////////////////////////////
// Reference to our vu1 program
///////////////////////////////////////////////////////////////////////////////

extern u32 g_PbVu1_Distort __attribute__((section(".vudata")));
extern u32 g_PbVu1_Distort_End __attribute__((section(".vudata")));

///////////////////////////////////////////////////////////////////////////////
// References to functions lower in the code
///////////////////////////////////////////////////////////////////////////////

void PbDistortSetup();
void PbDistortSetTexture( PbTexture* pTexture );
void PbDistortRadialBlur( PbTexture* pSource,PbTexture* pDest, float cx, float cy, 
                        float v, int n, int mix1, int mix2 );

void* PbDistort_DrawObject( PbMatrix* pCameraToScreen,PbMatrix* pWorldToCamera,
                          PbFvec* pAngles);

///////////////////////////////////////////////////////////////////////////////
// DoStuff
///////////////////////////////////////////////////////////////////////////////
float abs(float x)
{
  if (x > 0) return x;
  else return x * -1.0f;
}


u32 MakeCol(u8 r, u8 g, u8 b, u8 a)
{
    return (r<< 0) | (g<< 8) | (b<<16) | (a<<24);
}


int GetCharWidth(char ch)
{
    return (int)(*(&gsw_charSizesPointer[(ch*4)+2]) - 
				*(&gsw_charSizesPointer[ch*4]));
}

u8 CalcZAlpha(float z)
{
    if (z>1.0f)
        return(u8)(0x80/z);
    else 
        return (u8)(0x80*z);
}



void zs_ZoomText(char text, s16 xp, s16 yp, int z,
				  float zoom)
{
	char currentChar; // current character
	u16	*charRect; // current char tex coords	
    u16	x0, y0, x1, y1;	// rectangle for current character
	u16 w, h; // width and height of above rectangle
	u16 offsetY; // to compensate for shrunken char
	
	// Read the texture coordinates for current character
	charRect = gsw_charSizesPointer+(text*4);		
	x0 = *charRect++;
	y0 = *charRect++;
	x1 = *charRect++;
	y1 = *charRect++;
	//x0++;
	//x1;
	//y1++;
	//printf("%d\n",gsw_GetCharSizes());
	w  = x1-x0+1;

	if (1) // are we scaling horizontally?
	{
		w  = w * zoom;
	}	

	h  = y1-y0+1;
	
	
	if (1) // are we scaling vertically?
	{
		offsetY = h - (h * zoom);
		h  = h * zoom;
		offsetY = h / 2;
		yp-=(s16)offsetY;
	}		
	

	//printf("%d\n",text);


    /*
	if (gsw_stretch_font == ENABLED)
	{
		w = w*2;
	}*/
	
    PbPrimSpriteTexture( p_texture_font, 
                        xp<<4, yp<<4,  
						x0<<4, y0<<4, 
                        (xp+w)<<4, (yp+h)<<4,
						x1<<4, y1<<4,
						100, MakeCol(0x80,0x80,0x80,CalcZAlpha(zoom) ) ); 
	

}






int logoFadeout = 0;
int logoOffset = 256;
int logoAlpha = 0x00;
int preAlpha = 0x00;
int preToggle = 0;
int counter = 0;
int drawText = 0;

char strapline [] = {"THE PROJECT BREAKPOINT DEMO"};
int strapAlpha[27];
float strapZoom[27];

void InitTextStuff()
{
    int i;
    float zv = 3.0f;
    float zvInc = 0.01f;

    for (i=0; i<27; i++)
    {   
        strapAlpha[i]=0x00;
        strapZoom[i]=zv;
        zv+=zvInc;
        zvInc+=0.1f;
    }
}

void DrawDevLogo()
{
    int i;
    int toff=0;

    PbPrimSpriteTexture( p_texture_ps2dev, 
                        100<<4, -30<<4,  
						0<<4, 0<<4, 
                        500<<4, 120<<4,
						512<<4, 256<<4,
						100, MakeCol(0x80,0x80,0x80,logoAlpha) ); 

    PbPrimSpriteTexture( p_texture_presents, 
                        170<<4, 80<<4,  
						0<<4, 0<<4, 
                        426<<4, 144<<4,
						256<<4, 64<<4,
						100, MakeCol(0x80,0x80,0x80,preAlpha) ); 

    
    if (logoFadeout)
    {
        if (logoAlpha>0)
        {
            logoAlpha--;
        }
    }else{
        if (logoAlpha<0x80)
        {
            logoAlpha++;
        }else{
            if (!preToggle)
            {
                if (preAlpha<0x80)
                {
                    preAlpha++;
                }else{
                    preToggle=1;
                }            
            }else{
                if (counter<60)
                {
                    counter++;
                }else{
                    if (preAlpha>0x00)
                    {
                        preAlpha--;
                    }      
                }
            }
        }
    }

    if (logoFadeout)
    {
        if (drawText)
        {
            for (i=0; i<27; i++)
            {
                zs_ZoomText(*(strapline+i),90+toff,215,100,strapZoom[i]);
                toff += GetCharWidth(*(strapline+i))+1;
                if (strapZoom[i]>0.0f)strapZoom[i]-=0.1f;
                //printf("%c ",*(strapline+i));
            }    
        }
    }else{
        if (drawText)
        {
            for (i=0; i<27; i++)
            {
                zs_ZoomText(*(strapline+i),90+toff,215,100,strapZoom[i]);
                toff += GetCharWidth(*(strapline+i))+1;
                if (strapZoom[i]>1.0f)strapZoom[i]-=0.3f;
                //printf("%c ",*(strapline+i));
            }    
        }
    }
}
u32 start_demo( const demo_init_t* pInfo )
{
  PbMatrix viewscreen_matrix;
  PbMatrix camera_matrix;
  PbMatrix rotate_matrix;
  PbMatrix rotate_matrix2;
  PbMatrix final_matrix;
  PbMatrix temprot_matrix;
  PbMatrix temprot_matrix1;
  PbMatrix temprot_matrix2;
  PbMatrix temprot_matrix3;
  int      offset_x;
  int      offset_y;
  float    angle = -0.01f;
  int      i = 0;
  // .x is twist multiplier, z is the blur distance, w is the base angle for twist
  PbFvec   angles = {-0.0f,1.0f,1.0f,0.06f};
  int up = 1;
  float twist_timer = 0.0f;

  float static_timer = 0.0f;
  /////////////////////////////////////////////////////////////////////////////
  // Setup screen

  PbScreenSetup( SCR_W, SCR_H, SCR_PSM );

  offset_x = PbScreenGetOffsetX();
  offset_y = PbScreenGetOffsetY();

  /////////////////////////////////////////////////////////////////////////////
  // Upload our program

  PbVif1UploadPrg( 0, &g_PbVu1_Distort, &g_PbVu1_Distort_End );

  /////////////////////////////////////////////////////////////////////////////
  // Create our view to screen matrix
  PbMatrixViewScreen( &viewscreen_matrix, 512.0f,1.0f,1.0f, // 0.5,0.8// 1.3f for last
                      offset_x+512/2,offset_y+200/2,
                      1.0f, 6777215.0f,64.0f, 5536.0f );
  PbDistortSetup();

  
  // init textwriter stuff
  InitTextStuff();

  /////////////////////////////////////////////////////////////////////////////
  // Setup the matrix for our camera

  PbMatrixIdentity( &camera_matrix );
  PbMatrixIdentity( &rotate_matrix );
  PbMatrixIdentity( &rotate_matrix2 );
  PbMatrixTranslate( &camera_matrix, 0, 0, 280 );

  // This sets anim up
  twist_timer = 1.0f;
  angle = 0.7f;
  up = 0;

  p_texture_ps2dev = PbTextureCreate32( ps2dev_tex, 512, 256 ); 
  PbTextureUpload( p_texture_ps2dev);

  p_texture_presents = PbTextureCreate32( presents_tex, 256, 64 ); 
  PbTextureUpload( p_texture_presents);

  p_texture_font = PbTextureCreate32( font_tex, 1024, 64 ); 
  PbTextureUpload( p_texture_font);

  PbPrimSetAlpha( 0, 1, 0, 1, 0x80 );

  PbSetLinearFiltering();
  /////////////////////////////////////////////////////////////////////////////
  // Loop of the demo

  while( pInfo->time_count > 0 )
  {
    // angles.x is the rotate/twist mupliplier
    if (up == 1)
    { // This is fade out
        logoFadeout = 1;
        twist_timer += 0.013f;
        // This is the rotation
        angle += 0.02f;
        if (twist_timer > 1.0f) {up = 0; angle = 0.7f;twist_timer = 1.0f;} // Finished
        angles.x = twist_timer * -1.0f;
    }
    else if (up == 2)
    {
        drawText = 1;
        // this is the pause
        if (static_timer<1.0f)
        {
            static_timer+=0.008f;
        }else{
            up = 1;
        }

    }else{ // This is fade in
        twist_timer -= 0.002f;
        // This is the rotation
        angle += 0.01f;
        if (twist_timer < 0.0f){ up = 2;}
        angles.x = twist_timer;
    }
    // effect should be 1.0f for none, 0.5 (ish) for full ( this is blur)
    angles.z = 1.0f -( twist_timer / 1.5f);
 
    PbScreenSetCurrentActive();   

    PbScreenClear( 30<<16|20<<8|40 );

    
    PbTextureCopy( gp_RenderTarget2, gp_RenderTarget, TRUE );
    PbDistortRadialBlur( gp_RenderTarget2, gp_RenderTarget, 256, 128, 
                       PbSqrt(angles.z),6,115 -(int)(abs(twist_timer)*30.0f) ,20 );
    PbScreenSetCurrentActive();
    PbPrimSpriteTexture( gp_RenderTarget2, 
                         0<<4,  0<<4,   512<<4,   0<<4, 
                         640<<4, 256<<4, 0<<4, 255<<4, 0, 127<<16|127<<8|127 );

    PbPrimSetState( PB_ALPHA_BLENDING, PB_ENABLE );
    DrawDevLogo(); 
    PbPrimSetState( PB_ALPHA_BLENDING, PB_DISABLE );
    PbTextureSetRenderTarget( gp_RenderTarget );

    PbDmaWait02();

    PbPrimSpriteNoZtest( 0, 0, 512<<4, 256<<4, 0, 0 );
    
    PbMatrixRotateX( &rotate_matrix, angle );
    PbMatrixRotateY( &rotate_matrix2, angle );
    PbMatrixMultiply( &rotate_matrix, &rotate_matrix, &rotate_matrix2 );
    PbMatrixMultiply( &temprot_matrix, &rotate_matrix, &camera_matrix );
    PbMatrixMultiply( &final_matrix, &temprot_matrix, &viewscreen_matrix );
    FlushCache(0);  
    PbDistort_DrawObject( &viewscreen_matrix,&temprot_matrix,&angles);

    //PbTextureSetRenderTarget( gp_RenderTarget );

    //PbScreenSetCurrentActive();
    
    PbScreenSyncFlip();
  }
  
  return pInfo->screen_mode;
}


///////////////////////////////////////////////////////////////////////////////
// DoStuff
///////////////////////////////////////////////////////////////////////////////

void PbDistortSetup()
{
  gp_RenderTarget  = PbTextureCreate32( NULL, 512, 256 );
  gp_RenderTarget2 = PbTextureCreate32( NULL, 512, 256 );
  gp_Envmap        = PbTextureCreate32( (u32*)binary_oddments_start, 256, 256 );

  PbTextureUpload( gp_Envmap );
}

void* PbDistort_DrawObject( PbMatrix* pCameraToScreen,PbMatrix* pWorldToCamera,
                          PbFvec* pAngles)
{
  static u64 test[1024*400];
  s32  num_coords   = 0;
  u32  num_sections = 0;
  u16  i            = 0;
  u32* p_object    = NULL;
  void* pChain        = NULL;
  void* p_store_chain = pChain = (void*)&test;

  PbDistortSetTexture( gp_Envmap );

  p_object = (u32*)&OddmentsObject;

  /////////////////////////////////////////////////////////////////////////////////////
  // Setup double buffering

  *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
  *((u32*)pChain)++ = VIF_CODE( VIF_FLUSHA, 0, 0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_FLUSH, 0, 0 );

  *((u32*)pChain)++ = VIF_CODE( VIF_STMOD,0,0 );  // normalmode
  *((u32*)pChain)++ = VIF_CODE( VIF_BASE,0,9 );
  *((u32*)pChain)++ = VIF_CODE( VIF_OFFSET,0,512-9 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

  ////////////////////////////////////////////////////////////////////////////////////
  // Add CameraToScreen & WorldToCamera & angles to the list

  *((u64*)pChain)++ = DMA_REF_TAG( (u32)pCameraToScreen, 4 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,4,0 );

  *((u64*)pChain)++ = DMA_REF_TAG( (u32)pWorldToCamera, 4 );
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
      s32 current_count = num_coords > 80 ? 80 : num_coords; 
    
      ////////////////////////////////////////////////////////////////////////////////////
      // Add GifTag to list

      *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
      *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
      *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,1,VIF_UNPACK_DBLBUF | 0 );

      if( set_gif == 1 )
      {
        *((u64*)pChain)++ = GIF_TAG( current_count, 1, 1, GS_SETREG_PRIM( GS_PRIM_PRIM_TRISTRIP,0, 1, 0, 0, 0, 0, 0, 0 ), 0, 3 );

        *((u64*)pChain)++ = 0x512; // registers to set
        set_gif = 0;
      }
      else
      {
        *((u64*)pChain)++ = GIF_TAG( current_count, 1, 0, GS_SETREG_PRIM( GS_PRIM_PRIM_TRISTRIP,0, 1, 0, 0, 0, 0, 0, 0 ), 0, 3 );

        *((u64*)pChain)++ = 0x512; // pad
      }

      ////////////////////////////////////////////////////////////////////////////////////
      // Add Coordinates to list

      *((u64*)pChain)++ = DMA_REF_TAG( (u32)p_object, current_count * 2);
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

      p_object += current_count*4*2;
      num_coords -= current_count;
    }
  }

  *((u64*)pChain)++ = DMA_CNT_TAG( 2 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP, 0, 0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_DIRECT, 0, 2 );
  
  *((u64*)pChain)++ = GIF_TAG( 1, 1, 0, 0, 0, 1 );
  *((u64*)pChain)++ = GIF_AD;

  *((u64*)pChain)++ = 0x42;
  *((u64*)pChain)++ = GS_TEXFLUSH;

  *((u64*)pChain)++ = DMA_END_TAG( 0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_FLUSHA, 0, 0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_FLUSH, 0, 0 );

  FlushCache(0);
  PbDmaSend01Chain( p_store_chain );

  return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// void PbDistortUpdate
///////////////////////////////////////////////////////////////////////////////

void PbDistortSetTexture( PbTexture* pTexture )
{
  u64* p_store; 
  u64* p_data; 

  p_store = p_data = PbSprAlloc( 2*16 ); 

  *p_data++ = GIF_TAG( 1, 1, 0, 0, 0, 1 ); 
  *p_data++ = GIF_AD; 

  *p_data++ = PbTextureGetTex0( pTexture ); 
  *p_data++ = GS_TEX0_1; 

  PbDmaSend02Spr( p_store, 2 ); 
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PbDistortSimpleBlur( PbTexture* pDest, PbTexture* pSource, 
                        float u1, float v1, float u2, float v2,int a1, int a2 )
{
  u64* p_data  = NULL;
  u64* p_store = NULL;
  
  u1 += 0.5f;
  v1 += 0.5f;
  u2 += 0.5f;
  v2 += 0.5f;

  u1 *= 16.0f;
  v1 *= 16.0f;
  u2 *= 16.0f;
  v2 *= 16.0f;

  PbTextureSetRenderTarget( pDest );
  PbDmaWait02();

  p_data = p_store = PbSprAlloc( 11*16 );

  *p_data++ = GIF_TAG( 10, 1, 0, 0, 0, 1 );
  *p_data++ = GIF_AD;
  
  *p_data++ = 0x42;
  *p_data++ = GS_TEXFLUSH;

  *p_data++ = PbTextureGetTex0( pSource ) | ((u64)0)<<35;;
  *p_data++ = GS_TEX0_1;

  *p_data++ = GS_SETREG_CLAMP( 2, 2, 0, 512, 0, 256 );
  *p_data++ = GS_CLAMP_1;

  *p_data++ = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0);
  *p_data++ = GS_PRIM;

  // Alpha

  *p_data++ = 0x0000000029+(((u64)a2)<<32);
  *p_data++ = GS_ALPHA_1;

  // Color

  *((unsigned char*)p_data)++ = a1;
  *((unsigned char*)p_data)++ = a1;
  *((unsigned char*)p_data)++ = a1;
  *((unsigned char*)p_data)++ = 0;
  *((float*)p_data)++ = 256.0f;
  *p_data++ = GS_RGBAQ;

  *p_data++ = GS_SETREG_UV( (int)u2, (int)v2 );
  *p_data++ = GS_UV;
  
  *p_data++ = GS_SETREG_XYZ2( ( 0 + 2048 ) << 4, ( 0 + 2048 ) << 4, 0 );
  *p_data++ = GS_XYZ2;

  *p_data++ = GS_SETREG_UV( (int)u1, (int)v1 );
  *p_data++ = GS_UV;

  *p_data++ = GS_SETREG_XYZ2( ( 512 + 2048 ) << 4, ( 256 + 2048 ) << 4, 0 );
  *p_data++ = GS_XYZ2;

  PbDmaSend02Spr( p_store, 11 );
}

///////////////////////////////////////////////////////////////////////////////
// void PbDistortRadialBlur
///////////////////////////////////////////////////////////////////////////////

void PbDistortRadialBlur( PbTexture* pSource,PbTexture* pDest, float cx, float cy, 
                        float v, int n, int mix1, int mix2 )
{
  float scale=v;
  int t;

  // Based on aura for laura code.
  
  for( t = 0; t < n; t++ )
  {
    float x1,y1,x2,y2;
      
    x1=scale*(-cx);
    y1=scale*(-cy);
    x2=scale*(pSource->x-cx);
    y2=scale*(pSource->y-cy);
    PbDistortSimpleBlur( pSource, pDest, cx+x1,cy+y1,cx+x2,cy+y2,mix1,mix2 );

    scale=PbSqrt(scale)*PbSqrt(PbSqrt(scale));
      
    x1=scale*(-cx);
    y1=scale*(-cy);
    x2=scale*(pSource->x-cx);
    y2=scale*(pSource->y-cy);
    PbDistortSimpleBlur( pDest, pSource, cx+x1,cy+y1,cx+x2,cy+y2,mix1,mix2 );
    scale=PbSqrt(scale)*PbSqrt(PbSqrt(scale));
  }
}

