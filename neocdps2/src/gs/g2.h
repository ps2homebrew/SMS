//---------------------------------------------------------------------------
// File:	g2.h
// Author:	Tony Saveski, t_saveski@yahoo.com
// Notes:	Simple 'High Level' 2D Graphics Library
//---------------------------------------------------------------------------
#ifndef G2_H
#define G2_H

extern int disp_w;
extern int disp_h;

#include "../defines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	 PAL_256_256_32=0
	,PAL_320_256_32
	,PAL_384_256_32
	,PAL_512_256_32
	,PAL_640_256_32
	,PAL_320_256_16

	,NTSC_256_224_32
	,NTSC_320_224_32
	,NTSC_384_224_32
	,NTSC_512_224_32
	,NTSC_640_224_32
} g2_video_mode;

extern int g2_init(g2_video_mode mode);
extern void g2_end(void);

extern uint16 g2_get_max_x(void);
extern uint16 g2_get_max_y(void);

extern void g2_set_color(uint8 r, uint8 g, uint8 b);
extern void g2_set_fill_color(uint8 r, uint8 g, uint8 b);
extern void g2_get_color(uint8 *r, uint8 *g, uint8 *b);
extern void g2_get_fill_color(uint8 *r, uint8 *g, uint8 *b);

extern void g2_put_pixel(int16 x, int16 y);
extern void g2_line(int16 x0, int16 y0, int16 x1, int16 y1);
extern void g2_rect(int16 x0, int16 y0, int16 x1, int16 y1);
extern void g2_fill_rect(int16 x0, int16 y0, int16 x1, int16 y1);
extern void g2_put_image(int16 x, int16 y, int16 w, int16 h, int16 dw, int16 dh, const uint32 *data);
extern void g2_put_image16(uint16 x, uint16 y, uint16 w, uint16 h, uint16 *data); 
extern void g2_put_image_tc(int16 x, int16 y, int16 w, int16 h, uint32 *data,
				int16 iw, int16 ih, int16 u0, int16 v0, int16 u1, int16 v1);

extern void g2_set_viewport(uint16 x0, uint16 y0, uint16 x1, uint16 y1);
extern void g2_get_viewport(uint16 *x0, uint16 *y0, uint16 *x1, uint16 *y1);

extern void g2_flip_buffers();

extern void g2_set_visible_frame(uint8 frame);
extern void g2_set_active_frame(uint8 frame);
extern uint8 g2_get_visible_frame(void);
extern uint8 g2_get_active_frame(void);

extern void g2_wait_vsync(void);
extern void g2_wait_hsync(void);

extern void g2_set_font(uint32 *data, uint16 w, uint16 h, uint16 *tc);
extern void g2_out_text(int16 x, int16 y, char *str);
extern void g2_set_font_spacing(uint16 s);
extern uint16 g2_get_font_spacing(void);
extern void g2_set_font_mag(uint16 m);
extern uint16 g2_get_font_mag(void);
extern void g2_set_font_color(uint8 red, uint8 green, uint8 blue);

#ifdef __cplusplus
}
#endif

#endif // G2_H

