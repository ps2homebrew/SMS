/* Primitive drawing routines */

#include <kernel.h>
#include <tamtypes.h>
#include "ps2gs.h"
#include "dma.h"
#include "gs.h"
#include "gif.h"
#include "shapes.h"
#include "prim.h"

DECLARE_EXTERN_GS_PACKET(dma_buf);
extern int offs_x, offs_y; /* Offset of framebuffer to display */
int alpha_en = 0; /* Is alpha blending enabled ? */
u64 alpha;

int abs(int x)

{
   if(x < 0) x = -x;
 
   return x;
}

u16 gs_texture_wh(u16 n)
{
  u16 l=0;
  
  n--;
  while(n>0) n>>=1, l++;
  return(l);
}

void draw_trilist(const point_vu *v, const polygon *p, int count, u32 *col, polygon *map)

{
  int loop;
  s32 l_offs_x = offs_x << 4;
  s32 l_offs_y = offs_y << 4;

  BEGIN_GS_PACKET(dma_buf);

  if(alpha_en)
    {
      GIF_TAG_AD(dma_buf, 2 + 6 * count, 1, 0, 0, 0);
    }
  else
    {
      GIF_TAG_AD(dma_buf, 1 + 6 * count, 1, 0, 0, 0);
    }

  
  if(alpha_en)
    {
      GIF_DATA_AD(dma_buf, PS2_GS_ALPHA_1, alpha);
    }

  GIF_DATA_AD(dma_buf, PS2_GS_PRIM, PS2_GS_SETREG_PRIM(PS2_GS_PRIM_PRIM_TRIANGLE, 1, 0, 0, alpha_en, 0, 0, 0, 0));

  for(loop = 0; loop < count; loop++)
    {
      GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col[map[loop].v[0]]);
      GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(v[p[loop].v[0]].x + l_offs_x, v[p[loop].v[0]].y + l_offs_y, 
							   v[p[loop].v[0]].z));
      GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col[map[loop].v[1]]);
      GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(v[p[loop].v[1]].x + l_offs_x, v[p[loop].v[1]].y + l_offs_y, 
							   v[p[loop].v[1]].z));
      GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col[map[loop].v[2]]);
      GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(v[p[loop].v[2]].x + l_offs_x, v[p[loop].v[2]].y + l_offs_y, 
							   v[p[loop].v[2]].z));
    }

  SEND_GS_PACKET(dma_buf);
}

void draw_trilist_tex(const point_vu *v, const point_vu *uv1, const polygon *p, int count, u32 *col, polygon *map, u32 tex_p, s32 texw, s32 texh)

{
  int loop;
  s32 l_offs_x = offs_x << 4;
  s32 l_offs_y = offs_y << 4;

  BEGIN_GS_PACKET(dma_buf);

  if(alpha_en)
    {
      GIF_TAG_AD(dma_buf, 4 + 9 * count, 1, 0, 0, 0);
    }
  else
    {
      GIF_TAG_AD(dma_buf, 3 + 9 * count, 1, 0, 0, 0);
    }

  
  GIF_DATA_AD(dma_buf, PS2_GS_TEXFLUSH, 0x42);
  
  GIF_DATA_AD(dma_buf, PS2_GS_TEX0_1,
	      PS2_GS_SETREG_TEX0_1(
				   tex_p / 64,	// base pointer
				   texw  / 64,	// width
				   0,					// 32bit RGBA
				   gs_texture_wh(texw),	// width
				   gs_texture_wh(texh),	// height
				   1,					// RGBA
				   PS2_GS_TEX_TFX_MODULATE,		       
				   0,0,0,0,0));
  
  //GIF_DATA_AD(dma_buf, PS2_GS_TEX1_1, PS2_GS_SETREG_TEX1_1(0, 0, 1, 1, 0, 0, 0));
  
  if(alpha_en)
    {
      GIF_DATA_AD(dma_buf, PS2_GS_ALPHA_1, alpha);
    }

  GIF_DATA_AD(dma_buf, PS2_GS_PRIM, PS2_GS_SETREG_PRIM(PS2_GS_PRIM_PRIM_TRIANGLE, 1, 1, 0, alpha_en, 0, 1, 0, 0));

  for(loop = 0; loop < count; loop++)
    {
      GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col[map[loop].v[0]]);
      GIF_DATA_AD(dma_buf, PS2_GS_UV, PS2_GS_SETREG_UV(uv1[map[loop].v[0]].x, uv1[map[loop].v[0]].y));
      GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(v[p[loop].v[0]].x + l_offs_x, v[p[loop].v[0]].y + l_offs_y,
							   v[p[loop].v[0]].z));
      GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col[map[loop].v[1]]);
      GIF_DATA_AD(dma_buf, PS2_GS_UV, PS2_GS_SETREG_UV(uv1[map[loop].v[1]].x, uv1[map[loop].v[1]].y));
      GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(v[p[loop].v[1]].x + l_offs_x, v[p[loop].v[1]].y + l_offs_y,
							   v[p[loop].v[1]].z));
      GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col[map[loop].v[2]]);
      GIF_DATA_AD(dma_buf, PS2_GS_UV, PS2_GS_SETREG_UV(uv1[map[loop].v[2]].x, uv1[map[loop].v[2]].y));
      GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(v[p[loop].v[2]].x + l_offs_x, v[p[loop].v[2]].y + l_offs_y,
							   v[p[loop].v[2]].z));
    }

  SEND_GS_PACKET(dma_buf);
}

void draw_trilist_linear(const point_vu *v, int count, u32 *col)

{
  int loop;
  s32 l_offs_x = offs_x << 4;
  s32 l_offs_y = offs_y << 4;

  BEGIN_GS_PACKET(dma_buf);

  if(alpha_en)
    {
      GIF_TAG_AD(dma_buf, 2 + 2 * count, 1, 0, 0, 0);
      GIF_DATA_AD(dma_buf, PS2_GS_ALPHA_1, alpha);
    }
  else
    {
      GIF_TAG_AD(dma_buf, 1 + 2 * count, 1, 0, 0, 0);
    }

  GIF_DATA_AD(dma_buf, PS2_GS_PRIM, PS2_GS_SETREG_PRIM(PS2_GS_PRIM_PRIM_TRIANGLE, 1, 0, 0, alpha_en, 0, 0, 0, 0));

  for(loop = 0; loop < count; loop++)
    {
      GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col[loop]);
      GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(v[loop].x + l_offs_x, v[loop].y + l_offs_y, v[loop].z));
    }

  SEND_GS_PACKET(dma_buf);
}

void fill_rect(s32 x0, s32 y0, s32 x1, s32 y1, u32 z, u32 col)
{
  x0 += offs_x;
  y0 += offs_y;
  x1 += offs_x;
  y1 += offs_y;

  BEGIN_GS_PACKET(dma_buf);
 
  if(alpha_en)
    {
      GIF_TAG_AD(dma_buf, 5, 1, 0, 0, 0);
      GIF_DATA_AD(dma_buf, PS2_GS_ALPHA_1, alpha);
    }
  else
    {
      GIF_TAG_AD(dma_buf, 4, 1, 0, 0, 0);
    }
  
  GIF_DATA_AD(dma_buf, PS2_GS_PRIM,
	      PS2_GS_SETREG_PRIM(PS2_GS_PRIM_PRIM_SPRITE, 0, 0, 0, alpha_en, 0, 0, 0, 0));
  
  GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col);
  
  GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(x0<<4, y0<<4, z));
  
  GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2((x1+1)<<4, (y1+1)<<4, z));
  
  SEND_GS_PACKET(dma_buf);
}

void fill_rect_tex(rect r, u32 tex_p, u32 texw, u32 texh)
{
  r.v[0].x += offs_x;
  r.v[0].y += offs_y;
  r.v[1].x += offs_x;
  r.v[1].y += offs_y;

  BEGIN_GS_PACKET(dma_buf);
  
  if(alpha_en)
    {
      GIF_TAG_AD(dma_buf, 10, 1, 0, 0, 0);
    }
  else
    {
      GIF_TAG_AD(dma_buf, 9, 1, 0, 0, 0);
    }
  
  GIF_DATA_AD(dma_buf, PS2_GS_TEXFLUSH, 0x42);
  
  GIF_DATA_AD(dma_buf, PS2_GS_TEX0_1,
	      PS2_GS_SETREG_TEX0_1(
		      tex_p / 64,	// base pointer
		      texw  / 64,	// width
		      0,					// 32bit RGBA
		      gs_texture_wh(texw),	// width
		      gs_texture_wh(texh),	// height
		      1,					// RGBA
		      PS2_GS_TEX_TFX_MODULATE,		       
		      0,0,0,0,0));

  GIF_DATA_AD(dma_buf, PS2_GS_TEX1_1, PS2_GS_SETREG_TEX1_1(0, 0, 1, 1, 0, 0, 0));

  if(alpha_en)
    {
      GIF_DATA_AD(dma_buf, PS2_GS_ALPHA_1, alpha);
    }

  GIF_DATA_AD(dma_buf, PS2_GS_PRIM,
	      PS2_GS_SETREG_PRIM(PS2_GS_PRIM_PRIM_SPRITE, 0, 1, 0, alpha_en, 0, 1, 0, 0));
  
  GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, r.col);
  
  GIF_DATA_AD(dma_buf, PS2_GS_UV, PS2_GS_SETREG_UV(r.v[0].u<<4, r.v[0].v<<4));
  GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(r.v[0].x << 4, r.v[0].y << 4, r.v[0].z));
  GIF_DATA_AD(dma_buf, PS2_GS_UV, PS2_GS_SETREG_UV(r.v[1].u<<4, r.v[1].v<<4));  
  GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(r.v[1].x << 4, r.v[1].y << 4, r.v[1].z));

  SEND_GS_PACKET(dma_buf);
}

void put_pixel(u16 x, u16 y, u32 col)
{
  u32 *d;

  x += offs_x;
  y += offs_y;
  
  BEGIN_GS_PACKET(dma_buf);
  
  GIF_TAG_AD(dma_buf, 3, 1, 0, 0, 0);
  GIF_DATA_AD(dma_buf, PS2_GS_PRIM, PS2_GS_SETREG_PRIM(PS2_GS_PRIM_PRIM_POINT, 0, 0, 0, 0, 0, 0, 0, 0));
  GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col);
  GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(x<<4, y<<4, 0));

  d = (u32 *) dma_buf;
  
  SEND_GS_PACKET(dma_buf);
}

void draw_line(u16 x1, u16 y1, u16 x2, u16 y2, int col)
{
  x1 += offs_x;
  y1 += offs_y;
  x2 += offs_x;
  y2 += offs_y;
  
  BEGIN_GS_PACKET(dma_buf);
  
  GIF_TAG_AD(dma_buf, 6, 1, 0, 0, 0);
  GIF_DATA_AD(dma_buf, PS2_GS_PRIM, PS2_GS_SETREG_PRIM(PS2_GS_PRIM_PRIM_LINE, 1, 0, 0, 1, 1, 0, 0, 0));
  GIF_DATA_AD(dma_buf, PS2_GS_ALPHA_1, PS2_GS_SETREG_ALPHA(0, 1, 0, 1, 0x80));
  GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col);
  GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(x1<<4, y1<<4, 0));
  GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, 0x80000000 | col);
  GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(x2<<4, y2<<4, 0));
  
  SEND_GS_PACKET(dma_buf);
}

void draw_line_list(point_vu *p, u32 *col, int num)

     /* p is a list of end points of the line, 2 points per line. cols is colours of line */

{
  u32 i;
  u32 offs_x_f, offs_y_f;

  offs_x_f = offs_x << 4;
  offs_y_f = offs_y << 4;

  BEGIN_GS_PACKET(dma_buf);
 
  GIF_TAG_AD(dma_buf, (num << 1) + 2, 1, 0, 0, 0);
  GIF_DATA_AD(dma_buf, PS2_GS_ALPHA_1, PS2_GS_SETREG_ALPHA(0, 1, 0, 1, 0x80));

  GIF_DATA_AD(dma_buf, PS2_GS_PRIM, PS2_GS_SETREG_PRIM(PS2_GS_PRIM_PRIM_LINE, 1, 0, 0, 1, 1, 0, 0, 0));

  for(i = 0; i < num; i++)
    {

      GIF_DATA_AD(dma_buf, PS2_GS_RGBAQ, col[i]);
      GIF_DATA_AD(dma_buf, PS2_GS_XYZ2, PS2_GS_SETREG_XYZ2(p[i].x + offs_x_f, p[i].y + offs_y_f, p[i].z));
    }

  SEND_GS_PACKET(dma_buf);
}

void set_alpha_en(int alphamode)

{
  alpha_en = alphamode;
}

void set_alpha(u64 A, u64 B, u64 C, u64 D, u64 FIX)

{
  alpha = PS2_GS_SETREG_ALPHA(A, B, C, D, FIX);
}
