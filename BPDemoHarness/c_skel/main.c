/****************************************************************
    main.c - Main code of the C skeleton for the BP demo harness
*****************************************************************/

#include <tamtypes.h>
#include "../harness.h"
#include "PbScreen.h"
#include "PbPrim.h"

int bars[40];
#define FALLOFF 2
int xscale[41] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17,
		  18, 19, 20, 24, 27, 29, 31, 35, 40, 50, 60, 70, 80, 90, 
		  100, 120, 140, 160, 180, 240, 300, 350, 450, 511};

u32 start_demo(const demo_init_t *t)

{
   u32 colour = 0;
   u32 curr_frame = 0;
   u16 *fft_data;

   PbScreenSetup( SCR_W, SCR_H, SCR_PSM );

   t->printf("Hello World (C)!\n");

   while(t->frame_count > 0)
   {
     int loop;
     int loop2;

     PbScreenClear(colour++);
     fft_data = t->get_fft();

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

        PbPrimSprite(loop << 8, (SCR_H - bars[loop]) << 4, 
                    ((loop << 4) + 16) << 4, SCR_H << 4, 0, 0xFF00);
     }

     PbScreenSyncFlip();

     curr_frame++;
   }
 
   t->printf("Frame Count = %d\n", curr_frame);
   return t->screen_mode;
}
