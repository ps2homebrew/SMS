/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
	vid_ps2.c -- PlayStation 2 video driver
	
	by Nicolas Plourde a.k.a nic067 <nicolasplourde@hotmail.com>
	
	See http://www.ps2dev.org for all your ps2 coding need.
*/

#include "quakedef.h"
#include "d_local.h"

#include "ps2_gs.h"

viddef_t	vid;				// global video state

static int verbose=0;
int ignorenext;

#ifdef _NTSC
#define	BASEWIDTH	640
#define	BASEHEIGHT  224
#else
#define	BASEWIDTH	640
#define	BASEHEIGHT  480
#endif

byte	*vid_buffer;
long	zbuffer;
void	*surfcache;
int		surfcachesize;

static long highhunkmark;
static long buffersize;

unsigned short	d_8to16table[256];

static uint32 tmp[BASEWIDTH*BASEHEIGHT];

typedef struct
{
	char red;
	char green;
	char blue;
}RGB;

RGB myPalette[256];

void ResetFrameBuffer(void)
{
	//int mem;
	//int pwidth;

	if (d_pzbuffer)
	{
		D_FlushCaches ();
		Hunk_FreeToHighMark (highhunkmark);
		d_pzbuffer = NULL;
	}
	highhunkmark = Hunk_HighMark ();

// alloc an extra line in case we want to wrap, and allocate the z-buffer
	buffersize = vid.width * vid.height * sizeof (*d_pzbuffer);

	surfcachesize = D_SurfaceCacheForRes (vid.width, vid.height);

	buffersize += surfcachesize;

	d_pzbuffer = Hunk_HighAllocName (buffersize, "video");
	if (d_pzbuffer == NULL)
		Sys_Error ("Not enough memory for video mode\n");

	surfcache = (byte *) d_pzbuffer + vid.width * vid.height * sizeof (*d_pzbuffer);

	D_InitCaches(surfcache, surfcachesize);

	vid.buffer = vid_buffer;
	vid.conbuffer = vid.buffer;
}

void VID_SetPalette (unsigned char *palette)
{
	int i;
	for (i=0 ; i<256 ; i++)
	{
		myPalette[i].red = palette[i*3] * 257;
		myPalette[i].green = palette[i*3+1] * 257;
		myPalette[i].blue = palette[i*3+2] * 257;
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
	VID_SetPalette(palette);
}

void	VID_Init (unsigned char *palette)
{
	ignorenext=0;
	vid.width = BASEWIDTH;
	vid.height = BASEHEIGHT;
	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.numpages = 2;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	verbose=COM_CheckParm("-verbose");
	
	vid_buffer = malloc(vid.width*vid.height);
	
	ResetFrameBuffer();

	vid.rowbytes = vid.width;
	vid.buffer = vid_buffer;
	vid.direct = 0;
	vid.conbuffer = vid_buffer;
	vid.conrowbytes = vid.rowbytes;
	vid.conwidth = vid.width;
	vid.conheight = vid.height;
	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);

	#ifdef _NTSC
	GS_MODE mode = {2,640,224,0,32,4,656,36};//NTSC-I (640x224))
	#else
	GS_MODE mode = {0x1C,640,480,0,32,2,356,18}; //VESA 640x480x32 75hertz
	#endif

	gs_init(mode);
	fill_rect(0, 0, BASEWIDTH, BASEHEIGHT);
}

void	VID_Shutdown (void)
{
	Con_Printf("VID_Shutdown\n");
}
	
void	VID_Update (vrect_t *rects)
{
	extern int scr_fullupdate;
	scr_fullupdate = 0;
	
	int i;
	for(i=0;i<vid.width*vid.height;i++)
	{
		tmp[i] =	((uint8)(myPalette[vid.buffer[i]].red)  << 0)   |
					((uint8)(myPalette[vid.buffer[i]].green)<< 8)   | 
					((uint8)(myPalette[vid.buffer[i]].blue) << 16)  |
					((uint8)(255)							<< 24);
	}

	put_image(0, 0, vid.width, vid.height, (uint32*)tmp);
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
}


