#ifndef __PRIM_H__
#define __PRIM_H__

#include "shapes.h"

void draw_trilist(const point_vu *v, const polygon *p, int count, u32 *col, polygon *map);
void fill_rect(s32 x0, s32 y0, s32 x1, s32 y1, u32 z, u32 col);
void fill_rect_tex(rect r, u32 tex_p, u32 texw, u32 texh);
void quad_tex(const quad *qd, u32 tex_p, u32 texw, u32 texh);
void put_pixel(u16 x, u16 y, u32 col);
void set_alpha_en(int alphamode);
void set_alpha(u64 A, u64 B, u64 C, u64 D, u64 FIX);
void draw_line(u16 x1, u16 y1, u16 x2, u16 y2, int col);
void put_circle(int cx, int cy, int r, int col);
void draw_line_list(point_vu *p, u32 *col, int num);
void draw_trilist_linear(const point_vu *v,  int count, u32 *col);

#endif
