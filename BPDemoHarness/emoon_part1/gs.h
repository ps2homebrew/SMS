/* Include file for GS code */
#ifndef __GS_H__
#define __GS_H__

void close_gs(void);
int init_gs();

void clr_scr(u32 col); /* Clear the screen using col */
void set_visible_fb(u8 fb);
void set_active_fb(u8 fb);
void wait_vsync(void);
int get_max_x(void);
int get_max_y(void);
void load_image(void *img, int dest, int fbw, int psm, int x, int y, int w, int h);
void set_zbufcmp(int cmpmode);
u32 get_tex_pointer(void);
int is_pal(void);
void set_bg_colour(u32 r, u32 g, u32 b);

#endif
