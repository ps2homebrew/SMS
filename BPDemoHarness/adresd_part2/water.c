#include <tamtypes.h>
#include <kernel.h>
#include "../harness.h"
#include "PbScreen.h"
#include "PbTexture.h"
#include "PbDma.h"
#include "PbGs.h"
#include "PbSpr.h"
#include "PbPrim.h"
#include "PbMath.h"
#include "PbVu0m.h"
#include "PbTexture.h"

#include "water.h"

PbTexture *tex_envmap;
PbTexture *tex_image;
PbTexture *tex_sea;
PbTexture *tex_mfjg;

PbTexture *background_tex;
PbTexture *reflectionmap_tex;

#ifndef M_PI
#define M_PI 3.145
#endif

/*
  structures used to house ripple calculations
  This is used to store the x and y components (and radius) of a ray from the
  center.  These are scaled by the ripple magnitude at that point to find
  how much to displace the original texture coordinates.
*/
typedef struct
{
  float dx;
  float dy;
  float r;
}RIPPLE_VECTOR;


// x=t[0],y=t[1],z=n[0],w=n[1]
static PbFvec ripple_texcoords[GRID_SIZE_X][GRID_SIZE_Y];

/* lots of look up tables.  This one houses the amplitude of the ripple
   as a function of time. */
float ripple_amplitude[RIPPLE_LENGTH];

/* the grid housing diff's and radius information for each point on the grid */
RIPPLE_VECTOR ripple_precalc[GRID_SIZE_X][GRID_SIZE_Y];

/* a ripple record -- holds the position and how much time has elapsed since
   it's creation, as well as the maxium time */
typedef struct
{
  int x, y, t, max;
} ripple_t;

/* now make a bunch of them. */
ripple_t ripple[RIPPLE_COUNT];

/* we'll need the screenheight in width to the nearest power of two because
   textures can only have dimensions that are powers of two. */
int nScreenWidth=256, nScreenHeight=256;
int nScreenWidth2, nScreenHeight2;

/* these are all of the user configurable options. The initial values may
get overwritten when LoadConfig is called. */
//int user_options=OPTION_BUFFER|OPTION_QUALITY|OPTION_RIPPLE|OPTION_REFLECTION;
int user_options=OPTION_BUFFER|OPTION_QUALITY|OPTION_RIPPLE|OPTION_REFLECTION;
int max_texture_size=512;
int ripple_speed=10;
int ripple_freq=20;

void ripple_dynamics(void);

void ps2_basic_redraw_128(void);
void ps2_basic_redraw_256(void);

void water_init()
{
  nScreenWidth=256;
  nScreenHeight=256;
  nScreenWidth2=nScreenWidth;
  nScreenHeight2=nScreenHeight;
  ripple_init();
}

/* generates a ripple at the given x and y */
void ripple_click(int mousex, int mousey)
{
  int index;

  index = 0;
  while (ripple[index].t <  RIPPLE_LENGTH && index < RIPPLE_COUNT)
    index++;

  if (index < RIPPLE_COUNT)
  {
    ripple[index].x = (int) ( ((float)mousex/(float)nScreenWidth) * GRID_SIZE_X);
    ripple[index].y = (int) ( ((float)mousey/(float)nScreenHeight)* GRID_SIZE_Y);
    ripple[index].t = RIPPLE_STEP;
    ripple[index].max = RIPPLE_LENGTH;
  }
}

void water_tick(PbTexture *texture,PbTexture *textureenv)
{
  static int frame=50;
  water_draw_256(texture,textureenv);
  ripple_dynamics();
}

float mypow(float x,int n)
{
  float power = 1.0f;

  for (;n > 0; n--)
    power *= x;
  return (power);
}

/* initialize all of those look up tables as well as all of those ripple
   records */
void ripple_init(void)
{
  int i, j;
  float t, mag;

  for(i=0;i<RIPPLE_COUNT;i++)
  {
    ripple[i].t=RIPPLE_LENGTH+1;
    ripple[i].x=0;
    ripple[i].y=0;
    ripple[i].max=RIPPLE_LENGTH;
  }

  /* calculate the ripple amplitude as a function of time */
  ripple_amplitude[0]=0;
  for(i=1;i<RIPPLE_LENGTH;i++)
  {
    t=1.0-i/(RIPPLE_LENGTH-1.0);
    ripple_amplitude[i]=(-PbCos(t*2.0*M_PI*RIPPLE_CYCLES)*0.5 + 0.5)*RIPPLE_AMPLITUDE*mypow(t,RIPPLE_DAMPEN);
  }

  /* and now precalc the ripple table */
  for(i=0;i<GRID_SIZE_X;i++)
    for(j=0;j<GRID_SIZE_Y;j++)
    {
      mag = PbSqrt((i*i)+(j*j));
      //debug_printf(" %d:%d = %d\r\n",i,j,(int)(mag));
      if(mag==0.0)
      {
        ripple_precalc[j][i].dx= 0;
        ripple_precalc[j][i].dy= 0;
        ripple_precalc[j][i].r= 0;
      }
      else
      {
        ripple_precalc[j][i].dx= (float)i/mag;
        ripple_precalc[j][i].dy= (float)j/mag;
        ripple_precalc[j][i].r= mag*GRID_SIZE_X;
      }
    }

  ripple_dynamics();
}

/* add in a ripple at the given x y */
void do_ripple(int x, int y, int frame)
{
  int i, j, r, absx, absy;
  float a, t, sx, sy;
  float ax, ay;
  PbFvec adjuster;

  for(i=0;i<GRID_SIZE_X;i++)
    for(j=0;j<GRID_SIZE_Y;j++)
    {
      if((i-x)<0)
      {
        sx=-1.0f;
        absx=x-i;
      }
      else
      {
        sx=1.0f;
        absx=i-x;
      }

      if((j-y)<0)
      {
        sy=-1.0f;
        absy=y-j;
      }
      else
      {
        sy=1.0f;
        absy=j-y;
      }

      r =  ripple_precalc[absy][absx].r+(RIPPLE_LENGTH-frame);
      if(r<RIPPLE_LENGTH&&r>=0)
      {
        t=1.0f-(float)r/(float)RIPPLE_LENGTH;
        a=t*t*ripple_amplitude[RIPPLE_LENGTH-r];
      }
      else
        a=0.0f;

      if (a != 0.0f)
      {
      ax=a*ripple_precalc[absy][absx].dx*sx;
      ay=a*ripple_precalc[absy][absx].dy*sy;

      // x=t[0],y=t[1],z=n[0],w=n[1]
      PbVu0mVectorLoad(&adjuster,ax,ay,ax*REFMAP_SCALER,ay*REFMAP_SCALER);
      PbVu0mVectorAdd(&ripple_texcoords[i][j],&ripple_texcoords[i][j],&adjuster);
      }
    }
}


// update all of the ripples as well as compute the distortion coords for
// this frame 
void ripple_dynamics(void)
{
  int i, j;
  float ti,tj;
  PbFvec tempv;

  ti = ((float)1.0f/(float)(GRID_SIZE_X-1));
  tj = ((float)1.0f/(float)(GRID_SIZE_Y-1));

  // initalize the distortion grid to be distortion free 
  tempv.z = REFMAP_DEFAULT_X;
  tempv.w = REFMAP_DEFAULT_Y;
  for(i=0;i<GRID_SIZE_X;i++)
  {
    tempv.x = ti * (float)i;
    for(j=0;j<GRID_SIZE_Y;j++)
    {
      tempv.y= tj * (float)j;
      PbVu0mVectorCopy(&ripple_texcoords[i][j],(PbFvec *)&tempv);
    }
  }
  // Now update all the ripples
  for(i=0;i<RIPPLE_COUNT;i++)
    if(ripple[i].t<ripple[i].max)
    {
      ripple[i].t+=ripple_speed;
      do_ripple(ripple[i].x, ripple[i].y, ripple[i].t);
    }
}

//  256x256 version
void water_draw_256(PbTexture *texture,PbTexture *textureenv)
{
  int i, j;
  PbFvec multiplier;

  // convert to ints and apply multiplier
  PbVu0mVectorLoad(&multiplier,texture->x*16,texture->y*16,textureenv->x*16,textureenv->x*16);

  water_primsetup_solid(texture);
  water_giflist_packed_bg(&multiplier);
}


void water_primsetup_solid( PbTexture* pTex) 
{ 
  u64* p_store; 
  u64* p_data; 
  int  size = 4;

  u32 g_PbPrimAlphaEnable=0;
  u32 g_PbPrimContext=0;
  u32 g_PbPrimAlpha=0;
  u64 color = 0x80808080;

  p_store = p_data = PbSprAlloc( 11*16 ); 

  if( g_PbPrimAlphaEnable == TRUE )
    size++;    
   
  *p_data++ = GIF_TAG( size-1, 1, 0, 0, 0, 1 ); 
  *p_data++ = GIF_AD; 

  *p_data++ = PbTextureGetTex0( pTex ); 
  *p_data++ = GS_TEX0_1+g_PbPrimContext; 

  if( g_PbPrimAlphaEnable == TRUE )
  {
    *p_data++ = g_PbPrimAlpha;
    *p_data++ = GS_ALPHA_1+g_PbPrimContext;
  }

  *p_data++ = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 );     
  *p_data++ = GS_TEST_1+g_PbPrimContext;   

  *p_data++ = color; 
  *p_data++ = GS_RGBAQ; 

  PbDmaSend02Spr( p_store, size ); 
}

static PbFvec bg_data[(GRID_SIZE_X*GRID_SIZE_Y*9)+2];

void water_giflist_packed_bg(PbFvec *texmult) 
{ 
  u64* p_store; 
  u64* p_data; 
  u32  size = (GRID_SIZE_X-1) * (GRID_SIZE_Y-1);
  u32  size2 = (size*9)+2;
  u32 i,j;
  u32 i2,j2;
  u32 x,y,x2,y2;
  PbIvec xy;
 
#define OFFX ((2048+20)<<4)
#define OFFY ((2048+10)<<4)

  p_store = p_data = (u64 *) bg_data; 

#define RIP_X 21
#define RIP_Y 9
  //  Fudge - to convert to ints
  for (i = 0; i < (GRID_SIZE_X-1); i++)
  {
    x = (((i*RIP_X)-RIP_X)<<4);
    x2 = OFFX + x + (RIP_X<<4);
    x += OFFX;
    i2 = i + 1;
    xy.x = x;
    xy.z = x2;
    for (j = 0; j < (GRID_SIZE_Y-1) ; j++)
    {
      y = (((j*RIP_Y)-RIP_Y)<<4);
      y2 = OFFY + y + (RIP_Y<<4);
      y += OFFY;
      j2 = j + 1;

      *p_data++ = GIF_TAG( 1, 1, 1, GS_SETREG_PRIM( GS_PRIM_PRIM_TRISTRIP,
                                          0, 1, 0, 0, 0, 1, 0, 0 ), 0, 8 );
      *p_data++ = 0x53535353; // enough for a quad as tristrip

      // x,y,z,w  xy zw
      // Vert 1
      *((u32*)p_data)++ = (u32)(ripple_texcoords[i][j].x * texmult->x);
      *((u32*)p_data)++ = (u32)(ripple_texcoords[i][j].y * texmult->y);
      *p_data++ = 0; 
      *((u32*)p_data)++ = x;
      *((u32*)p_data)++ = y; 
      *p_data++ = 0; 

      // Vert 2
      *((u32*)p_data)++ = (u32)(ripple_texcoords[i][j2].x * texmult->x);
      *((u32*)p_data)++ = (u32)(ripple_texcoords[i][j2].y * texmult->y);
      *p_data++ = 0; 
      *((u32*)p_data)++ = x;
      *((u32*)p_data)++ = y2; 
      *p_data++ = 0; 
  
      // Vert 3
      *((u32*)p_data)++ = (u32)(ripple_texcoords[i2][j].x * texmult->x);
      *((u32*)p_data)++ = (u32)(ripple_texcoords[i2][j].y * texmult->y);
      *p_data++ = 0; 
      *((u32*)p_data)++ = x2;
      *((u32*)p_data)++ = y; 
      *p_data++ = 0; 

      // Vert 4
      *((u32*)p_data)++ = (u32)(ripple_texcoords[i2][j2].x * texmult->x);
      *((u32*)p_data)++ = (u32)(ripple_texcoords[i2][j2].y * texmult->y);
      *p_data++ = 0; 
      *((u32*)p_data)++ = x2;
      *((u32*)p_data)++ = y2; 
      *p_data++ = 0; 
    }
  }
  *p_data++ = GIF_TAG( 1, 1, 0, 0, 0, 1 ); 
  *p_data++ = GIF_AD; 
  *p_data++ = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 );     
  *p_data++ = GS_TEST_1;   

  PbDmaSend02( p_store, size2 ); 
}

