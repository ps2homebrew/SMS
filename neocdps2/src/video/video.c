/*
 *  video.c - Video Hardware Emulation
 *  Copyright (C) 2003 Foster (Original Code)
 *  Copyright (C) 2004-2005 Olivier "Evilo" Biot (PS2 Port)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//-- Include Files -----------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <libpad.h>

#include "video.h"
#include "neocd.h"
#include "../input/input.h"
#include "../misc/misc.h"
#include "../misc/timer.h"

#include "../gs/clut.h"
#include "../gs/gfxpipe.h"
#include "../gs/gs.h"
#include "../gs/hw.h"


// resources
#include "resources/splash.h"
#include "resources/loading.h"
#include "resources/credit.h"

//-- Defines -----------------------------------------------------------------

#define	LINE_BEGIN	mydword = *((int *)fspr)
#define LINE_MID	mydword = *((int *)fspr+1)
#define PIXEL_LAST	col = (mydword&0x0F); if (col) *bm = paldata[col]
#define PIXEL_R		col = (mydword&0x0F); if (col) *bm = paldata[col]; bm--
#define PIXEL_F		col = (mydword&0x0F); if (col) *bm = paldata[col]; bm++
#define SHIFT7		mydword >>= 28
#define SHIFT6		mydword >>= 24
#define SHIFT5		mydword >>= 20
#define SHIFT4		mydword >>= 16
#define SHIFT3		mydword >>= 12
#define SHIFT2		mydword >>= 8
#define SHIFT1		mydword >>= 4


//-- Global Variables --------------------------------------------------------
char *video_vidram;
//unsigned short *video_vidram;
unsigned short	*video_paletteram_ng __attribute__((aligned(64)));
unsigned short	video_palette_bank0_ng[4096] __attribute__((aligned(64)));
unsigned short	video_palette_bank1_ng[4096] __attribute__((aligned(64)));
unsigned short	*video_paletteram_pc __attribute__((aligned(64)));
unsigned short	video_palette_bank0_pc[4096] __attribute__((aligned(64)));
unsigned short	video_palette_bank1_pc[4096] __attribute__((aligned(64)));
unsigned short	video_color_lut[32768] __attribute__((aligned(64)));

short		video_modulo;
unsigned short	video_pointer;

unsigned short 	*video_line_ptr[224] __attribute__((aligned(64)));

unsigned char	video_fix_usage[4096] __attribute__((aligned(64))) ;
//unsigned char	*video_fix_usage;
unsigned char   rom_fix_usage[4096] __attribute__((aligned(64)));  // Utilisé ????????

unsigned char	video_spr_usage[32768] __attribute__((aligned(64))); 
unsigned char   rom_spr_usage[32768] __attribute__((aligned(64))); 

unsigned int dpw = 320;
unsigned int dph = 224;
// swap buffer in vram
int whichdrawbuf = 0; 

#define BUF_WIDTH	336 // size should be 320, but it causes gfx glitches .. !
#define BUF_HEIDTH	224

static uint16 	video_buffer[ BUF_WIDTH * BUF_HEIDTH ] __attribute__((aligned(64))) __attribute__ ((section (".bss"))); // 320*224, rest is padding

static uint32 	*video_frame_buffer;
//static uint32 	video_frame_buffer[ BUF_WIDTH * BUF_HEIDTH ] __attribute__((aligned(64))) __attribute__ ((section (".bss"))); // 320*224, rest is padding;

static uint32 	video_back_buffer[2][ BUF_WIDTH * BUF_HEIDTH ] __attribute__((aligned(64)))  __attribute__ ((section (".bss"))); // 320*224, rest is padding


char 		display_fps[12]  __attribute__((aligned(64))); 
extern int 	fps; 
extern int 	frame_counter;

extern unsigned vsync_freq; 

int videoBufferAvailable = 0; 
// swap soft backbuffer
int whichbackbuf = 0; 
 

//static uint16	*src_blit;  // blitter pointer
//static uint32	*dest_blit; // blitter pointer

unsigned char	*dda_y_skip; // scratchpad
static unsigned char full_y_skip[16] __attribute__ ((section (".sdata"))) = {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};



unsigned int	neogeo_frame_counter = 0;
unsigned int	neogeo_frame_counter_speed = 4;

unsigned int	video_hide_fps=0;
double		gamma_correction = 1.0;
unsigned int	fc=0;

int		frameskip=0,xoffs,yoffs,offs;



//----------------------------------------------------------------------------
int	video_init(void)
{

	int		y,i;

	video_precalc_lut();

	video_vidram = malloc(131072);

	if (video_vidram==NULL) {
		printf("VIDEO: Could not allocate vidram (128k)\n");
		return	0;
	}

	memset((void*)video_palette_bank0_ng, 0, sizeof(video_palette_bank0_ng));
	memset((void*)video_palette_bank1_ng, 0, sizeof(video_palette_bank1_ng));
	memset((void*)video_palette_bank0_pc, 0, sizeof(video_palette_bank0_pc));
	memset((void*)video_palette_bank1_pc, 0, sizeof(video_palette_bank1_pc));

	video_paletteram_ng = video_palette_bank0_ng;
	video_paletteram_pc = video_palette_bank0_pc;
	video_modulo = 1;
	video_pointer = 0;

        for(i=0;i<32768;i++)
            video_spr_usage[i]=1;

	for(y=0;y<dph;y++) {
		video_line_ptr[y] = video_buffer + y * BUF_WIDTH; // 512
	}
	
	// zero-fill both buffers
	memset((void*)video_buffer, 0, sizeof(video_buffer)); 
	//memset((void*)video_frame_buffer, 0, sizeof(video_frame_buffer)); 
	//memset((void*)video_back_buffer[0], 0, sizeof(video_back_buffer[0])); 
	//memset((void*)video_back_buffer[1], 0, sizeof(video_back_buffer[1])); 

	// put in scratchpad
	dda_y_skip  =  (unsigned char *)(0x70000000);
	
	video_frame_buffer = video_back_buffer[whichbackbuf];
	
	return video_set_mode();
}
//----------------------------------------------------------------------------
int video_set_mode()
{

	DmaReset();

	if(pal_ntsc() == GS_PAL) // PAL
	{
		GS_InitGraph(GS_PAL,GS_NONINTERLACE);
		machine_def.dispx = neocdSettings.dispXPAL;
		machine_def.dispy = neocdSettings.dispYPAL;
		machine_def.vdph = 256;
		machine_def.vidsys = PAL_MODE;
		neocdSettings.region = REGION_EUROPE;
		machine_def.fps_rate = FPS_PAL; //50;
		machine_def.snd_sample = (960 >> 2); //12000khz /50
		machine_def.m68k_cycles = 12000000 / 50; // 240.000
		machine_def.z80_cycles = 4000000 / 50;
		machine_def.z80_cycles_slice = machine_def.z80_cycles / 256;
		vsync_freq = GETTIME_FREQ_PAL; 
		
	} 
	else // NTSC
	{
		GS_InitGraph(GS_NTSC,GS_NONINTERLACE);
		machine_def.dispx = neocdSettings.dispXNTSC;
		machine_def.dispy = neocdSettings.dispYNTSC;
		machine_def.vdph = 224;
		machine_def.vidsys = NTSC_MODE;
        	neocdSettings.region = REGION_USA;
        	machine_def.fps_rate = FPS_NTSC; // 60;
        	machine_def.snd_sample = (800 >> 2); //12000khz /60
        	machine_def.m68k_cycles = 12000000 / 60; // 200.000
        	machine_def.z80_cycles = 4000000 / 60;
		machine_def.z80_cycles_slice = machine_def.z80_cycles / 256;
		vsync_freq = GETTIME_FREQ_NTSC; 
		

	}
	
	// beurk..
	machine_def.y1_offset = ((machine_def.vdph-dph)>>1) << 4 ;
	machine_def.y2_offset = (machine_def.vdph-((machine_def.vdph-dph)>>1)) << 4;
	
			
	// set video mode
	GS_SetDispMode(machine_def.dispx,machine_def.dispy,dpw,machine_def.vdph);
	
	// init pipe
	GS_SetEnv(dpw, machine_def.vdph, 0, 0x080000, GS_PSMCT32, 0x100000, GS_PSMZ32);
	install_VRstart_handler();
	createGfxPipe(&thegp /*, (void *)0xF00000*/ , 0x080000);

	// vsync callback
    	addVSyncCallback(&system_graphics_blit); 

	// display loading screen...
	display_loadingscreen();
	
   	
   	return machine_def.vidsys;
}


void incframeskip(void)
{
    frameskip++;
    frameskip=frameskip%12;
}
//----------------------------------------------------------------------------

void video_precalc_lut(void)
{
	int	ndx, rr, rg, rb;
	
	for(rr=0;rr<32;rr++) {
		for(rg=0;rg<32;rg++) {
			for(rb=0;rb<32;rb++) {
				ndx = ((rr&1)<<14)|((rg&1)<<13)|((rb&1)<<12)|((rr&30)<<7)
					|((rg&30)<<3)|((rb&30)>>1);
				video_color_lut[ndx] =
				     ((int)( 31 * pow( (double)rb / 31, 1 / gamma_correction ) )<<0)
					|((int)( 63 * pow( (double)rg / 31, 1 / gamma_correction ) )<<5)
					|((int)( 31 * pow( (double)rr / 31, 1 / gamma_correction ) )<<11);
			}
		}
	}
	
}

//---------------------------------------------------------------------------- 
void video_draw_screen1() 
{ 
   register 	int count,y,i,offs;
   int		sx =0,sy =0,oy =0,my =0,zx = 1, rzy = 1; 
   int         	tileno,tileatr,t1,t2,t3; 
   int          dday=0,rzx=15,yskip=0;
   char        	fullmode=0; 


   for( y=224; y--; )
      memset ((void*)video_line_ptr[y],video_paletteram_pc[4095], BUF_WIDTH * 2);


   for (count=0;count< 0x300 ;count+=2) 
   { 
      t3 = *((unsigned short *)( &video_vidram[0x10000 + count] )); 
      t1 = *((unsigned short *)( &video_vidram[0x10400 + count] )); 
      
      // If this bit is set this new column is placed next to last one 
      if (t1 & 0x40)
      { 
         sx += (rzx + 1); 
         if ( sx >= 0x1F0 ) 
            sx -= 0x200; 

         // Get new zoom for this column 
         zx = (t3 >> 8) & 0x0F; 

         sy = oy; 
      } 
      else // nope it is a new block  
      {   
         // Sprite scaling 
         t2 = *((unsigned short *)( &video_vidram[0x10800 + count] )); 
         
         zx = (t3 >> 8) & 0x0F; 
         rzy = t3 & 0xff; 
	 	 
         sx = (t2 >> 7); 
         if ( sx >= 0x1F0 )
            sx -= 0x200; 	

         // Number of tiles in this strip 
         my = t1 & 0x3f; 
         
         sy = 0x1F0 - (t1 >> 7); // 0x200
         
         
         
         //--------Original
         if (my == 0x20) 
            fullmode = 1; 
         else if (my >= 0x21) 
            fullmode = 2;   // most games use 0x21, but 
         else 
            fullmode = 0;   // Alpha Mission II uses 0x3f 
        
         
         if (sy > 0x100) sy -= 0x200;
          
         if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) 
         { 
            while (sy < -16) sy += (rzy + 1) << 1; //2 * (rzy + 1); 
         } 
         oy = sy; 
         
         if(my==0x21) my=0x20; 
         
         else if(rzy!=0xff && my!=0) 
            my = ((my<<4<<8)/(rzy+1)+15)>>4; //my = ((my*16*256)/(rzy+1)+15)/16;

         if(my>0x20) my=0x20; 

      } 

      rzx = zx; 
      
      // No point doing anything if tile strip is 0 
      if ((my==0)||(sx>=320)) // 320
         continue; 

      // Setup y zoom 
      if(rzy==255) 
         yskip=16; 
      else 
         dday=0;   // =256; NS990105 mslug fix 

      offs = count<<6; 
      

      // my holds the number of tiles in each vertical multisprite block 
      for( y=my; y--; )
      { 
      	
         tileno  = *((unsigned short *)(&video_vidram[offs])); 
         offs+=2; 
         tileatr = *((unsigned short *)(&video_vidram[offs])); 
         offs+=2; 
        

         if (tileatr&0x8) 
            tileno = (tileno&~7)|(neogeo_frame_counter&7); 
         else if (tileatr&0x4) 
            tileno = (tileno&~3)|(neogeo_frame_counter&3); 
            

         //  tileno &= 0x7FFF; 
         if (tileno>0x7FFF) 
            continue; 

         if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) 
         { 
            if (sy >= 248) sy -= (rzy + 1) << 1; //2 * (rzy + 1); 
         } 
         else if (fullmode == 1) 
         { 
            if (y == 0x10) sy -= (rzy + 1) << 1; //2 * (rzy + 1); 
         } 
         else if (sy > 0x110) sy -= 0x200;// NS990105 mslug2 fix

         if (((tileatr>>8)||(tileno!=0))&&(sy < 224))
         { 
	 	// setup yzoom
         	if(rzy!=255) 
	        { 
	            yskip=0; 
	            dda_y_skip[0]=0; 
	            for(i=0;i<16;i++) 
	            { 
	               dda_y_skip[i+1]=0; 
	               dday-=rzy+1; 
	               if(dday<=0) 
	               { 
	                  dday+=256; 
	                  yskip++; 
	               } 
	               dda_y_skip[yskip]++; 
	            } 
	         } 
         
            	video_draw_spr( tileno, 
               			tileatr >> 8, 
               		   	tileatr & 0x01,
               		   	tileatr & 0x02, 
              			sx,sy,rzx,yskip); 
         } 

         sy +=yskip; 
      }  // for y 
   }  // for count 

   if (fc++ > neogeo_frame_counter_speed) 
   { 
      fc=0; // 3
      neogeo_frame_counter++;
   } 
   fc++; 
    
   video_draw_fix(); 
    
   // DRAW !!!!!!!
   blitter();


} 

//       Without  flip    	    With Flip
// 01: X0000000 00000000	00000000 0000000X
// 02: X0000000 X0000000	0000000X 0000000X
// 03: X0000X00 00X00000	00000X00 00X0000X
// 04: X000X000 X000X000	000X000X 000X000X
// 05: X00X00X0 0X00X000	000X00X0 0X00X00X
// 06: X0X00X00 X0X00X00	00X00X0X 00X00X0X
// 07: X0X0X0X0 0X0X0X00	00X0X0X0 0X0X0X0X
// 08: X0X0X0X0 X0X0X0X0	0X0X0X0X 0X0X0X0X
// 09: XX0X0X0X X0X0X0X0	0X0X0X0X X0X0X0XX
// 10: XX0XX0X0 XX0XX0X0	0X0XX0XX 0X0XX0XX
// 11: XXX0XX0X X0XX0XX0	0XX0XX0X X0XX0XXX
// 12: XXX0XXX0 XXX0XXX0	0XXX0XXX 0XXX0XXX
// 13: XXXXX0XX XX0XXXX0	0XXXX0XX XX0XXXXX
// 14: XXXXXXX0 XXXXXXX0	0XXXXXXX 0XXXXXXX
// 15: XXXXXXXX XXXXXXX0	0XXXXXXX XXXXXXXX
// 16: XXXXXXXX XXXXXXXX	XXXXXXXX XXXXXXXX

inline void video_draw_spr(unsigned int code, unsigned int color, int flipx,
			int flipy, int sx, int sy, int zx, int zy)
{

	register int 	y,col;
	register 	uint16 *bm;
	int		oy, ey, dy;
	int		mydword;
	unsigned char	*fspr = 0;
	char		*l_y_skip;
	const unsigned short *paldata;
	


	// IF total transparency, no need to draw
	if (!video_spr_usage[code])  return;

        if (sx < -16) return ; // previously -8, fix left "blinking" column	

   	if(zy == 16)
		 l_y_skip = &full_y_skip[0];
	else
		 l_y_skip = &dda_y_skip[0];

	fspr = neogeo_spr_memory;

	// Mish/AJP - Most clipping is done in main loop
	oy = sy;
  	ey = sy + zy -1; 	// Clip for size of zoomed object

	if (sy < 0)
		sy = 0;
	if (ey > 223) // >=0
		ey = 223;

	if (flipy)	// Y flip
	{
		dy = -8;
	        fspr += ((code+1)<<7) - 8 - ((sy-oy)<<3);
   	} 
	else		// normal
	{
		dy = 8;
         	fspr += ((code)<<7) + ((sy-oy)<<3); 
   	} 

	paldata  = &video_paletteram_pc[color<<4]; //color*16
	
	if (flipx)	// X flip
	{
    		switch(zx) {
		case	0:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_MID;
				SHIFT7;
				PIXEL_LAST;
			}
			break;
		case	1:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 1;
				LINE_BEGIN;
				SHIFT7;
				PIXEL_R;
				LINE_MID;
				SHIFT7;
				PIXEL_LAST;
			}
			break;
		case	2:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 2;
				LINE_BEGIN;
				SHIFT5;
				PIXEL_R;
				LINE_MID;
				SHIFT2;
				PIXEL_R;
				SHIFT5;
				PIXEL_LAST;
			}
			break;
		case	3:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 3;
				LINE_BEGIN;
				SHIFT3;
				PIXEL_R;
				SHIFT4;
				PIXEL_R;
				LINE_MID;
				SHIFT3;
				PIXEL_R;
				SHIFT4;
				PIXEL_LAST;
			}
			break;
		case	4:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 4;
				LINE_BEGIN;
				SHIFT3;
				PIXEL_R;
				SHIFT3;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT3;
				PIXEL_R;
				SHIFT3;
				PIXEL_LAST;
			}
			break;
		case	5:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 5;
				LINE_BEGIN;
				SHIFT2;
				PIXEL_R;
				SHIFT3;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				SHIFT2;
				PIXEL_R;
				SHIFT3;
				PIXEL_R;
				SHIFT2;
				PIXEL_LAST;
			}
			break;		
		case	6:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 6;
				LINE_BEGIN;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_LAST;
			}
			break;
		case	7:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 7;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_LAST;
			}
			break;		
		case	8:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 8;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
			}
			break;
		case	9:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 9;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
			}
			break;		
		case	10:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 10;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
			}
			break;		
		case	11:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 11;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;	
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;	
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
			}
			break;		
		case	12:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 12;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
			}
			break;		
		case	13:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 13;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
			}
			break;		
		case	14:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 14;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
			}
			break;
		case	15:
			for (y = sy;y <= ey;y++)
			{
				fspr += (*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx + 15;
				LINE_BEGIN;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
			}
			break;		
		}
	}
	else		// normal
	{
    		switch(zx) {
		case	0:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_LAST;
			}
			break;
		case	1:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				LINE_MID;
				PIXEL_LAST;
			}
			break;
		case	2:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT5;
				PIXEL_F;
				LINE_MID;
				SHIFT2;
				PIXEL_LAST;
			}
			break;
		case	3:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT4;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT4;
				PIXEL_LAST;
			}
			break;
		case	4:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT3;
				PIXEL_F;
				SHIFT3;
				PIXEL_F;
				LINE_MID;
				SHIFT1;
				PIXEL_F;
				SHIFT3;
				PIXEL_LAST;
			}
			break;
		case	5:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT3;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT3;
				PIXEL_LAST;
			}
			break;
		case	6:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_LAST;
			}
			break;
		case	7:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_LAST;
			}
			break;
		case	8:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_LAST;
			}
			break;
		case	9:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_LAST;
			}
			break;
		case	10:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
			}
			break;
		case	11:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;	
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;	
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
			}
			break;
		case	12:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
			}
			break;
		case	13:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
			}
			break;
		case	14:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
			}
			break;
		case	15:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=(*l_y_skip++)*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
			}
			break;
		}
	}

}

void inline ps2_vsync()
{
	WaitForNextVRstart(1);
}

void inline ps2_switch_buffers()
{
	// Update drawing and display enviroments.
    	GS_SetCrtFB(whichdrawbuf);
    	whichdrawbuf ^= 1;
    	GS_SetDrawFB(whichdrawbuf); 
}
 

/*------------------------------------------------------------*/
/* blit image to screen */
void blitter(void)
{
	// make an awfull RGB565 to 32bit color conversion
	// faster way to do it ???	
	// this should be made using the hardware
	

	uint16 *src_blit  = (uint16 *)&video_buffer[0];
	uint32 *dest_blit = (uint32 *)&video_frame_buffer[0];
	
	
	for( offs= ( BUF_WIDTH * BUF_HEIDTH ); offs--; )
	{	
	   *dest_blit++=CLUT[(uint16)*src_blit++];
	}

	displayGPFrameBuffer(video_frame_buffer);
	//displayGPFrameBuffer(video_back_buffer[whichbackbuf]);
	

    	 // switch back buffers
    	
    	whichbackbuf  ^= 1;
    	video_frame_buffer = video_back_buffer[whichbackbuf]; 
    	
	
	// make video buffer available
	videoBufferAvailable=1;	
	
	//ps2_vsync();
	
	//ps2_switch_buffers();
	
}

// called by the vsync interrupt
void system_graphics_blit(void)
{
    if (videoBufferAvailable==1)
    {
	   ps2_switch_buffers();
	   videoBufferAvailable=0;	
	   frame_counter++;
    } 
    asm __volatile__ ("ei");
}
 

/*-- UI functions -------------------------------------------------*/
// display a 512x256x32 image buffer using GfxPipe
void inline displayGPFrameBuffer(uint32 *buffer)
{	

	gp_uploadTexture(&thegp, NGCD_TEX, 512, 0, 0, GS_PSMCT32, buffer, BUF_WIDTH, BUF_HEIDTH);
	
	//gp_uploadTexture(&thegp, NGCD_TEX, 512, 0, 0, GS_PSMCT16, video_buffer, BUF_WIDTH, BUF_HEIDTH);
	//gp_uploadTexture(&thegp, NGCD_CLUT, 512, 0, 0, GS_PSMCT16, &ngcd_clut, 512, 1); 
	
	gp_setTex(&thegp, NGCD_TEX, 512, GS_TEX_SIZE_512, GS_TEX_SIZE_256, GS_PSMCT32, 0, 0, 0);
	//gp_setTex(&thegp, NGCD_TEX, 512, GS_TEX_SIZE_512, GS_TEX_SIZE_256, GS_PSMCT16, 0, 0, 0);

	gp_texrect(&thegp, 			// gfxpipe	
		   0,    machine_def.y1_offset, // x1,y1
		   0,    0, 			// u1,v1
		   5120, machine_def.y2_offset,	// x2 (320<<4) ,y2
		   5120, 3584, 			// u2 (320<<4), v2(224<<4)
		   10, 				// z
		   GS_SET_RGBA(255, 255, 255, 200) // color
		  );
	
	if (neocdSettings.showFPS)
	{
		sprintf(display_fps,"FPS %d/%d",fps,machine_def.fps_rate);
		textpixel(10,5,GS_SET_RGBA(255, 255, 255, 255)   ,0,0,12,display_fps);
	}		 
		
	gp_hardflush(&thegp);

}
//------------------------------------------------------------------
// display the splash/credit screen
void display_insertscreen(void)
{

   int cmpt=0;
   char text[100];
   static int scroll_delay = 0;
   int i,dec=0;
   
   while(1) 
   {
	cmpt++;
	if(cmpt>81)cmpt=0;
	
	gp_uploadTexture(&thegp, NGCD_TEX, 256, 0, 0, GS_PSMCT32, splash, 256, 256);
	
	//gp_setTex(&thegp, NGCD_TEX, 256, GS_TEX_SIZE_256, GS_TEX_SIZE_256, GS_PSMCT32, NGCD_TEX, 256, GS_PSMCT32);
	gp_setTex(&thegp, NGCD_TEX, 256, GS_TEX_SIZE_256, GS_TEX_SIZE_256, GS_PSMCT32, 0, 0, 0);

	gp_texrect(&thegp, 			// gfxpipe	
		   32<<4, 0, 			// x1,y1
		   0, 0, 			// u1,v1
		   (256+32)<<4, 256<<4,		// x2,y2
		   256<<4, 256<<4, 		// u2,v2
		   10, 				// z
		   GS_SET_RGBA(255, 255, 255, 200) // color
		  );
	
	//gp_gouradrect(&thegp, (/*88+32*/8+32+56)<<4, 92<<4, GS_SET_RGBA(0x00, 0x00, 0x40, 128), (320-56-32-8/*266*/)<<4, 172<<4, GS_SET_RGBA(0x40,0x40, 0x80, 128), 1);
	//gp_linerect(&thegp, (8+32+56)<<4, 92<<4, (320-56-32-8)<<4, 172<<4, 2, GS_SET_RGBA(255, 255, 255, 128));

	//146
	// Neocd/PS2 x.x (c)Evilo '04 	
	sprintf(text,"%s %c.%c %s", neocd_ps2, (char)(VERSION2_MAJOR+0x30), (char)(VERSION2_MINOR+0x30), copyright);
	textCpixel(0,320,146, GS_SET_RGBA(255, 255, 255, 255)   ,0,0,12,text);
	
	//164
	textCpixel(0,320,164, GS_SET_RGBA(255, 255, 255, 255)   ,0,0,12,official_homepage);
	//172
	textCpixel(0,320,172, GS_SET_RGBA(255, 255, 255, 255)   ,0,0,12,neocd_ps2scene);
	

	//196
	if(cmpt<31)
		textCpixel(0,320,198,GS_SET_RGBA(255, 255, 255, 255)	   ,0,0,12," - Insert a CD and/or Press X to continue - ");
        else if (cmpt<64)
        	textCpixel(0,320,198,GS_SET_RGBA(255, 255, 255, (63 - cmpt)),0,0,12," - Insert a CD and/or Press X to continue - ");

	// 212
	for(i=0;i<sizeof(credit_scroll_text);i++)
           if((i*6-dec)<320 && (i*6-dec)>0) printch(i*6-dec,216+4,GS_SET_RGBA(104, 104, 104, 104),credit_scroll_text[i],0,0,12);	
     
     	if(--scroll_delay < 0) 
     	{
           dec++;
           if(dec>(6*sizeof(credit_scroll_text)+1))dec=0;
           scroll_delay = 0;
     	}   


	if (isButtonPressed (PAD_CROSS)) break;
		
	gp_hardflush(&thegp);
	
	ps2_vsync();
	
	ps2_switch_buffers();
    	
    	
   } // end while  	

}

// display the loading screen
void display_loadingscreen(void)
{
	// deactivate bi-linear filtering
	gp_setFilterMethod(0);
	// Clear the screen (with ZBuffer Disabled)
	gp_disablezbuf(&thegp);
	gp_frect(&thegp,0,0,320<<4,machine_def.vdph<<4,0,GS_SET_RGBA(0,0,0,0x80));
	gp_enablezbuf(&thegp);
	displayGPImageBuffer(loading);
	// reset current filter
	gp_setFilterMethod(neocdSettings.renderFilter);
}

// display an error msg screen
void display_errorMessage(char *msg)
{
	// deactivate bi-linear filtering
	gp_setFilterMethod(0);

	gp_uploadTexture(&thegp, NGCD_TEX, 256, 0, 0, GS_PSMCT32, loading, 256, 256);
	
	gp_setTex(&thegp, NGCD_TEX, 256, GS_TEX_SIZE_256, GS_TEX_SIZE_256, GS_PSMCT32, 0, 0, 0);

	gp_texrect(&thegp, 			// gfxpipe	
		   32<<4, 0, 			// x1,y1
		   0, 0, 			// u1,v1
		   (256+32)<<4, 256<<4,		// x2,y2
		   256<<4, 256<<4, 		// u2,v2
		   10, 				// z
		   GS_SET_RGBA(255, 255, 255, 200) // color
		  );	
		  
	
	
	gp_gouradrect(&thegp,(56)<<4, 102<<4,GS_SET_RGBA(204,0x00, 51, 100), (264)<<4, 122<<4, GS_SET_RGBA(204,0x00, 51, 100), 11);
	gp_linerect(&thegp,  (56)<<4, 102<<4, 			             (264)<<4, 122<<4, 12, GS_SET_RGBA(255, 255, 255, 128));
	
	textCpixel(0,320,108, GS_SET_RGBA(255, 255, 255, 255)   ,0,0,13,msg);
	
	gp_hardflush(&thegp);
	
	ps2_vsync();
	
	ps2_switch_buffers();

}

//------------------------------------------------------------------
// display a 256x256x32 fixed image buffer using GfxPipe
void inline displayGPImageBuffer(uint32 *buffer)
{
	gp_uploadTexture(&thegp, NGCD_TEX, 256, 0, 0, GS_PSMCT32, buffer, 256, 256);
	
	//gp_setTex(&thegp, NGCD_TEX, 256, GS_TEX_SIZE_256, GS_TEX_SIZE_256, GS_PSMCT32, NGCD_TEX, 256, GS_PSMCT32);
	gp_setTex(&thegp, NGCD_TEX, 256, GS_TEX_SIZE_256, GS_TEX_SIZE_256, GS_PSMCT32, 0, 0, 0);

	gp_texrect(&thegp, 			// gfxpipe	
		   32<<4, 0, 			// x1,y1
		   0, 0, 			// u1,v1
		   (256+32)<<4, 256<<4,		// x2,y2
		   256<<4, 256<<4, 		// u2,v2
		   1, 				// z
		   GS_SET_RGBA(255, 255, 255, 200) // color
		  );
		
	gp_hardflush(&thegp);

	ps2_vsync();
	
	ps2_switch_buffers();
}
