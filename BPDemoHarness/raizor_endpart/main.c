/*
 * main.c - Sample of a simple vu1 drawing program.
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include <tamtypes.h>
#include <kernel.h>
#include "../harness.h"
#include "PbScreen.h"
#include "PbTexture.h"
#include "PbDma.h"
#include "PbPrim.h"

extern u16 font_lookup[];
extern u32 font_tex[];
extern u32 logo_tex[];
extern u32 bg1_tex[];
extern u32 bg2_tex[];

u16* gsw_charSizesPointer = font_lookup;
u16 gsw_screenWidth = 640;
u16 gsw_screenHeight = 256;
u16 gsw_font_spacing = 1;
u8 gsw_stretch_font = 0;

// blah blah blah
#define PI 3.1415926
#define RADIAN(n) (((n)*PI)/180)
#define FSIN(v)   (FLOAT_SIN[((int)(v))%360])
#define FCOS(v)   (FLOAT_COS[((int)(v))%360])
#define ABS(v)    (((v) < 0)?(v)*-1:(v))
// text alignments
#define GS_ALIGN_LEFT   0
#define GS_ALIGN_CENTER 1
#define GS_ALIGN_RIGHT  2
extern const float FLOAT_SIN[360];
extern const float FLOAT_COS[360];

typedef struct TVector3D // 3d vertex
{
   float x, y, z;
   short int sx, sy;
   unsigned int sz;
}TVector3D;

PbTexture* p_texture_font = NULL; // font tex
PbTexture* p_texture_logo = NULL; // logo tex
PbTexture* p_texture_bg1 = NULL; // bg tex 1
PbTexture* p_texture_bg2 = NULL; // bg tex 2

#define xscale 1.0f // x scaling
#define yscale 0.5f // y scaling 
#define gscale 400 // global scaling
#define offx 320 // x offset
#define offy 128 // y offset

// rotation plane & transformed rot plane
TVector3D VERTEXLIST_square_Trans[4];
TVector3D VERTEXLIST_square[4]={
	{ -1.000f, -1.000f,  1.000f, 0, 0}, // front
	{  1.000f, -1.000f,  1.000f, 0, 0}, // top
	{ -1.000f,  1.000f,  1.000f, 0, 0}, // bottom
	{  1.000f,  1.000f,  1.000f, 0, 0}, // left
};

int scrollsize = 34; // number of lines for scroller

static char *scrolltext[] =
{
  "Oddments",
  "**********",
  "",
  "A PS2Dev Production",
  " ",
  "at Breakpoint 2004",
  "",
  "",
  "We give our respect to:",
  "Soopadoopa",
  "Farbrausch",
  "Haujobb",
  "Kewlers",
  "TBL",
  "Tubgirl",
  "and all we forgot!",
  "",
  "", 
  "We would like to give",
  "special thanks to",
  "Drukluft & Gibson / Soopadoopa",
  "for letting us use this font.",
  "",
  "Big thanks also to Trinodia",
  "for providing the soundtrack",
  "to this demo.",
  "",
  "Check out Trinodia's great work:",
  "www.trinodia.net",
  "",
  "",
  "As is traditional in demos,",
  "a few words from the team...",
  "",
  /*"a b c d e f g h i j k l m",
  "n o p q r s t u v w x y z",
  "A B C D E F G H I J K L M",
  "N O P Q R S T U V W X Y Z",
  "a b c d e f g h i j k l m",
  "n o p q r s t u v w x y z",
  "A B C D E F G H I J K L M",
  "N O P Q R S T U V W X Y Z",
  "a b c d e f g h i j k l m",*/
};
// guess ;)
void RotateVertexX(TVector3D *pInput, TVector3D *pOutput, int Angle)
{
   float tempy;
   TVector3D Out;

   tempy = pInput->y * FCOS (Angle) - pInput->z * FSIN (Angle);
   Out.z = pInput->y * FSIN (Angle) + pInput->z * FCOS (Angle);
   Out.x = pInput->x;
   Out.y = tempy;

   *pOutput = Out;
}

void RotateVertexY(TVector3D *pInput, TVector3D *pOutput, int Angle)
{
   float tempx;
   TVector3D Out;

   tempx = pInput->x * FCOS (Angle) - pInput->z * FSIN (Angle);
   Out.z = pInput->x * FSIN (Angle) + pInput->z * FCOS (Angle);
   Out.x = tempx;
   Out.y = pInput->y;

   *pOutput = Out;
}

void RotateVertexZ(TVector3D *pInput, TVector3D *pOutput, int Angle)
{
   float tempx;
   TVector3D Out;

   tempx = (pInput->x * FCOS (Angle) - pInput->y * FSIN (Angle));
   Out.y = (pInput->x * FSIN (Angle) + pInput->y * FCOS (Angle));
   Out.x = tempx;
   Out.z = pInput->z;

   *pOutput = Out;
}

u32 MakeCol(u8 r, u8 g, u8 b, u8 a)
{
    return (r<< 0) | (g<< 8) | (b<<16) | (a<<24);
}


void setBGColorRGB(unsigned char red, unsigned char green, unsigned char blue)
{
	*(unsigned int*)0x120000E0 = (red<< 0) | (green<< 8) | (blue<<16);
}

int ff = 0;

/*
int s_GetCharWidth(char ch)
{
    int cw = (int)( *(font_lookuptable+((ch*4)+2)) - *(font_lookuptable+(ch*4)) );

    if (ch=='G' && !ff)
    {
        printf("Width of %c = %d\n",ch,cw);
        ff=1;
    }
	return cw;
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

    PbPrimSpriteTexture( p_texture_font, 
                         x<<4, y<<4,  
						 x0<<4, y0<<4, 
                         (x+ch_w)<<4, (y+(ch_h))<<4,
						 x1<<4, y1<<4,
						 99, 0x80808080 );

    //printf("PUTCHAR x:%d, y:%d   ",x,y);
}

void s_TextLine(short x, short y, char* txt)
{
    u16 xoff = x;

    while(*txt!=0x00)
    {
        s_PutChar(xoff, y, *txt);
        txt++;
        xoff+=s_GetCharWidth(*txt);
    }      
}

int foff = 256;
int fox = 20;

*/

int internal_gsw_GetStringPos(char* str, int alignMode, int offset)
{
	int stringWidth = 0;
	char ch = *str;
	
	if (alignMode == GS_ALIGN_LEFT)
	{
		return offset;
	}else{
		while (ch)
		{
			if (gsw_stretch_font == 1)
			{
				stringWidth+=(*(&gsw_charSizesPointer[(ch*4)+2]) - 
							*(&gsw_charSizesPointer[ch*4]))*2;
				stringWidth+=4;
			}else{
				stringWidth+=(*(&gsw_charSizesPointer[(ch*4)+2]) - 
							*(&gsw_charSizesPointer[ch*4]));
				stringWidth+=2;
			}
			stringWidth+=gsw_font_spacing;
			str++;
			ch = *str;
		}	

		switch (alignMode)
		{

			case (GS_ALIGN_CENTER):
			{
				return ((gsw_screenWidth-(int)(stringWidth/1.2f))/2);
			}

			case (GS_ALIGN_RIGHT):
			{
				return (gsw_screenWidth+offset)-stringWidth;
			}
		}
	}

	return offset; // catch all
}

u8 CalcTextAlpha(s16 yp)
{
    if (yp<0x60)
    {  
        if (yp>-0x20)
        {
            return yp+0x20;
        }else{
            return 0x0;
        }
    }

    if (yp>256-0x80)
    {
        if (yp<256)
        {
            return (256-yp);
        }else{
            return 0x0;
        }
    }
    return 0x80;
}

void gsw_TextLine(char* text, s16 xp, s16 yp, int z,
				  int alignment, s16 alignPos)
{
	char currentChar; // current character
	u16	*charRect; // current char tex coords	
    u16	x0, y0, x1, y1;	// rectangle for current character
	u16 w, h; // width and height of above rectangle

	currentChar = *text;

	xp += internal_gsw_GetStringPos(text, alignment, alignPos);

	while(currentChar)
	{
		// Read the texture coordinates for current character
		charRect = &gsw_charSizesPointer[currentChar*4];		
		x0 = *charRect++;
		y0 = *charRect++;
		x1 = *charRect++;
		y1 = *charRect++;
		//x0++;
		//x1;
		//y1++;
		w  = x1-x0+1;
		h  = y1-y0+1;

		if (gsw_stretch_font == 1)
		{
			w = w*2;
		}

        PbPrimSpriteTexture( p_texture_font, 
                            xp<<4, yp<<4,  
						    x0<<4, y0<<4, 
                            (int)(xp+(w/1.2f))<<4, (int)(yp+(h/1.2f))<<4,
						    x1<<4, y1<<4,
						    z, MakeCol(0x80,0x80,0x80,CalcTextAlpha(yp)));

		// Advance drawing position
		xp += (int)(w/1.2f) + gsw_font_spacing;	

		// Get next character
		text++;
		currentChar = *text;		
	}
}

int foff = 256;

void DrawScroller()
{
    int i;
    for (i=0; i<scrollsize; i++)
    {
        //s_TextLine(fox,foff+(54*i),scrolltext[i]);
        gsw_TextLine(scrolltext[i], 50, foff+(40*i), 99, GS_ALIGN_CENTER, 0); 
    }
    foff--;

    if (foff==-(scrollsize*50))
    {
        foff = 256;
    }
}

///////////////////////////////////////////////////////////////////////////////
// u32 start_demo
///////////////////////////////////////////////////////////////////////////////

#define DEBUGMODE

u32 start_demo( const demo_init_t* pInfo )
{

  int i;
  int rotAngle = 0; 
  int scalemin = 400;
  int scalemax = 800;  
  int scale[8];
  u8 scaleflag[8];

  scale[0] = 400; 
  scale[1] = 800;
  scaleflag[0] = 0;
  scaleflag[1] = 1;
  /////////////////////////////////////////////////////////////////////////////
  // Setup screen

  PbScreenSetup( SCR_W, SCR_H, SCR_PSM );

  PbPrimSetAlpha( 0, 1, 0, 1, 0x80 );

  FlushCache(0);

  p_texture_font = PbTextureCreate32( font_tex, 1024, 128 ); 
  p_texture_logo = PbTextureCreate32( logo_tex, 256, 256 ); 
  p_texture_bg1 = PbTextureCreate32( bg1_tex, 256, 256 ); 
  p_texture_bg2 = PbTextureCreate32( bg2_tex, 256, 256 ); 

  

  //////////////////////////////////////////////////////////////////////////////
  // Upload our texture to vram

  PbTextureUpload( p_texture_font );
  PbTextureUpload( p_texture_logo );
  PbTextureUpload( p_texture_bg1 );
  PbTextureUpload( p_texture_bg2 );
  PbDmaWait02();

  PbSetLinearFiltering();


  /////////////////////////////////////////////////////////////////////////////
  // Loop of the demo (just clears the screen, vsync and flip buffer

  while( pInfo->time_count > 0 )
  {

#ifdef DEBUGMODE
    setBGColorRGB(0xFF,0,0);
#endif

    PbScreenClear( 0x000000 );

    PbPrimSprite( 0, 0, 640, 256, 1, 0x80000000); 

    ///////////////////////////////////////////////////////////////////////////
    // Draws a sprite with texture
    PbPrimSetState( PB_ALPHA_BLENDING, PB_ENABLE );
    
    /*
    PbPrimSpriteTexture( p_texture_font, 
                          0<<4,  0<<4,   0<<4,   0<<4, 
                         1024<<4, 128<<4, 1024<<4, 128<<4, 2, 0x80808080  );

    PbPrimSpriteTexture( p_texture_bg1, 
                          0<<4,  0<<4,   0<<4,   0<<4, 
                         256<<4, 256<<4, 256<<4, 256<<4, 3, 0x80808080  );
    */
    RotateVertexZ(&VERTEXLIST_square[0], &VERTEXLIST_square_Trans[0], rotAngle%359);
    RotateVertexZ(&VERTEXLIST_square[1], &VERTEXLIST_square_Trans[1], rotAngle%359);
    RotateVertexZ(&VERTEXLIST_square[2], &VERTEXLIST_square_Trans[2], rotAngle%359);
    RotateVertexZ(&VERTEXLIST_square[3], &VERTEXLIST_square_Trans[3], rotAngle%359);

    PbPrimQuadTextureGouraud(p_texture_bg1,
                               (offx+(int)(VERTEXLIST_square_Trans[0].x*xscale*scale[0]))<<4, (offy+(int)(VERTEXLIST_square_Trans[0].y*yscale*scale[0]))<<4,
                               0<<4, 0<<4,
                               (offx+(int)(VERTEXLIST_square_Trans[1].x*xscale*scale[0]))<<4, (offy+(int)(VERTEXLIST_square_Trans[1].y*yscale*scale[0]))<<4,
                               0<<4, 256<<4, 
                               (offx+(int)(VERTEXLIST_square_Trans[2].x*xscale*scale[0]))<<4, (offy+(int)(VERTEXLIST_square_Trans[2].y*yscale*scale[0]))<<4,
                               256<<4, 0<<4, 
                               (offx+(int)(VERTEXLIST_square_Trans[3].x*xscale*scale[0]))<<4, (offy+(int)(VERTEXLIST_square_Trans[3].y*yscale*scale[0]))<<4,
                               256<<4, 256<<4,
                               3,
                               0x20808080, 0x20808080, 0x20808080, 0x20808080);


    RotateVertexZ(&VERTEXLIST_square[0], &VERTEXLIST_square_Trans[0], ABS((359-rotAngle)%359));
    RotateVertexZ(&VERTEXLIST_square[1], &VERTEXLIST_square_Trans[1], ABS((359-rotAngle)%359));
    RotateVertexZ(&VERTEXLIST_square[2], &VERTEXLIST_square_Trans[2], ABS((359-rotAngle)%359));
    RotateVertexZ(&VERTEXLIST_square[3], &VERTEXLIST_square_Trans[3], ABS((359-rotAngle)%359));

    PbPrimQuadTextureGouraud(p_texture_bg2,
                               (offx+(int)(VERTEXLIST_square_Trans[0].x*xscale*scale[1]))<<4, (offy+(int)(VERTEXLIST_square_Trans[0].y*yscale*scale[1]))<<4,
                               0<<4, 256<<4,
                               (offx+(int)(VERTEXLIST_square_Trans[1].x*xscale*scale[1]))<<4, (offy+(int)(VERTEXLIST_square_Trans[1].y*yscale*scale[1]))<<4,
                               0<<4, 0<<4, 
                               (offx+(int)(VERTEXLIST_square_Trans[2].x*xscale*scale[1]))<<4, (offy+(int)(VERTEXLIST_square_Trans[2].y*yscale*scale[1]))<<4,
                               256<<4, 256<<4, 
                               (offx+(int)(VERTEXLIST_square_Trans[3].x*xscale*scale[1]))<<4, (offy+(int)(VERTEXLIST_square_Trans[3].y*yscale*scale[1]))<<4,
                               256<<4, 0<<4,
                               4,
                               0x20808080, 0x20808080, 0x20808080, 0x20808080);

    DrawScroller();

    PbPrimSpriteTexture( p_texture_logo, 
                        -30<<4, 3<<4,  
						0<<4, 0<<4, 
                        226<<4, 259<<4,
						256<<4, 256<<4,
						100, 0x80808080 );
    
    PbPrimSetState( PB_ALPHA_BLENDING, PB_DISABLE );

    ///////////////////////////////////////////////////////////////////////////
    // Sync and flipscreen


    if (rotAngle<358)
    {
        rotAngle++;
    }else
    {
        rotAngle=0;
    }    

    for (i=0; i<2; i++)
    {
        if (scaleflag[i])
        {
            if (scale[i]<scalemax)
            {
                scale[i]++;
            }else{
                scaleflag[i] = 0;
            }
        }else{
            if (scale[i]>scalemin)
            {
                scale[i]--;
            }else{
                scaleflag[i] = 1;
            }
        }
    }   

    #ifdef DEBUGMODE
        setBGColorRGB(0x0,0,0);
    #endif    
    PbScreenSyncFlip();
    //printf("a: %d\n",rotAngle);
  }
  
  return pInfo->screen_mode;
}
