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
PbTexture* texture = NULL;
float max_time;

#define NUM_X (SCR_W / 8)
#define NUM_Y (SCR_H / 8)
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
static u64 gif_buffer[(12*NUM_X*NUM_Y+4)*2] __attribute__((aligned(16)));
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
		
			fd_tunnel(Origin,Direction,&a_u,&a_v,&a_z);
			fd_planes(Origin,Direction,&b_u,&b_v,&b_z);
			
			g->st.uv.u = lerp(a_u, b_u, blend);
			g->st.uv.v = lerp(a_v, b_v, blend);
			g->z = lerp(a_z, b_z, blend);
		}
	}	
}

u64 expand_color(int c)
{
	if(c>127) c=127;
	else if(c<0) c=0;
	c = z_table[c];
	return 0x3F80000000000000|(u64)(c<<16|c<<8|c);
}

void draw_quad(gridcoord*a, gridcoord*b, gridcoord*c, gridcoord*d)
{
	int x0,y0,x1,y1,x2,y2,x3,y3;
	
	x0 = FTOI4(a->x * SCR_W + 2048);
	y0 = FTOI4(a->y * SCR_H + 2048);
	x1 = FTOI4(b->x * SCR_W + 2048);
	y1 = FTOI4(b->y * SCR_H + 2048);
	x2 = FTOI4(c->x * SCR_W + 2048);
	y2 = FTOI4(c->y * SCR_H + 2048);
	x3 = FTOI4(d->x * SCR_W + 2048);
	y3 = FTOI4(d->y * SCR_H + 2048);

	{ 
			
		*p_data++ = expand_color(127*a->z); 
		*p_data++ = GS_RGBAQ; 
		
		*p_data++ = a->st._st;
		*p_data++ = GS_ST; 
		
		*p_data++ = GS_SETREG_XYZ3( x0, y0, 0 ); 
		*p_data++ = GS_XYZ3; 
		
		
		
		*p_data++ = expand_color(127*c->z); 
		*p_data++ = GS_RGBAQ; 
		
		*p_data++ = c->st._st; 
		*p_data++ = GS_ST; 
		
		*p_data++ = GS_SETREG_XYZ3( x2, y2, 0 ); 
		*p_data++ = GS_XYZ3; 
		
		
		
		*p_data++ = expand_color(127*b->z); 
		*p_data++ = GS_RGBAQ;  
		
		*p_data++ = b->st._st;
		*p_data++ = GS_ST; 
		
		*p_data++ = GS_SETREG_XYZ2( x1, y1, 0 ); 
		*p_data++ = GS_XYZ2; 
		
		
		
		
		*p_data++ = expand_color(127*d->z); 
		*p_data++ = GS_RGBAQ;  
		
		*p_data++ = d->st._st;
		*p_data++ = GS_ST; 
		
		*p_data++ = GS_SETREG_XYZ2( x3, y3, 0 ); 
		*p_data++ = GS_XYZ2; 
	}
}

void draw_grid()
{
	int x,y;
	
	
	p_data = gif_buffer; 

	*p_data++ = GIF_TAG( 12*NUM_X*NUM_Y+3, 1, 0, 0, 0, 1 ); 
	*p_data++ = GIF_AD; 
	
	*p_data++ = PbTextureGetTex0( texture ); 
	*p_data++ = GS_TEX0_1; 
	
	*p_data++ = 1<<GS_TEX1_MMIN_LINEAR_MIPMAP_LINEAR;
	*p_data++ = GS_TEX1_1; 
	
	*p_data++ = GS_SETREG_PRIM( GS_PRIM_PRIM_TRISTRIP, 1, 1, 0, 0, 0, 0, 0, 0) ; 
	*p_data++ = GS_PRIM; 
			
	for(y=0; y<NUM_Y; ++y)
	{
		for(x=0; x<NUM_X; ++x)
		{
			draw_quad(&coords[CALC_GRID_POS(x,y)],&coords[CALC_GRID_POS(x+1,y)],&coords[CALC_GRID_POS(x,y+1)], &coords[CALC_GRID_POS(x+1,y+1)]);
		}
	}	

	FlushCache(0);
	PbDmaSend02( gif_buffer, 12*NUM_X*NUM_Y+4 );
}

void init_textures()
{
	int cnt;
	for(cnt=0; cnt<65536; ++cnt)
	{
		u32 r = binary_texture_start[cnt*3+0];
		u32 g = binary_texture_start[cnt*3+1];
		u32 b = binary_texture_start[cnt*3+2];
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
		z_table[i] = 127 * ((i*i)/(128.0f*128.0f));
}

u32 start_demo( const demo_init_t* pInfo )
{
	demo_init();
	
	max_time = pInfo->time_count;

	while( pInfo->time_count > 0 )
	{
	    PbScreenClear( 0x000000);
	        
	    calc_coords(pInfo->curr_time, pInfo->curr_time / max_time);
	    
	    draw_grid();

		PbScreenSyncFlip();
	}
	
	LoadExecPS2("cdrom0:\\PUKKLINK.ELF",0,0);
  
  	return pInfo->screen_mode;
}
