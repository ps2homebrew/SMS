/*
 * main.c - initial attempt at a demo part.
 *
 * Copyright (c) 2004   jar <dsl123588@vip.cybercity.dk>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 * Makes use of PbDemoLib by emoon
 * Copyright (c) 2004   emoon <daniel@collin.com>
 */

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

#include "math.h"

extern u8 binary_texture_start[];
static u32 texture_test_32[256*256] __attribute__((aligned(16)));
PbTexture* texture;
PbTexture* rendertarget;
float max_time;


#define RENDERTARGET_W 256
#define RENDERTARGET_H 256

#define NUM_X (RENDERTARGET_W / 8)
#define NUM_Y (RENDERTARGET_H / 8)
#define NUM_GRID_POINTS ((NUM_X+1)*(NUM_Y+1))
#define CALC_GRID_POS(x,y) ((x) + (y) * (NUM_X+1))

#define FTOI4(x) ((int)((x)*16.0f))

typedef union 
{
	struct {
		float u, v;
	} uv;
	u64 _st;
} uvcoord;

typedef struct 
{
	uvcoord st;
	float x, y, z;
} gridcoord;

typedef struct{
	float x, y, z, w;
} vertex __attribute__((aligned(16)));

gridcoord coords[NUM_GRID_POINTS] __attribute__((aligned(16)));
static u64 gif_buffer[(6*NUM_X*NUM_Y+4)*2] __attribute__((aligned(16)));
static u64* p_data; 
static int z_table[128];

void init_grid()
{
	int x,y;
	gridcoord* g;
	for(y=0; y<NUM_Y+1; ++y)
	{
		for(x=0; x<NUM_X+1; ++x)
		{
			g= &coords[CALC_GRID_POS(x,y)];
			
			g->x = (float)x / (float)NUM_X;
			g->y = (float)y / (float)NUM_Y;
		}
	}	
}

#define sqr(x) ((x)*(x))
#define fabs(x) ((x)<0 ? -(x) : (x))
#define sgn(x) ((x)>0 ? 1 : (x)<0 ? -1 : 0)

static void fd_tunnel(vertex Origin, vertex Direction, float* u, float* v, float* z)
{
	vertex Intersect;
	float a, b, c, delta, t1, t2, t;
	float vinkel;
	
	a = sqr(Direction.x) + sqr(Direction.y);
	b = 2*(Origin.x*Direction.x + Origin.y*Direction.y);
	c = sqr(Origin.x) + sqr(Origin.y) - sqr(255);
	
	delta = PbSqrt(b*b - 4*a*c);
	t1 = (-b + delta) / (2*a+0.00001);
	t2 = (-b - delta) / (2*a+0.00001);
	t = t1 > 0 ? t1 : t2;	
	Intersect.x = Origin.x + Direction.x*t;
	Intersect.y = Origin.y + Direction.y*t;
	Intersect.z = Origin.z + Direction.z*t;
	vinkel = Intersect.y == 0 ? 0 : atan2(Intersect.y, Intersect.x);
	*u = (fabs(Intersect.z)*0.6)/256.0;
	*v = (fabs(vinkel/M_PI));
	t = 20000.0/t;
	*z = log((0.8+0.25*(PbCos(8*vinkel+*u*2)))*2.718282*((t > 63 ? 63 : t)/63.0));	
	if(*z < 0.0f) *z = 0.0f;
}

#define PLANE_OFFSET 650
void fd_planes(vertex Origin, vertex Direction, float *u, float *v, float *z)
{
	vertex Intersect;
	float t;

	t = (sgn(Direction.y)*PLANE_OFFSET-Origin.y) / Direction.y;

	Intersect.x = Origin.x + Direction.x*t;
	Intersect.y = Origin.y + Direction.y*t;
	Intersect.z = Origin.z + Direction.z*t;

	*u = fabs(Intersect.x)*0.001171875f;
	*v = fabs(Intersect.z)*0.001171875f;

	t = sqr(Intersect.x-Origin.x) + sqr(Intersect.z-Origin.z);
	if (t <= 0.001f) 
	{
		*z = 0;
	}	
	else
	{
		t = 50000.0 / PbSqrt(t);
		*z = log(1.2f * (t > 63 ? 63 : t)/63.0f) * 2.7182818284590452353602874713527f;
		if(*z > 1) *z = 1;
	}
}

void Rotate(vertex* v, float ang)
{
	float s = PbSin(ang);
	float c = PbCos(ang);
	float x=v->x,y=v->y;
	
	v->x = x*c + y*s;
	v->y = -x*s + y*c;
}

#define lerp(a,b,t) ((a) + ((b)-(a))*(t))


void calc_coords(float t, float progress)
{
	int x,y;
	gridcoord* g;
	
	float a_u, a_v, a_z;
	float b_u, b_v, b_z;
	float blend = progress;//0.5f + 0.5f * PbCos(1.0f + t*0.56f);
	
	vertex Origin = { 0,0,-1+t*100 };
	vertex Direction;
	
	float xmov = PbSin(t), ymov = PbSin(t*2.2f);
	
	for(y=0; y<NUM_Y+1; ++y)
	{
		for(x=0; x<NUM_X+1; ++x)
		{
			g= &coords[CALC_GRID_POS(x,y)];
			
			Direction.x = g->x*2-1 + xmov;
			Direction.y = g->y*2-1 + ymov;
			Direction.z = 1.0f;
			
			Rotate(&Direction, t*1.2f);
		
#if 1
			fd_tunnel(Origin,Direction,&a_u,&a_v,&a_z);
			fd_planes(Origin,Direction,&b_u,&b_v,&b_z);
			
			g->st.uv.u = lerp(a_u, b_u, blend);
			g->st.uv.v = lerp(a_v, b_v, blend);
			g->z = lerp(a_z, b_z, blend);
#else
			fd_planes(Origin,Direction,&g->st.uv.u,&g->st.uv.v,&g->z);
#endif			
		}
	}	
}

void calc_coords2(float t, float progress)
{
	int x,y;
	gridcoord* g;
	
	float cx = 0.5f + 0.3f * PbCos(t*1.4f);
	float cy = 0.5f + 0.2f * PbSin(t*-0.37564f + 3);
	float xit = 0.03f * PbCos(t*1.4f);
	float yit = 0.03f * PbSin(-t*0.8f+4);
	
	for(y=0; y<NUM_Y+1; ++y)
	{
		for(x=0; x<NUM_X+1; ++x)
		{
			g= &coords[CALC_GRID_POS(x,y)];
			
			if(y <= 1 || y>=NUM_Y-1 || x <= 1 || x>=NUM_X-1)
			{
				g->st.uv.u = g->x;
				g->st.uv.v = g->y;
				g->z = 1.0f;
			}
			else
			{
				float a = g->x - cx;
				float b = g->y - cy;
				float d= PbSqrt(a*a+b*b);
				g->st.uv.u = g->x+xit*PbSin(g->y * 10.2f + t * 0.5f + 15*d);
				g->st.uv.v = g->y+yit*PbSin(g->x * 24.8f + t * -1.7f + 3 -3.783f*d);
				g->z = 1.0f;
			}
		}
	}	
}

void wait_gif()
{
	PbDmaWait02();
	while( *((volatile unsigned int *)(0x10003020)) & 1 << 10 )  // GIF_STAT
		;	    
}

u64 expand_color(int c)
{
	if(c>127) c=127;
	else if(c<0) c=0;
	c = z_table[c];
	return 0x3F80000000000000|(u64)(c<<16|c<<8|c);
}

void add_gridcoord(gridcoord* a, int coord_reg, int w, int h)
{
	*p_data++ = expand_color(127*a->z); 
	*p_data++ = GS_RGBAQ; 
		
	*p_data++ = a->st._st;
	*p_data++ = GS_ST; 
	
	*p_data++ = GS_SETREG_XYZ3( FTOI4(a->x * w + 2048), FTOI4(a->y * h + 2048), 0 ); 
	*p_data++ = coord_reg;  
}

void draw_grid(PbTexture* texture, int w, int h)
{
	int x,y;
	
	
	p_data = gif_buffer; 

	*p_data++ = GIF_TAG( 6*NUM_X*NUM_Y+3, 1, 0, 0, 0, 1 ); 
	*p_data++ = GIF_AD; 
	
	*p_data++ = PbTextureGetTex0( texture ); 
	*p_data++ = GS_TEX0_1; 
	
	*p_data++ = 1<<GS_TEX1_MMIN_LINEAR_MIPMAP_LINEAR;
	*p_data++ = GS_TEX1_1; 
	
	*p_data++ = GS_SETREG_PRIM( GS_PRIM_PRIM_TRISTRIP, 1, 1, 0, 0, 0, 0, 0, 0) ; 
	*p_data++ = GS_PRIM; 
			
	for(y=0; y<NUM_Y; ++y)
	{
		add_gridcoord(&coords[CALC_GRID_POS(0,y)], GS_XYZ3, w, h);
		add_gridcoord(&coords[CALC_GRID_POS(0,y+1)], GS_XYZ3, w, h);
		for(x=1; x<NUM_X; ++x)
		{
			add_gridcoord(&coords[CALC_GRID_POS(x,y)], GS_XYZ2, w, h);
			add_gridcoord(&coords[CALC_GRID_POS(x,y+1)], GS_XYZ2, w, h);
		}
	}	

	FlushCache(0);
	PbDmaWait02();
	PbDmaSend02( gif_buffer, 6*NUM_X*NUM_Y+4 );
}

void init_textures()
{
	int cnt;

	// create render target texture
	
	rendertarget = PbTextureCreate32(NULL, RENDERTARGET_W, RENDERTARGET_H); 
	
	// create and upload the fd-textuer
	
	for(cnt=0; cnt<65536; ++cnt)
	{
		u32 r = binary_texture_start[cnt*3+0] * 2;
		u32 g = binary_texture_start[cnt*3+1] * 2;
		u32 b = binary_texture_start[cnt*3+2] * 2;
		if(r>255) r=255;
		if(g>255) g=255;
		if(b>255) b=255;
		texture_test_32[cnt] = r | (g<<8) | (b<<16) | 0xFF000000;
	}
	FlushCache(0);
	texture = PbTextureCreate32(texture_test_32, 256, 256); 
	PbTextureUpload(texture);
	PbDmaWait02();
}

void demo_init()
{
	int i;
	PbScreenSetup( SCR_W, SCR_H, SCR_PSM );
	
	init_textures();
	init_grid();
	for(i=0;i<128;++i)
		z_table[i] = 127 * log((i/128.0f)*2.7182818284590452353602874713527f);
}
#define ALPHA_SRC 0
#define ALPHA_DST 1
#define ALPHA_ZERO 2
#define ALPHA_FIX 2
#define ALPHA(A,B,C,D,FIX) ( (((u64)(A))&3) | ((((u64)(B))&3)<<2) | ((((u64)(C))&3)<<4) | ((((u64)(D))&3)<<6) | ((((u64)(FIX)))<<32UL) )//(A - B)*C >> 7 + D 

#define ALPHA_BLEND_NORMAL (ALPHA(ALPHA_SRC,ALPHA_DST,ALPHA_SRC,ALPHA_DST,0x00))
#define ALPHA_BLEND_ADD    (ALPHA(ALPHA_SRC,ALPHA_ZERO,ALPHA_SRC,ALPHA_DST,0x00))
#define ALPHA_BLEND_ADD_NOALPHA    (ALPHA(ALPHA_SRC,ALPHA_ZERO,ALPHA_FIX,ALPHA_DST,0x80))
/*
void blit_rendertarget(PbTexture* tex, int xoff, int yoff, int xzoom, int yzoom, int intensity)
{		
	  u64* p_store;
	  u64* p_data;
	
	  int x1 = xzoom   + ((0 + 2048) << 4);
	  int y1 = yzoom   + ((0 + 2048) << 4);
	  int x2 = -xzoom  + ((SCR_W + 2048) << 4);
	  int y2 = -yzoom  + ((SCR_H + 2048) << 4);
	  int u1 = (0 + 2048) << 4;
	  int v1 = (0 + 2048) << 4;
	  int u2 = (RENDERTARGET_W - 1) << 4;
	  int v2 = (RENDERTARGET_H - 1) << 4;
	  
	  x1 += xoff;
	  y1 += yoff;
	  x2 += xoff;
	  y2 += yoff;
	
	  p_store = p_data = PbSprAlloc( 10*16 );
	
	  *p_data++ = GIF_TAG( 9, 1, 0, 0, 0, 1 );
	  *p_data++ = GIF_AD;
	
	  *p_data++ = PbTextureGetTex0( tex );
	  *p_data++ = GS_TEX0_1;
	
	  *p_data++ = 0x20; // bilinear filtering
	  *p_data++ = GS_TEX1_1;
	  
	  *p_data++ = ALPHA(ALPHA_SRC,ALPHA_ZERO,ALPHA_FIX,ALPHA_DST,intensity);
	  *p_data++ = GS_ALPHA_1;
	
	  *p_data++ = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, TRUE, 0, 1, 0, 0) ;
	  *p_data++ = GS_PRIM;

	  *p_data++ = 0x80808080;
	  *p_data++ = GS_RGBAQ;
	
	  *p_data++ = GS_SETREG_XYZ2( x1, y1, 0 );
	  *p_data++ = GS_XYZ2;
	
	  *p_data++ = GS_SETREG_UV( u1, v1 );
	  *p_data++ = GS_UV;
	
	  *p_data++ = GS_SETREG_XYZ2( x2, y2, 0 );
	  *p_data++ = GS_XYZ2;
	
	  *p_data++ = GS_SETREG_UV( u2, v2 );
	  *p_data++ = GS_UV;
	  
	  PbDmaWait02();
	  PbDmaSend02Spr( p_store, 10 );
}

#define BLURSIZE 4
void funk(float t)
{
	int x,y;
	
	float focus = 40*PbCos(t*3);
	float zoom = 80 * PbSin(t*1.2 + 4);
	for(y=-BLURSIZE;y<BLURSIZE;++y)
	{
		for(x=-BLURSIZE;x<BLURSIZE;++x)
		{
			blit_rendertarget(rendertarget,x*focus,y*focus,zoom,zoom*256.0f/640.0f,0x80 / (BLURSIZE*BLURSIZE));
		}
	}	
		
}*/

/*void blackrect()
{		
	u64* p_store;
	u64* p_data;
	
	int x1 = ((0 + 2048) << 4);
	int y1 = ((0 + 2048) << 4);
	int x2 = ((SCR_W + 2048) << 4);
	int y2 = ((SCR_H + 2048) << 4);
	
	p_store = p_data = PbSprAlloc( 8*16 );
	
	
	*p_data++ = GIF_TAG( 7, 1, 0, 0, 0, 1 );
	*p_data++ = GIF_AD;
	
	*p_data++ = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 1, 0, 0, 0, 0) ;
	*p_data++ = GS_PRIM;
	
	*p_data++ = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 );     
	*p_data++ = GS_TEST_1;   
	
	*p_data++ = ALPHA_BLEND_NORMAL;     
	*p_data++ = GS_ALPHA_1;   	
	
	*p_data++ = 0x20000000;
	*p_data++ = GS_RGBAQ;
	
	*p_data++ = GS_SETREG_XYZ2( x1, y1, 0 );
	*p_data++ = GS_XYZ2;
	
	*p_data++ = GS_SETREG_XYZ2( x2, y2, 0 );
	*p_data++ = GS_XYZ2;
	
	*p_data++ = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 2 );     
	*p_data++ = GS_TEST_1;     
	
	PbDmaSend02Spr( p_store, 8 );
}*/


u32 start_demo( const demo_init_t* pInfo )
{
	demo_init();
	
	max_time = pInfo->time_count;
	
	while( pInfo->time_count > 0 )
	{
		float t= pInfo->curr_time;
		// Draw to render target    
	    PbTextureSetRenderTarget(rendertarget);
	    PbPrimSpriteNoZtest( 0, 0, RENDERTARGET_W<<4, RENDERTARGET_H<<4, 0, 0x00 );
	    calc_coords(t, pInfo->curr_time / max_time);
	    draw_grid(texture,RENDERTARGET_W,RENDERTARGET_H);

	    // Blit the render target to the frame buffer
	    PbScreenSetCurrentActive();
	    PbPrimSpriteNoZtest( 0, 0, SCR_W<<4, SCR_H<<4, 0, 0x000000 );
	    calc_coords2(t, pInfo->curr_time / max_time);
	    draw_grid(rendertarget,SCR_W,SCR_H);

		PbScreenSyncFlip();
	}
	
//	LoadExecPS2("cdrom0:\\PUKKLINK.ELF",0,0);
  
  	return pInfo->screen_mode;
}
