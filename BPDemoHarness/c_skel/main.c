#include <tamtypes.h>
#include <stdio.h>
#include "gs.h"
#include "prim.h"
#include "../harness.h"

u32 start_demo(const demo_init_t *t)

{
   int fb = 0;
   u32 colour = 0;
   u32 last_time;
   u32 curr_frame;

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
     clr_scr(colour++); 
     fill_rect(-10, -20, 320, 112, 0, 0xFFFFFF);
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
