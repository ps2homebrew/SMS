/*========================================================================
==				AltimitGS.cpp handles GSlib init, font and texture		==
==				(c) 2004 t0mb0la (tomhawcroft@comcast.net)				==
== Refer to the file LICENSE in the main folder for license information	==
========================================================================*/

#include "altimit.h"

//static int fileMode =  FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP | FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH;

gsFontTex* fontTex;
//IIF* buttonTex;
altimitGS altGS;
gsDriver altGsDriver;
gsFont altFont;

extern char elfloadpath[MAX_PATHNAME];
extern char loadpath[MAX_PATHNAME];
extern int pointerX, pointerY;
extern int usepointer;
extern int FONT_WIDTH;
extern u8 *lucida_fnt;
extern int size_lucida_fnt;
//extern u8 *buttons_iif;
//extern int size_buttons_iif;

gsFontTex *expandfont_psm8_psm32(gsFontTex *font8)
{
 int i, j, width, height;
 gsFontTex *font32;
 u8 *fontptr, *fontptr2;

 width = font8->TexWidth;
 height = font8->TexHeight;
 font32 = (gsFontTex *)memalign(64, (width*height)*4 + 0x120);
 if (font32)
 {
	memcpy(font32, font8, 0x120);
	font32->PSM = 0;
	fontptr=(u8 *)font32+0x120;
	fontptr2=(u8 *)font8+0x120;
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			*fontptr++ = 255; *fontptr++ = 255; *fontptr++ = 255;
			*fontptr++ = *fontptr2++;
		}
	}
	return (font32);
 }
 return NULL;
}

//u8 *buttonsiif = (u8 *)&buttons_iif;
//IIF *buttonTex = (IIF *)buttonsiif;
////////////////////////////////////////////////////////////////////////
// initialises graphics mode, loads font and texture from program folder
void initGS()
{
// int tex_width, tex_height;
 int i; //fdir, size, i;

// altGsDriver = gsDriver altGsDrive;
// altFont = gsFont altFon;

 pointerX = altGS.WIDTH/2;
 pointerY = altGS.HEIGHT/2;
 altGsDriver.setDisplayMode(altGS.WIDTH, altGS.HEIGHT, altGS.OFFSETX, altGS.OFFSETY,
		GS_PSMCT32, 2, altGS.PALORNTSC, altGS.INTERLACING, GS_ENABLE, GS_PSMZ32);	
 altFont.assignPipe(&altGsDriver.drawPipe);
 altGsDriver.drawPipe.setAlphaEnable(GS_ENABLE);
 altGsDriver.drawPipe.setDither(GS_ENABLE);
// strcpy(loadpath,elfloadpath);		// *** do need a default font, perhaps better
							// *** linked in to elf at compile time
							// *** instead of looking in the program folder
// strcat(loadpath,"lucida.fnt");		// *** this should be user definable
// if ((fdir = fileXioOpen(loadpath, O_RDONLY, fileMode)) < 0) scr_printf("Failed to open font %s\n", loadpath);
// else
// {
//	size = size_lucida_fnt; //fileXioLseek(fdir, 0, SEEK_END);
	fontTex = expandfont_psm8_psm32((gsFontTex *)&lucida_fnt); //memalign(64,size);
	if (fontTex)
	{
//		fileXioLseek(fdir, 0, SEEK_SET);
//		fileXioRead(fdir, (unsigned char *)fontTex, size);
//		fileXioClose(fdir);
		FONT_WIDTH = 0;
		for (i=32; i<128; i++) if (fontTex->CharWidth[i] > FONT_WIDTH) FONT_WIDTH = fontTex->CharWidth[i];
		altFont.uploadFont(fontTex, altGsDriver.getTextureBufferBase(), fontTex->TexWidth, 0, 0);
		free(fontTex);
	}
//	else fileXioClose(fdir);
// }
// strcpy(loadpath,elfloadpath);
// strcat(loadpath,"buttons.iif");		// *** this could also be user definable
// if ((fdir = fileXioOpen(loadpath, O_RDONLY, fileMode)) < 0) scr_printf("Failed to open IIF %s\n", loadpath);
// else
// {
//	size = size_buttons_iif; //fileXioLseek(fdir, 0, SEEK_END);
//x	buttonTex = (IIF *)&buttons_iif; //memalign(64,size);	// *** and some error checking here
//x	if (buttonTex)
//x	{
//		fileXioLseek(fdir, 0, SEEK_SET);
//		fileXioRead(fdir, (unsigned char *)buttonTex, size);
//		fileXioClose(fdir);
//x		tex_width=buttonTex->width;
//x		tex_height=buttonTex->height;
//x		altGsDriver.drawPipe.TextureUpload(altGsDriver.getTextureBufferBase(), 256, 0, 256,
//x			(int)buttonTex->psm, (const unsigned char*)&(buttonTex->pixel_data), tex_width, tex_height); // gslib 0.51
//		altGsDriver.drawPipe.TextureUpload(altGsDriver.getTextureBufferBase(), 256, 0, 256,
//			(int)buttonTex->psm, &(buttonTex->pixel_data), tex_width, tex_height); // gslib 0.5
//		free(buttonTex);
//	}
//	else fileXioClose(fdir);
//x }
}

////////////////////////////////////////////////////////////////////////
// draws a pointer, a triangle with a tail :P
void drawPointer()
{
 if (usepointer)
 {
	altGsDriver.drawPipe.Line(pointerX, pointerY, pointerX+12, pointerY+16,
		10, altGS.POINTERCOL);
	altGsDriver.drawPipe.TriangleFlat(pointerX, pointerY, 10,
		pointerX+16, pointerY+8, 10,
		pointerX, pointerY+12, 10, altGS.POINTERCOL);
 }
}
