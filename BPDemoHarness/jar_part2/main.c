#include <tamtypes.h>
#include <kernel.h>

#include "../harness.h"

#include "PbScreen.h"
#include "PbDma.h"
#include "PbGs.h"
#include "PbScreen.h"
#include "PbMisc.h"
#include "PbTexture.h"
#include "PbSpr.h"
#include "PbMatrix.h"
#include "PbVif.h"
#include "PbVu1.h"
#include "PbPrim.h"
#include "PbMath.h"
#include "PbVec.h"
#include "PbVu0m.h"

#include "Particles/PbParticle.h"

/*  DEFINES AND ENUMERATIONS
 */
 
#define NUM_POINTS 18000
#define RT_W 256
#define RT_H 256

/*  DATA
 */
PbMatrix view_screen_matrix;
PbMatrix camera_matrix;
PbFvec points[NUM_POINTS];
PbFvec direction[NUM_POINTS];
PbTexture* texture;
PbTexture* rendertarget[2];
static u32 texture_test_32[16*16] __attribute__((aligned(16)));
static u8 binary_texture_start[] = {0,0,0,0,0,0,0,0,0,0,1,1,0,1,2,0,2,4,1,2,5,1,3,6,1,4,7,1,3,6,1,4,6,1,2,5,1,2,3,0,1,2,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,2,3,1,3,6,1,4,9,2,6,12,2,7,15,2,9,17,2,9,16,2,8,15,2,6,12,1,4,8,1,3,6,1,1,2,0,1,1,0,0,0,0,1,2,0,2,4,1,4,7,2,6,12,2,10,18,3,13,25,5,16,31,6,18,34,6,18,32,5,17,30,4,13,24,3,10,18,2,6,10,1,4,6,1,2,3,0,1,1,0,2,3,1,3,7,2,7,13,3,12,21,5,17,32,7,23,42,9,28,51,10,31,56,11,31,56,10,29,51,9,24,43,6,18,33,4,12,21,2,7,12,1,4,6,0,1,2,1,2,6,1,6,10,2,12,22,6,19,35,9,27,50,13,35,64,17,43,74,21,48,80,22,49,81,20,46,76,16,39,67,11,29,51,7,20,35,4,12,21,2,6,11,0,2,4,1,4,8,2,9,17,4,17,31,8,27,50,14,38,67,22,50,83,30,60,95,37,68,104,38,69,106,35,65,100,27,56,89,18,43,72,12,29,51,6,18,31,3,9,16,1,2,4,1,6,11,3,12,23,6,21,40,12,34,61,21,48,81,33,63,99,46,78,115,56,89,124,58,91,126,53,86,122,41,73,108,29,56,90,17,39,65,9,24,42,4,12,23,1,2,5,2,7,13,4,14,27,7,26,48,15,39,69,26,56,91,43,75,112,61,94,128,73,105,137,76,108,139,70,102,135,55,87,123,37,67,102,21,46,76,12,29,49,6,15,26,1,3,6,2,7,15,4,15,29,9,28,50,17,43,73,31,61,96,50,82,119,70,102,135,83,113,142,86,115,143,80,110,141,64,95,130,42,73,108,24,51,81,13,31,54,6,17,29,1,2,5,2,7,14,4,15,28,8,27,50,17,42,72,31,61,96,51,82,118,71,102,135,84,113,142,87,116,143,81,111,141,64,96,130,43,73,108,23,50,80,12,30,51,6,16,28,1,2,5,1,6,12,3,13,25,7,24,43,14,37,66,26,54,89,43,75,111,62,94,128,75,106,138,78,108,139,72,102,135,56,87,122,36,65,100,21,45,73,11,28,47,5,14,24,0,2,4,1,5,9,2,10,20,6,20,37,11,32,56,19,45,76,32,61,95,46,77,112,56,88,123,59,91,125,53,84,119,40,70,105,27,54,84,16,36,60,9,21,37,4,11,20,0,1,2,1,3,6,2,7,14,4,14,26,7,23,43,12,34,60,21,46,78,29,58,91,35,65,99,37,67,102,33,62,95,26,51,83,17,39,65,10,25,44,6,15,26,3,8,13,0,1,1,1,2,4,1,4,9,2,9,16,4,15,28,7,23,42,12,31,55,16,39,66,20,43,73,20,43,72,18,41,69,14,34,58,10,25,43,6,17,29,4,9,17,2,5,8,0,0,0,0,1,1,1,2,4,1,5,9,2,9,17,4,13,25,6,18,34,8,23,42,9,26,45,10,26,46,9,24,42,7,20,34,5,14,25,3,9,15,2,6,9,1,2,4,0,0,0,0,0,0,0,1,2,1,2,5,1,4,9,2,7,12,2,9,17,4,12,21,4,13,24,5,14,25,4,12,21,4,10,18,2,7,12,2,5,9,1,2,4,1,1,2};

/*  FUNCTIONS
 */
 
float frandom()
{
	return (float)PbRand() / (float)PB_RAND_MAX;
}

void build_matrices()
{
	PbMatrixViewScreen( &view_screen_matrix, 512.0f,0.5f,0.5f*(float)RT_H/(float)RT_W, PbScreenGetOffsetX()+RT_W/2,PbScreenGetOffsetY()+RT_H/2, 1.0f, 6777215.0f,64.0f, 5536.0f );
	PbMatrixIdentity( &camera_matrix );
	PbMatrixTranslate( &camera_matrix, 0, 0, 850 );	
}

void generate_points()
{
	int i;
	
	for(i=0; i<NUM_POINTS; ++i)
	{
		points[i].x = frandom()*500-250;
		points[i].y = frandom()*500-250;
		points[i].z = frandom()*400-200;
		points[i].w = 1.0f;
		
		direction[i].x = direction[i].y = direction[i].z = direction[i].w = 0.0f;
	}	
}

void move_points(float t, float dt)
{
	int i;
    float a = 140.0f + 80*PbSin(t*1.2f);
    float b = 200.0f + 120*PbCos(t*-0.8+23);
    float c = 40.0f + 10*PbSin(t*1.7f+878);
    PbFvec temp = { dt * 0.06f, 0, 0, 0 };
    
    for(i=0; i<NUM_POINTS; ++i)
    {
    	direction[i].x = a * (points[i].y-points[i].x);
    	direction[i].y = points[i].x * (b - points[i].z) - points[i].y;
    	direction[i].z = points[i].x * points[i].y - c * points[i].z;    	
    }
    
  /*  for(i=0; i<NUM_POINTS; ++i)
    {
    	points[i].x += direction[i].x * temp.x;
    	points[i].y += direction[i].y * temp.x;
    	points[i].z += direction[i].z * temp.x;
    }*/
    
    
 __asm__ ( 
  "move		       $10, %3                      \n"
  "lqc2			   vf13, 0x00($10)              \n"
  "move		       $8, %0                       \n"
  "move		       $9, %1                       \n"
  "move		       $10, %2                      \n"
  "tranform_loop:                               \n"
  "lqc2            vf05,0x00($9)                \n"
  "lqc2            vf06,0x10($9)                \n"
  "lqc2            vf07,0x20($9)                \n"
  "lqc2            vf08,0x30($9)                \n"
  "lqc2            vf01,0x00($8)                \n" 
  "lqc2            vf02,0x10($8)                \n" 
  "lqc2            vf03,0x20($8)                \n" 
  "lqc2            vf04,0x30($8)                \n" 
  "vmulx.xyz       vf05,vf05,vf13x              \n"
  "vmulx.xyz       vf06,vf06,vf13x              \n"
  "vmulx.xyz       vf07,vf07,vf13x              \n"
  "vmulx.xyz       vf08,vf08,vf13x              \n"
  "vadd.xyz        vf01,vf01,vf05               \n"
  "vadd.xyz        vf02,vf02,vf06               \n"
  "vadd.xyz        vf03,vf03,vf07               \n"
  "vadd.xyz        vf04,vf04,vf08               \n"
  "sqc2            vf01,0x00($8)                \n"   
  "sqc2            vf02,0x10($8)                \n"   
  "sqc2            vf03,0x20($8)                \n"   
  "sqc2            vf04,0x30($8)                \n"   
  "addiu 		   $8, 0x40                     \n"
  "addiu 		   $9, 0x40                     \n"
  ".set	push                                    \n"
  ".set	noreorder                               \n"  
  "addiu		   $10, -1                      \n"
  "bnez		       $10, tranform_loop           \n"
  "nop                                          \n"
  "nop                                          \n"
  ".set	pop                                     \n"
  : : "r"(&points[0]), "r"(&direction[0]), "r"(NUM_POINTS/4), "r"(&temp) : "memory" );
}

void load_texture()
{	
	int cnt;
	
	for(cnt=0; cnt<16*16; ++cnt)
	{
		u32 r = binary_texture_start[cnt*3+0]>>3;
		u32 g = binary_texture_start[cnt*3+1]>>3;
		u32 b = binary_texture_start[cnt*3+2]>>3;
		texture_test_32[cnt] = r | (g<<8) | (b<<16) | (0x80<<24UL);
	}
	FlushCache(0);
	texture = PbTextureCreate32(texture_test_32, 16, 16); 
	PbTextureUpload(texture);
	PbDmaWait02();
	
	rendertarget[0] = PbTextureCreate32(NULL, RT_W, RT_H); 

	// clear render target	
	PbTextureSetRenderTarget(rendertarget[0]);		
	PbPrimSpriteNoZtest( 0, 0, RT_W<<4, RT_H<<4, 0, 0x00 );
	PbScreenSetCurrentActive();
	
	rendertarget[1] = PbTextureCreate32(NULL, RT_W, RT_H); 

	// clear render target	
	PbTextureSetRenderTarget(rendertarget[1]);		
	PbPrimSpriteNoZtest( 0, 0, RT_W<<4, RT_H<<4, 0, 0x00 );
	PbScreenSetCurrentActive();	
}

void setup()
{
	PbScreenSetup( SCR_W, SCR_H, SCR_PSM );
	build_matrices();
	generate_points();
	load_texture();
}

void draw_quad_to_rt(int c)
{
	int x1 = (0+2048) << 4;
	int y1 = (0+2048) << 4;
	int x2 = (RT_W+2048) << 4;
	int y2 = (RT_H+2048) << 4;	

	PbGifListBegin();
	PbGifListAdd( GS_PRIM, GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 1, 1, 1, 0, 0 ));	
	PbGifListAdd( GS_TEST_1, GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 ) );	
	PbGifListAdd( GS_PABE, 0 );	
	PbGifListAdd( GS_ALPHA_1, GS_SETREG_ALPHA(0,1,0,1,0x00));	
	PbGifListAdd( GS_RGBAQ, c<<24 );
	PbGifListAdd( GS_XYZ2, GS_SETREG_XYZ2( x1, y1, 0 ) );	
	PbGifListAdd( GS_XYZ2, GS_SETREG_XYZ2( x2, y2, 0 ) );	
	PbGifListSend();
}



void draw_fullscreen_texture(PbTexture *tex, int xoff, int yoff, int c)
{
	int x1 = (0+2048) << 4;
	int y1 = (0+2048) << 4;
	int x2 = (RT_W+2048) << 4;
	int y2 = (RT_H+2048) << 4;
	
	PbGifListAdd( GS_PRIM, GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, 0, 0 ));
	PbGifListAdd( GS_TEST_1, GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 ) );
	PbGifListAdd( GS_PABE, 0 );
	PbGifListAdd( GS_ALPHA_1, GS_SETREG_ALPHA(0,2,2,1,0x80));
	PbGifListAdd( GS_TEX1_1, 0x20 );
	PbGifListAdd( GS_TEX0_1, PbTextureGetTex0( tex ) );
	PbGifListAdd( GS_RGBAQ, 0x80000000+((c<<16)+(c<<8)+c) );
	PbGifListAdd( GS_UV, GS_SETREG_UV( xoff, yoff ) );
	PbGifListAdd( GS_XYZ2, GS_SETREG_XYZ2( x1, y1, 0 ) );
	PbGifListAdd( GS_UV, GS_SETREG_UV( ((tex->x-1) << 4)-xoff, ((tex->y-1) << 4)-yoff ) );
	PbGifListAdd( GS_XYZ2, GS_SETREG_XYZ2( x2, y2, 0 ) );
	PbGifListAdd( GS_TEST_1, GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 2 ) );
}


int cur_rt = 0;

#define BLUR_RATIO 27
#define MAX_INTENS 14
void blury_thingie_with_bad_function_name(float t)
{
	int i;
	float blah = 36.0f + 36.0f * PbSin(t*3.2+3);
	float blah2 = 36.0f + 36.0f * PbCos(t*-1.4+734);
	float crapzilla = (MAX_INTENS/2) + (MAX_INTENS/4)*PbCos(t*1.5f);
	float cx = 30 + 15 * PbSin(t*3.23 + 542);
	float cy = 30 + 15 * PbCos(t*-2.78);
	//PbPrimSpriteNoZtest( 0, 0, SCR_W<<4, SCR_H<<4, 0, 0 );
	
	PbGifListBegin();
	for(i=0;i<BLUR_RATIO;++i)
		draw_fullscreen_texture(rendertarget[cur_rt],i*blah2+cx,i*blah2+cy,crapzilla);
	PbGifListSend();
   
}
u32 start_demo(const demo_init_t *pInfo)
{
	float last_t;
	int frame = 0;
	setup();
	
	last_t = pInfo->curr_time;
	while(pInfo->frame_count > 0)
	{
		float t= pInfo->curr_time;
		float dt = t-last_t;
		last_t= t;
		
//		GS_SET_BGCOLOR(0xff, 0x00, 0x00);
					
		PbTextureSetRenderTarget(rendertarget[cur_rt]);		
		draw_quad_to_rt(0x60);
		{
			PbMatrix rotate_matrix, rotate_x_matrix, rotate_y_matrix, temprot_matrix, final_matrix;

			move_points(t, dt);
			
		    PbMatrixRotateX( &rotate_x_matrix, t*-0.2f+2 );
		    PbMatrixRotateY( &rotate_y_matrix, t*1.5f );
		    PbMatrixMultiply( &rotate_matrix, &rotate_x_matrix, &rotate_y_matrix );
		    PbMatrixMultiply( &temprot_matrix, &rotate_matrix, &camera_matrix );
		    PbMatrixMultiply( &final_matrix, &temprot_matrix, &view_screen_matrix );
		    
		    PbParticleSetup(&final_matrix, texture, 12.0f *16.0f, 1.0f, 0x12121212);
			PbParticleDraw(points, NUM_POINTS);
		}
		
		PbTextureSetRenderTarget(rendertarget[cur_rt^1]);		
		draw_quad_to_rt(0x60);
		blury_thingie_with_bad_function_name(t);
		
		PbScreenSetCurrentActive();
		PbPrimSpriteNoZtest( 0, 0, SCR_W<<4, SCR_H<<4, 0, 0 );
		PbPrimSpriteTexture(rendertarget[cur_rt^1], 0, 0, 0, 0, SCR_W<<4, SCR_H<<4, (RT_W-1)<<4, (RT_H-1)<<4, 0, 0x80808080);
		
		cur_rt^=1;
		
//		GS_SET_BGCOLOR(0x00, 0x00, 0x00);
		
		PbScreenSyncFlip();
	}
	
//	LoadExecPS2("cdrom0:\\PUKKLINK.ELF",0,0);

	return pInfo->screen_mode;
}
