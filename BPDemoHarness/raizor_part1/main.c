/*
 * main.c - quick credits part for bp demo.
 * should probably be at end of demo, just before greets bit.
 *
 * Copyright (c) 2004 raizor <raizor@c0der.net> and jar <dsl123588@vip.cybercity.dk>
 *
 * Licensed under the AFL v2.0. See the file LICENfSE included with this
 * distribution for licensing terms.
 *
 * Makes use of PbDemoLib by emoon
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * FD Tunnel code written by Jar...
 *
 * code is a bit messy, maybe clean after BP :P
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
#include "PbVram.h"
#include "PbPrim.h"
#include "PbMath.h"

#include "math.h"

extern u8 binary_texture_start[];
static u32 texture_test_32[256*256] __attribute__((aligned(16)));
extern u32 c_rzr_tex[]; // raizor tex
extern u32 c_emoon_tex[]; // emoon tex
extern u32 c_adresd_tex[]; //  tex
extern u32 c_tyranid_tex[]; //  tex
extern u32 c_jar_tex[]; //  tex
extern u32 c_gfxcoding_tex[]; //  tex
extern u32 credits_tex[]; //  tex

PbTexture* texture;
PbTexture* rendertarget;
PbTexture* rendertarget2;
PbTexture* tex_emoon;
PbTexture* tex_font;
PbTexture* tex_credits;
PbTexture* tex_gfxcoding;
float max_time;

u32 vramTexStart = 0;

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

void setBGColorRGB(unsigned char red, unsigned char green, unsigned char blue)
{
	*(unsigned int*)0x120000E0 = (red<< 0) | (green<< 8) | (blue<<16);
}

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
    const int shadow_points = 3;
    const int shadow_twist = 3;

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
	//*z = log((0.8+0.25*(PbCos(8*vinkel+*u*2)))*2.718282*((t > 63 ? 63 : t)/63.0));	
    *z = log((0.8+0.25*(PbCos(shadow_points*vinkel+*u*shadow_twist)))*2.718282*((t > 63 ? 63 : t)/63.0));	
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
	
    float originCalc = 0.0f;
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
		
#if 0
			fd_tunnel(Origin,Direction,&a_u,&a_v,&a_z);
			fd_planes(Origin,Direction,&b_u,&b_v,&b_z);
			
			g->st.uv.u = lerp(a_u, b_u, blend);
			g->st.uv.v = lerp(a_v, b_v, blend);
			g->z = lerp(a_z, b_z, blend);
#else
			fd_tunnel(Origin,Direction,&g->st.uv.u,&g->st.uv.v,&g->z);
#endif			
		}
	}	
    /*
    if (originCalc < 1000.0f)
    {
        originCalc+=10.0f;
    }else{
        originCalc=0.0f;
    }
*/
}

void calc_coords2(float t, float progress)
{
	int x,y;
	gridcoord* g;
	
	float whirl_x = 0.5f + 0.3f * PbCos(t*1.4f);
	float whirl_y = 0.5f + 0.2f * PbSin(t*-0.37564f + 3);
	float radius = 0.4f;
	float ratio = 1.0f/radius;
	
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
				float cx,cy,dist,angle,ca,sa;
	
				cx = g->x-whirl_x;
				cy = g->y-whirl_y;
				dist = PbSqrt(cx*cx+cy*cy);
				dist = (dist>radius)? 0 : ((radius-dist)*(radius-dist))/radius;
				angle = M_PI*2*dist;    
				ca = PbCos(angle);   
				sa = PbSin(angle);
				g->st.uv.u = whirl_x + (cx*ca+cy*sa);
				g->st.uv.v = whirl_y + (-cx*sa+cy*ca);
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
	
	// create and upload the fd-texture
	
	for(cnt=0; cnt<65536; ++cnt)
	{
		u32 r = binary_texture_start[cnt*3+0]>>1;
		u32 g = binary_texture_start[cnt*3+1]>>1;
		u32 b = binary_texture_start[cnt*3+2]>>1;
		texture_test_32[cnt] = r | (g<<8) | (b<<16) | (0x80<<24UL);
	}
	FlushCache(0);
	texture = PbTextureCreate32(texture_test_32, 256, 256); 
    tex_credits = PbTextureCreate32( credits_tex, 512, 128 ); 
    tex_gfxcoding = PbTextureCreate32( c_gfxcoding_tex, 512, 32 ); 
    tex_emoon = PbTextureCreate32( c_emoon_tex, 512, 64 );  
    
    PbTextureUpload(texture);		
    PbTextureUpload(tex_credits);
    PbTextureUpload(tex_gfxcoding);
    PbTextureUpload(tex_emoon);    
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


#define BLUR_RATIO 4
#define MAX_INTENS 48
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
    PbPrimSetState( PB_ALPHA_BLENDING, PB_ENABLE );
	for(i=0;i<BLUR_RATIO;++i)
		draw_fullscreen_texture(rendertarget,i*blah2+cx,i*blah2+cy,crapzilla);
	for(i=0;i<BLUR_RATIO;++i)
		draw_fullscreen_texture(rendertarget2,i*blah,i*blah,MAX_INTENS-crapzilla);
    PbPrimSetState( PB_ALPHA_BLENDING, PB_DISABLE );
	PbGifListSend();	
   	
}

u32 MakeCol(u8 r, u8 g, u8 b, u8 a)
{
    return (r<< 0) | (g<< 8) | (b<<16) | (a<<24);
}

/*

Viva la spagetti code...

*/

#define TIME_PER_LOGO 100
int logotimer = 0;
u8 logotoggle = 0;
u8 logocounter = 0;
float logo_trans_pc = 0.0f;
// ...
#define LOGO_MAXCOUNT 4
#define LOGO_EMOON    0
#define LOGO_ADRESD   1
#define LOGO_JAR      2
#define LOGO_TYRANID  3
#define LOGO_RAIZOR   4
#define LOGO_DUKE     5
#define LOGO_HIRYU    6

u8 fadingout = 0;

// draws the stretchy name logos

void DrawNameLogo()
{ 
    
    PbPrimQuadTextureGouraud( tex_emoon, 
                        (100-(int)((1.0f-logo_trans_pc)*800))<<4,  155<<4,   0<<4,   0<<4, 
                        (642+(int)((1.0f-logo_trans_pc)*800))<<4,  155<<4, 512<<4,   0<<4, 
                        (100-(int)((1.0f-logo_trans_pc)*800))<<4,  239<<4,   0<<4, 64<<4, 
                        (642+(int)((1.0f-logo_trans_pc)*800))<<4,  239<<4, 512<<4, 64<<4, 10,
                        MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)),
                        MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)),
                        MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)),
                        MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)));

    if (logocounter != LOGO_RAIZOR)
    {
        PbPrimQuadTextureGouraud( tex_gfxcoding, 
                            (300+(int)((1.0f-logo_trans_pc)*400))<<4,  217<<4,   127<<4,   0<<4, 
                            (453-(int)((1.0f-logo_trans_pc)*400))<<4,  217<<4, 280<<4,   0<<4, 
                            (300+(int)((1.0f-logo_trans_pc)*400))<<4,  249<<4,   127<<4, 32<<4, 
                            (453-(int)((1.0f-logo_trans_pc)*400))<<4,  249<<4, 280<<4, 32<<4, 10,
                            MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)),
                            MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)),
                            MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)),
                            MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)));
    }else{
        PbPrimQuadTextureGouraud( tex_gfxcoding, 
                            (220+(int)((1.0f-logo_trans_pc)*400))<<4,  217<<4,   0<<4,   0<<4, 
                            (500-(int)((1.0f-logo_trans_pc)*400))<<4,  217<<4, 280<<4,   0<<4, 
                            (220+(int)((1.0f-logo_trans_pc)*400))<<4,  249<<4,   0<<4, 32<<4, 
                            (500-(int)((1.0f-logo_trans_pc)*400))<<4,  249<<4, 280<<4, 32<<4, 10,
                            MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)),
                            MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)),
                            MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)),
                            MakeCol(0x80,0x80,0x80,(u8)(0x80*logo_trans_pc)));
    }

    if (!logotoggle && logotimer==0)
    {
        if (logo_trans_pc<1.0f)
        {
            logo_trans_pc+=0.04f;
        }else{
            logotoggle = 1;   
        }
    }else{
        if (logotoggle && logotimer>TIME_PER_LOGO)
        {
            if (logo_trans_pc>0.0f)
            {
                logo_trans_pc-=0.04f;
            }else{
                logotoggle = 0;
                logotimer=0;
                logocounter++;
                switch (logocounter)
                {
                    case(LOGO_ADRESD):
                    {
                        tex_emoon->pMem = c_adresd_tex;
                        PbTextureUpload(tex_emoon);
                        break;                   
                    }

                    case(LOGO_JAR):
                    {
                        tex_emoon->pMem = c_jar_tex;
                        PbTextureUpload(tex_emoon);                    
                        break;
                    }

                    case(LOGO_TYRANID):
                    {
                        tex_emoon->pMem = c_tyranid_tex;
                        PbTextureUpload(tex_emoon);                    
                        break;
                    }

                    case(LOGO_RAIZOR):
                    {
                        tex_emoon->pMem = c_rzr_tex;
                        PbTextureUpload(tex_emoon);                    
                        break;
                    }

                    default:
                    {
                        fadingout = 1;               
                        break;
                    }
                }   
                PbDmaWait02();             
            }   
        }
    }

    if (logotoggle)
    {
        logotimer++;
    }
}

//#define TEST

u32 start_demo( const demo_init_t* pInfo )
{
    u8 fadingin = 1;    
    u8 fadealpha = 0x80;
    float barsize = 0.0f;

	demo_init();
    PbPrimSetAlpha( 0, 1, 0, 1, 0x80 );
	
	max_time = pInfo->time_count;  
	
	while( pInfo->time_count > 0 )
	{
		float t= pInfo->curr_time;
		
		#ifdef TEST
		GS_SET_BGCOLOR(0xff, 0x00, 0x00);
		#endif

		// Draw to render target    
	    
	    PbTextureSetRenderTarget(rendertarget);	    
	    PbPrimSpriteNoZtest( 0, 0, RENDERTARGET_W<<4, RENDERTARGET_H<<4, 0, 0x00 );
	    calc_coords(t, pInfo->curr_time / max_time);
	    draw_grid(texture,RENDERTARGET_W,RENDERTARGET_H);
	    
	    #ifdef TEST
	    GS_SET_BGCOLOR(0x00, 0xff, 0x00);
	    #endif

	    // Distort into next rendertarget 
	    
	    PbTextureSetRenderTarget(rendertarget2);	    
	    PbPrimSpriteNoZtest( 0, 0, RENDERTARGET_W<<4, RENDERTARGET_H<<4, 0, 0x000000 );
	    calc_coords2(t, pInfo->curr_time / max_time);
	    draw_grid(rendertarget,RENDERTARGET_W, RENDERTARGET_H);
	    
	    #ifdef TEST
	    GS_SET_BGCOLOR(0x00, 0x00, 0xff);
	    #endif
	    
	    // Now combine	    
	    PbScreenSetCurrentActive();
   		blury_thingie_with_bad_function_name(t);
   		
   		#ifdef TEST
   		GS_SET_BGCOLOR(0x00, 0x00, 0x00);
   		#endif

        PbPrimSetState( PB_ALPHA_BLENDING, PB_ENABLE );
        // vert dark strip
        PbPrimSprite(35<<4, 0<<4, 149<<4, (int)(480*barsize)<<4, 1, 0x40604429);
        // edge highlights
        PbPrimSprite(35<<4, 0<<4, 36<<4, (int)(480*barsize)<<4, 1, 0x40604429);
        PbPrimSprite(148<<4, 0<<4, 149<<4, (int)(480*barsize)<<4, 1, 0x40604429);

        // horiz dark strip
        PbPrimSprite(0<<4, 180<<4, (int)(640*barsize)<<4, 250<<4, 1, 0x40604429);
        // edge highlights
        PbPrimSprite(0<<4, 180<<4, (int)(640*barsize)<<4, 181<<4, 1, 0x20604429);
        PbPrimSprite(0<<4, 250<<4, (int)(640*barsize)<<4, 251<<4, 1, 0x20604429);
       
        // draw credits vert text
        PbPrimQuadTextureGouraud( tex_credits, 
                            148<<4,  0<<4,   0<<4,   0<<4, 
                            148<<4,  512<<4,   512<<4,   0<<4, 
                            20<<4,  0<<4,   0<<4,   128<<4, 
                            20<<4,  512<<4,   512<<4,   128<<4, 4,
                            MakeCol(0x80,0x80,0x80,(int)(0x80*barsize)),
                            MakeCol(0x80,0x80,0x80,(int)(0x80*barsize)),
                            MakeCol(0x80,0x80,0x80,(int)(0x80*barsize)),
                            MakeCol(0x80,0x80,0x80,(int)(0x80*barsize)));

        // fading in/out etc
        if (fadingout)
        {
            if (barsize-0.02f > 0.0f)
            {
                barsize-=0.02f;
            }else{
                barsize= 0.0f;
            }
            if (fadealpha < 0x80)
            {   
                fadealpha++;                
                PbPrimSprite(0<<4, 0<<4, 640<<4, 256<<4, 99, MakeCol(0x00,0x00,0x00,fadealpha));
            }else{
                // EXIT!!
                return pInfo->screen_mode;
            }
        }else{
            if (!fadingin)
            {
                DrawNameLogo();
            }else{
                if (fadealpha > 0x00)
                {
                    fadealpha--;
                    PbPrimSprite(0<<4, 0<<4, 640<<4, 256<<4, 99, MakeCol(0x00,0x00,0x00,fadealpha));                    
                }else{
                    if (barsize < 1.0f)
                    {
                        barsize+=0.01f;
                    }else{
                        fadingin=0;
                    }     
                }
            }       
        }

        PbPrimSetState( PB_ALPHA_BLENDING, PB_DISABLE );   		
        
		PbScreenSyncFlip();
	}

#ifdef TEST	
	//LoadExecPS2("cdrom0:\\PUKKLINK.ELF",0,0);
#endif
  
  	return pInfo->screen_mode;
}
