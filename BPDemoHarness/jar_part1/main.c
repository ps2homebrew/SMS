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
PbTexture* rendertarget2;
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

#define PLANE_OFFSET 200 //650
void fd_planes(vertex Origin, vertex Direction, float *u, float *v, float *z)
{
	vertex Intersect;
	float t;

	t = (sgn(Direction.y)*PLANE_OFFSET-Origin.y) / Direction.y;

	Intersect.x = Origin.x + Direction.x*t;
	Intersect.y = Origin.y + Direction.y*t;
	Intersect.z = Origin.z + Direction.z*t;

	*u = fabs(Intersect.x)*0.0008;
	*v = fabs(Intersect.z)*0.0008;

	t = sqr(Intersect.x-Origin.x) + sqr(Intersect.z-Origin.z);
	if (t <= 0.001f) 
	{
		*z = 0;
	}	
	else
	{
		t = 50000.0 / PbSqrt(t);
		*z = (t > 63 ? 63 : t)/63.0f;
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
	float blend = progress*1.5;//0.5f + 0.5f * PbCos(1.0f + t*0.56f);
	if(blend>1.0f)blend=1.0f;
	
	vertex Origin = { 0,0,-500+t*125 };
	vertex Direction;
	
	float xmov = PbSin(t)*0.5f, ymov = PbSin(t*2.2f)*0.5f;
	
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
			g->z = a_z*(1.5f-blend)+b_z*blend*2-1; /*lerp(a_z, b_z, blend)*/
			if(g->z < 0) g->z=0;
			if(g->z > 1) g->z=1;
#else
			fd_tunnel(Origin,Direction,&g->st.uv.u,&g->st.uv.v,&g->z);
#endif			
		}
	}	
}

void calc_coords2(float t, float progress, float syncie)
{
	int x,y;
	gridcoord* g;
	
	float num_xsines = 4 + 1*PbSin(t);
	float xoff = t*4.6;
	float xamp = syncie*0.4;
	float num_ysines = 5 + 3*PbSin(t*0.7+23);
	float yoff = t*8.2-syncie*2;
	float yamp = syncie*0.3;
	 
	float xinc = num_xsines*(M_PI/NUM_X);
	float xang = xoff*M_PI/180.0;
	float yinc = num_ysines*(M_PI/NUM_Y);
	float yang, xtemp;

	float whirl_x = 0.5f + 0.3f * PbCos(t*1.4f);
	float whirl_y = 0.5f + 0.2f * PbSin(t*-0.37564f + 3);
	float radius = 0.4f;
	float ratio = 1.0f/radius;
	

	for(y=0; y<NUM_Y+1; ++y)
	{
		yang = yoff*M_PI/180.0;
		xtemp = xamp*PbSin(xang);
		for(x=0; x<NUM_X+1; ++x)
		{
			g= &coords[CALC_GRID_POS(x,y)];
			
			if(y <= 1 || y>=NUM_Y-2 || x <= 1 || x>=NUM_X-2)
			{
				g->st.uv.u = g->x;
				g->st.uv.v = g->y;
				g->z = 1.0f;
			}
			else
			{
				float cx,cy,dist,angle,ca,sa;
	
				cx = g->x-whirl_x;
				cy = g->y-whirl_y;
				dist = PbSqrt(cx*cx+cy*cy);
				dist = (dist>radius)? 0 : ((radius-dist)*(radius-dist))/radius;
				angle = M_PI*2*dist;    
				ca = PbCos(angle);   
				sa = PbSin(angle);
				g->st.uv.u = whirl_x + (cx*ca+cy*sa)  + xtemp;
				g->st.uv.v = whirl_y + (-cx*sa+cy*ca)  + yamp*PbSin(yang);
				g->z = 1.0f;
				g->z = 1.0f;
			}
			yang += yinc;
		}
		xang += xinc;
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

	// create render target textures
	
	rendertarget = PbTextureCreate32(NULL, RENDERTARGET_W, RENDERTARGET_H); 
	
	rendertarget2 = PbTextureCreate32(NULL, RENDERTARGET_W, RENDERTARGET_H); 
	
	// create and upload the fd-textuer
	
	for(cnt=0; cnt<65536; ++cnt)
	{
		u32 r = binary_texture_start[cnt*3+0]>>1;
		u32 g = binary_texture_start[cnt*3+1]>>1;
		u32 b = binary_texture_start[cnt*3+2]>>1;
		texture_test_32[cnt] = r | (g<<8) | (b<<16) | (0x80<<24UL);
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
}

#define ALPHA_SRC 0
#define ALPHA_DST 1
#define ALPHA_ZERO 2
#define ALPHA_FIX 2
#define ALPHA(A,B,C,D,FIX) ( (((u64)(A))&3) | ((((u64)(B))&3)<<2) | ((((u64)(C))&3)<<4) | ((((u64)(D))&3)<<6) | ((((u64)(FIX)))<<32UL) )//(A - B)*C >> 7 + D 

#define ALPHA_BLEND_NORMAL (ALPHA(ALPHA_SRC,ALPHA_DST,ALPHA_SRC,ALPHA_DST,0x00))
#define ALPHA_BLEND_ADD_NOALPHA    (ALPHA(ALPHA_SRC,ALPHA_ZERO,ALPHA_FIX,ALPHA_DST,0x80))
#define ALPHA_BLEND_ADD    (ALPHA(ALPHA_SRC,ALPHA_ZERO,ALPHA_SRC,ALPHA_DST,0x00))

void draw_fullscreen_texture(PbTexture *tex, int xoff, int yoff, int c)
{
	int x1 = (0+2048) << 4;
	int y1 = (0+2048) << 4;
	int x2 = (SCR_W+2048) << 4;
	int y2 = (SCR_H+2048) << 4;	

	PbGifListAdd( GS_PRIM, GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, 0, 0 ));	
	PbGifListAdd( GS_TEST_1, GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 ) );	
	PbGifListAdd( GS_PABE, 0 );	
	PbGifListAdd( GS_ALPHA_1, ALPHA_BLEND_ADD_NOALPHA);	
	PbGifListAdd( GS_TEX1_1, 0x20 );
	PbGifListAdd( GS_TEX0_1, PbTextureGetTex0( tex ) );
	PbGifListAdd( GS_RGBAQ, 0x80000000+((c<<16)+(c<<8)+c) );
	PbGifListAdd( GS_UV, GS_SETREG_UV( xoff, yoff ) );	
	PbGifListAdd( GS_XYZ2, GS_SETREG_XYZ2( x1, y1, 0 ) );	
	PbGifListAdd( GS_UV, GS_SETREG_UV( ((tex->x-1) << 4)-xoff, ((tex->y-1) << 4)-yoff ) );
	PbGifListAdd( GS_XYZ2, GS_SETREG_XYZ2( x2, y2, 0 ) );	
	PbGifListAdd( GS_TEST_1, GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 2 ) );
}


#define MIN_BLUR 14
#define MAX_BLUR 20
float BLUR_RATIO = MIN_BLUR;
#define MAX_INTENS ((0x80*4) / BLUR_RATIO)
void blury_thingie_with_bad_function_name(float t)
{
	int i;
	float blah = 12.0f + 6.0f * PbSin(t*0.2+3);
	float blah2 = 12.0f + 6.0f * PbCos(t*-1.4+734);
	float crapzilla = (MAX_INTENS/2) + (MAX_INTENS/2)*PbCos(t*1.5f);
	float cx = 15 + 8 * PbSin(t*3.23 + 542);
	float cy = 15 + 8 * PbCos(t*-2.78);
	PbPrimSpriteNoZtest( 0, 0, SCR_W<<4, SCR_H<<4, 0, 0 );

	PbGifListBegin();
	for(i=0;i<BLUR_RATIO;++i)
		draw_fullscreen_texture(rendertarget,i*blah2+cx,i*blah2+cy,MAX_INTENS-crapzilla);
	for(i=0;i<BLUR_RATIO;++i)
		draw_fullscreen_texture(rendertarget2,i*blah,i*blah,crapzilla);
	PbGifListSend();	
   	
}

#define TEST

u32 start_demo( const demo_init_t* pInfo )
{
	u32 syncs_left = pInfo->no_syncs;
	volatile u32* p_syncs = pInfo->sync_points;
	float sync = 0.0f;
	float last_t = pInfo->curr_time;

	demo_init();
	
	max_time = pInfo->time_count;

	while( pInfo->time_count > 0 )
	{
		float t= pInfo->curr_time;
		float dt = t-last_t;
		float progress = t / max_time;
		last_t= t;
		
		
	    if(syncs_left > 0)
	    {
			if(*p_syncs < pInfo->get_pos())
			{
				sync = 0.15f;
				BLUR_RATIO = MAX_BLUR;
				p_syncs++;
				syncs_left--;
			} 
	    }
	    	
	    BLUR_RATIO = MIN_BLUR + (MAX_BLUR-MIN_BLUR)*(0.5+0.5*PbSin(t*2.2+4));

		sync -= 0.8f * dt;
		if(sync<0) sync = 0;
		
		#ifdef TEST
		GS_SET_BGCOLOR(0xff, 0x00, 0x00);
		#endif
		
		// Draw to render target    
	    
	    PbTextureSetRenderTarget(rendertarget);	    
	    PbPrimSpriteNoZtest( 0, 0, RENDERTARGET_W<<4, RENDERTARGET_H<<4, 0, 0x00 );
	    calc_coords(t, progress);
	    draw_grid(texture,RENDERTARGET_W,RENDERTARGET_H);
	    
	    #ifdef TEST
	    GS_SET_BGCOLOR(0x00, 0xff, 0x00);
	    #endif

	    // Distort into next rendertarget 
	    
	    PbTextureSetRenderTarget(rendertarget2);	    
	    PbPrimSpriteNoZtest( 0, 0, RENDERTARGET_W<<4, RENDERTARGET_H<<4, 0, 0x000000 );
	    calc_coords2(t, pInfo->curr_time / max_time, sync);
	    draw_grid(rendertarget,RENDERTARGET_W, RENDERTARGET_H);
	    
	    #ifdef TEST
	    GS_SET_BGCOLOR(0x00, 0x00, 0xff);
	    #endif
	    
	    // Now combine	    
	    PbScreenSetCurrentActive();
   		blury_thingie_with_bad_function_name(t);
	    //PbPrimSpriteTexture( rendertarget2, 0<<4,  0<<4,   0<<4,   0<<4, SCR_W<<4, SCR_H<<4, RENDERTARGET_W<<4, RENDERTARGET_H<<4, 0, 0xff<<16|0xff<<8|0xff );
   		
   		#ifdef TEST
   		GS_SET_BGCOLOR(0x00, 0x00, 0x00);
   		#endif
   		
		PbScreenSyncFlip();				
	}

#ifdef TEST	
	LoadExecPS2("cdrom0:\\PUKKLINK.ELF",0,0);
#endif
  
  	return pInfo->screen_mode;
}
