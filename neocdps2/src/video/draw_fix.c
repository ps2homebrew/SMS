/******************************************
**** Fixed Text Layer Drawing Routines ****
******************************************/

#include <stdio.h>
#include <string.h>

#include "video.h"
#include "../neocd.h"


/* Draw Single FIX character */
inline void draw_fix(uint16 code, uint16 colour, uint16 sx, uint16 sy)
{

	register uint16 y;
	uint32 mydword;
	uint32 * fix=(uint32*)&(neogeo_fix_memory[(code<<5)])+7;//fix_memory
	uint16 * dest;
	uint16 * paldata=&video_paletteram_pc[colour]; //palette
	uint16 col;

	for( y=8; y--; )
	{
		dest     = video_line_ptr[sy+y]+sx;
		mydword  = *fix--;
	
		col = (mydword>> 0)&0x0f; if (col) dest[0] = paldata[col];
		col = (mydword>> 4)&0x0f; if (col) dest[1] = paldata[col];
		col = (mydword>> 8)&0x0f; if (col) dest[2] = paldata[col];
		col = (mydword>>12)&0x0f; if (col) dest[3] = paldata[col];
		col = (mydword>>16)&0x0f; if (col) dest[4] = paldata[col];
		col = (mydword>>20)&0x0f; if (col) dest[5] = paldata[col];
		col = (mydword>>24)&0x0f; if (col) dest[6] = paldata[col];
		col = (mydword>>28)&0x0f; if (col) dest[7] = paldata[col];
	}

}


/* Draw entire Character Foreground */
inline void video_draw_fix(void)
{

	register uint16 x, y;
	register uint16 code, colour;
	uint16 * fixarea=(uint16 *)&video_vidram[0xe004];

	for (y=0; y < 28; y++)
	{
		for( x=40; x--; )
		{
			code = fixarea[x << 5];
			colour = (code&0xf000)>>8;
			code  &= 0xfff;
			if (video_fix_usage[code])//==0)
			  //continue;
			draw_fix(code,colour,x<<3,y<<3);
		}
		fixarea++;
	}
}

