#include <tamtypes.h>
#include <stdio.h>
#include "gs.h"
#include "prim.h"
#include "../harness.h"

int bars[40];
#define FALLOFF 2
int xscale[41] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17,
		  18, 19, 20, 24, 27, 29, 31, 35, 40, 50, 60, 70, 80, 90, 
		  100, 120, 140, 160, 180, 240, 300, 350, 450, 511};

u32 start_demo(const demo_init_t *t)

{
   int fb = 0;
   u32 colour = 0;
   u32 last_time;
   u32 curr_frame;
   u16 *fft_data;

   init_gs();
   t->printf("Hello World (C)!\n");
   set_bg_colour(0xFF, 0xFF, 0xFF);
   set_active_fb(fb);
   clr_scr(colour++);

   set_visible_fb(fb);
   fb ^= 1;
   set_active_fb(fb);
   last_time = t->time_count_i >> 16;
   curr_frame = 0;

   while(t->frame_count > 0)
   {
     int loop;
     int loop2;

     fft_data = t->get_fft();
     clr_scr(colour++); 
     for(loop = 0; loop < 40; loop++)
     { 
        u16 y;

        y = 0;
        for(loop2 = xscale[loop]; loop2 < xscale[loop+1]; loop2++)
        {
           if(fft_data[loop2] > y)
             y = fft_data[loop2];
        }
 
        y >>= 7; 

        if(y > bars[loop])
        {
           bars[loop] = y;
        } 
        else
        {
           bars[loop] -= FALLOFF;
           if(bars[loop] < 0) bars[loop] = 0;
        }

        fill_rect(loop * 16, 224 - bars[loop], loop*16 + 16, 224, 0, 0xF000);
     }
     wait_vsync();
     set_visible_fb(fb);
     fb ^= 1;
     set_active_fb(fb);
     if(last_time != (t->time_count_i >> 16))
     {
        t->printf("%d\n", last_time);
        last_time = t->time_count_i >> 16;
     }
     curr_frame++;
   }
 
   t->printf("Frame Count = %d\n", curr_frame);
   return t->screen_mode;
}
