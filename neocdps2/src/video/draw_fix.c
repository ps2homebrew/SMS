/******************************************
**** Fixed Text Layer Drawing Routines ****
******************************************/

#include <stdio.h>
#include <string.h>

#include "video.h"
#include "../neocd.h"


/* Draw Single FIX character */
inline void draw_fix(uint16 code, uint16 colour, uint16 sx, uint16 sy, uint16 * palette, char * fix_memory)
{

	uint8 y;
	uint32 mydword;
	uint32 * fix=(uint32*)&(fix_memory[code<<5]);
	uint16 * dest;
	uint16 * paldata=&palette[colour];
	uint16 col;

	for(y=0;y<8;y++)
	{
		dest     = video_line_ptr[sy+y]+sx;
		mydword  = *fix++;
		
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
void video_draw_fix(void)
{

	uint16 x, y;
	uint16 code, colour;
	uint16 * fixarea=(uint16 *)&video_vidram[0xe004];

	for (y=0; y < 28; y++)
	{
		for (x = 0; x < 40; x++)
		{
			code = fixarea[x << 5];

			colour = (code&0xf000)>>8;
			code  &= 0xfff;

			if(video_fix_usage[code])
				draw_fix(code,colour,(x<<3),(y<<3), video_paletteram_pc, neogeo_fix_memory);
		}
		fixarea++;
	}
}

/* FIX palette for fixputs*/
uint16 palette[16]={0x0000,0xffff,0x0000,0x0000,
		    0x0000,0x0000,0x0000,0x0000,
		    0xffff,0x0000,0x0000,0x0000,
		    0x0000,0x0000,0x0000,0xffff};

void fixputs( uint16 x, uint16 y, const char * string )
{

	uint8 i;
	int length=strlen(string);
	
	if ( y>27 ) return;
	
	if ( x+length > 40 ) {
		length=40-x;
	}
	
	if (length<0) return;


	y<<=3;
		
	for (i=0; i<length; i++) {	
		draw_fix(toupper(string[i])+0x300,0,(x+i)<<3,y,palette, &neogeo_rom_memory[458752]);
	}
	

	return;
}
