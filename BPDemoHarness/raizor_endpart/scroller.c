/*
 * scroller.c - simple horizontal scroller, uses Pb demo library
 *
 * Copyright (c) 2004   raizor <raizor@c0der.net>
 *
 */

#include <string.h>
#include <fileio.h>
#include "PbPrim.h"

#define scrollMaxLen 8200 // max len of scrolltext
char scrollerSpeed = 1; // speed of scroller
int scrollOffset = 0; // text array offset
int scrollOffsetX = 0; // current screen offset for text
char scrollText[scrollMaxLen]; // total scrolltext
char scrollBuffer[scrollMaxLen]; // current chars on screen
unsigned int scrollTextSize = 0; // number of chars in text
unsigned short scrollWidth = 660; //screen width+
unsigned int scrollBufferWidth; // width of buffer
unsigned short* font_lookuptable;
int scrollerY = 0;
PbTexture* tex;

int s_GetCharWidth(char ch)
{
	return (int)( *(font_lookuptable+(ch*4))+
				  *(font_lookuptable+(ch*4)+2) );	
}

// puts a char to screen at pos x/y
void s_PutChar(short x, short y, char ch)
{
	unsigned short *charRect; // current char tex coords
	unsigned short x0, y0, x1, y1, ch_w, ch_h;	// rectangle for current character

	// Read the texture coordinates for current character
	charRect = font_lookuptable+(ch*4);		
	x0 = *charRect++;
	y0 = *charRect++;
	x1 = *charRect++;
	y1 = *charRect++;
	ch_w = x1 - x0;
	ch_h = y1 - y0;

    PbPrimSpriteTexture( tex, 
                         x<<4, y<<4,  
						 x1<<4, y1<<4, 
                         (x+ch_w)<<4, (y+ch_h)<<4,
						 x0<<4, y0<<4,
						 0, 0x80808080 );
	//printf("charput: %c\n",ch
}

// internal buffer update func
void s_UpdateBuffer()
{
	int i;

	scrollBufferWidth = 0;

	for (i=0; scrollBufferWidth<scrollWidth+20; i++)
	{
		if (i+scrollOffset<scrollTextSize)
		{
			scrollBuffer[i] = scrollText[i+scrollOffset];
			
			//scrollBufferWidth += g2_get_char_width(scrollText[i+scrollOffset], 1.0f, 1);
			scrollBufferWidth += s_GetCharWidth(scrollText[i+scrollOffset]);
			
		}else{
			if (i>0)
			{
				scrollBuffer[i] = 0x00; //null
			}else{
				scrollOffset = 0;
				scrollOffsetX = scrollWidth;
				s_UpdateBuffer();
			}
			break;
		}
	}
}

// init function, args:scrolltext, lookup, y screen offset
void Scroller_init(char* stxt, unsigned short *lookup,
				   PbTexture *p_tex, unsigned short y)
{	
	scrollTextSize = strlen(stxt);
	sprintf(scrollText,stxt);
	font_lookuptable = lookup;
	tex = p_tex;
	scrollOffsetX = scrollWidth;
	scrollerY = y;
	s_UpdateBuffer();
}

// update function, call once per frame to draw scroller
void Scroller_tick()
{
	int i, charOffset;

	if (scrollOffsetX>-s_GetCharWidth(scrollBuffer[0]))
	{
		scrollOffsetX-=scrollerSpeed;
	}else{
		scrollOffset++;
		s_UpdateBuffer();

		// since we're updating the buffer, we also need to update the scroll offsetX

		scrollOffsetX+=1+s_GetCharWidth(scrollText[scrollOffset-1])-scrollerSpeed;
	}

	charOffset = 0;
	
	for (i=0; i<strlen(scrollBuffer); i++)
	{
		s_PutChar(scrollOffsetX+charOffset, scrollerY,scrollBuffer[i]);
		charOffset+=s_GetCharWidth(scrollBuffer[i])+1;		
	}
}