/*******************************************
**** VIDEO.H - Video Hardware Emulation ****
****            Header File             ****
*******************************************/

#ifndef	VIDEO_H
#define VIDEO_H

#include "../defines.h"

/*-- Defines ---------------------------------------------------------------*/
#define PAL_MODE 50
#define NTSC_MODE 60

#define SMS_TEX		0x0B0000

/*
	X zoom table - verified on real hardware
	        8         = 0
	    4   8         = 1
	    4   8  c      = 2
	  2 4   8  c      = 3
	  2 4   8  c e    = 4
	  2 4 6 8  c e    = 5
	  2 4 6 8 a c e   = 6
	0 2 4 6 8 a c e   = 7
	0 2 4 6 89a c e   = 8
	0 234 6 89a c e   = 9
	0 234 6 89a c ef  = A
	0 234 6789a c ef  = B
	0 234 6789a cdef  = C
	01234 6789a cdef  = D
	01234 6789abcdef  = E
	0123456789abcdef  = F

static char zoomx_draw_tables[16][16] =
{
	{ 0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0 },
	{ 0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0 },
	{ 0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0 },
	{ 0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0 },
	{ 0,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0 },
	{ 0,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0 },
	{ 0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
	{ 1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
	{ 1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0 },
	{ 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,0 },
	{ 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,1 },
	{ 1,0,1,1,1,0,1,1,1,1,1,0,1,0,1,1 },
	{ 1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
	{ 1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
	{ 1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1 },
	{ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
};
*/
/*-- Global Variables ------------------------------------------------------*/
extern char		*video_vidram ;// __attribute__((aligned(64)));
//extern unsigned short	*video_vidram;//  __attribute__((aligned(64)));
extern unsigned short	*video_paletteram_ng __attribute__((aligned(64)));
extern unsigned short	video_palette_bank0_ng[4096] __attribute__((aligned(64)));
extern unsigned short	video_palette_bank1_ng[4096] __attribute__((aligned(64)));
extern unsigned short	*video_paletteram_pc __attribute__((aligned(64)));
extern unsigned short	video_palette_bank0_pc[4096] __attribute__((aligned(64)));
extern unsigned short	video_palette_bank1_pc[4096] __attribute__((aligned(64)));
extern short		video_modulo;
extern unsigned short	video_pointer;
extern unsigned short	*video_paletteram __attribute__((aligned(64)));
extern unsigned short	*video_paletteram_pc __attribute__((aligned(64)));
extern unsigned short	video_palette_bank0[4096] __attribute__((aligned(64)));
extern unsigned short	video_palette_bank1[4096] __attribute__((aligned(64)));
extern unsigned short	*video_line_ptr[224] __attribute__((aligned(64)));
extern unsigned char	video_fix_usage[4096] __attribute__((aligned(64)));
extern unsigned char	rom_fix_usage[4096] __attribute__((aligned(64)));
extern unsigned char	video_spr_usage[32768] __attribute__((aligned(64)));
extern unsigned char	rom_spr_usage[32768] __attribute__((aligned(64)));
extern unsigned int	video_hide_fps;
extern unsigned short	video_color_lut[32768] __attribute__((aligned(64)));

extern int		video_mode;
extern double		gamma_correction;
extern int		frameskip;

extern unsigned int	neogeo_frame_counter;
extern unsigned int	neogeo_frame_counter_speed;

/*-- video.c functions ----------------------------------------------------*/
int  video_init(void);
void video_shutdown(void);
int  video_set_mode();
void video_precalc_lut(void);
void video_draw_screen1(void);
inline void video_draw_spr(unsigned int code, unsigned int color, int flipx,
			int flipy, int sx, int sy, int zx, int zy);
void video_setup(void);
void incframeskip(void);
void clean_buffers(void);
void blitter(void);

/*-- draw_fix.c functions -------------------------------------------------*/
void video_draw_fix(void);
//void clean_buffers(void);
/*-- UI functions -------------------------------------------------*/
void display_splashscreen(void);
void display_insertscreen(void);
void display_loadingscreen(void);
#endif /* VIDEO_H */

