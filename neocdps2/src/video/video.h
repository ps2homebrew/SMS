/*******************************************
**** VIDEO.H - Video Hardware Emulation ****
****            Header File             ****
*******************************************/

#ifndef	VIDEO_H
#define VIDEO_H

#include "../defines.h"

/*-- Defines ---------------------------------------------------------------*/
#define VIDEO_TEXT	0
#define VIDEO_NORMAL	1
#define	VIDEO_SCANLINES	2

#define PAL_MODE 50
#define NTSC_MODE 60

/*-- Global Variables ------------------------------------------------------*/
extern char		*video_vidram;
extern unsigned short	*video_paletteram_ng;
extern unsigned short	video_palette_bank0_ng[4096] __attribute__((aligned(64)));
extern unsigned short	video_palette_bank1_ng[4096] __attribute__((aligned(64)));
extern unsigned short	*video_paletteram_pc;
extern unsigned short	video_palette_bank0_pc[4096] __attribute__((aligned(64)));
extern unsigned short	video_palette_bank1_pc[4096] __attribute__((aligned(64)));
extern short		video_modulo;
extern unsigned short	video_pointer;
extern unsigned short	*video_paletteram;
extern unsigned short	*video_paletteram_pc;
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
int  video_set_mode(int);
void video_draw_screen1(void);
void video_draw_spr(unsigned int code, unsigned int color, int flipx,
			int flipy, int sx, int sy, int zx, int zy);
void video_setup(void);
void video_mode_toggle(void);
void incframeskip(void);

void set_neo_fill_color (uint16 color);
void flip_buffers(void);
void clean_active_buffer(void);
void clean_buffers(void);
void neocd_draw_curframe(void);

void blitter(void);

/*-- draw_fix.c functions -------------------------------------------------*/
void video_draw_fix(void);
void fixputs(uint16 x, uint16 y, const char * string);

/*-- UI functions -------------------------------------------------*/
void display_splashscreen(void);
void display_insertscreen(void);
void display_loadingscreen(void);
#endif /* VIDEO_H */

