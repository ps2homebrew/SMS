/*========================================================================
==				AltimitGUI.cpp handles basic graphics GUI				==
==				(c) 2004 t0mb0la (tomhawcroft@comcast.net)				==
== Refer to the file LICENSE in the main folder for license information	==
========================================================================*/

#include "altimit.h"

extern gsDriver altGsDriver;
extern gsFont altFont;
extern int FONT_WIDTH;
extern altimitGS altGS;

////////////////////////////////////////////////////////////////////////
// draws a simple tooltip window
// a box, some text and a border
void drawTooltipWindow(TooltipWindow tooltip)
{
 int Xsize, Ysize;

 Xsize = strlen(tooltip.Content)*FONT_WIDTH;
 Ysize = FONT_HEIGHT;
 altGsDriver.drawPipe.RectFlat(tooltip.Xpos, tooltip.Ypos,
	tooltip.Xpos+Xsize, tooltip.Ypos+Ysize, tooltip.Zpos, tooltip.Background);
 altFont.Print(tooltip.Xpos, tooltip.Xpos+Xsize, tooltip.Ypos, tooltip.Zpos+1,
	tooltip.Foreground, GSFONT_ALIGN_CENTRE, tooltip.Content);
 altGsDriver.drawPipe.RectLine(tooltip.Xpos, tooltip.Ypos,
	tooltip.Xpos+Xsize, tooltip.Ypos+Ysize, tooltip.Zpos+2, tooltip.Foreground);
}

void drawFunctionWindow(FunctionWindow functions)
{
 int Xsize, Ysize, totallines, maxwidth, curwidth, i, j;
 char *textptr;
 char parse;
 char textline[20];

 maxwidth = 0;
 curwidth = 0;
 totallines = 0;
 textptr = functions.Content;
 while((parse = *textptr++) != '\0')
 {
	if (parse == '\n')
	{
		totallines++;
		if (curwidth > maxwidth) maxwidth = curwidth;
		curwidth = 0;
	}
	else curwidth++;
 }
 Xsize = maxwidth * (FONT_WIDTH + 2);
 Ysize = totallines * FONT_HEIGHT;
 altGsDriver.drawPipe.RectFlat(functions.Xpos, functions.Ypos,
	functions.Xpos+Xsize, functions.Ypos+Ysize,
	functions.Zpos, functions.Background);
 textptr = functions.Content;
 for (i = 0; i < totallines; i++)
 {
	j = 0;
	while((parse = *textptr++) != '\n')
	{
		if (parse == '\0') break;
		if (j<20) { textline[j] = parse; j++; } // hopefully force to fit :P
	}
	textline[j] = '\0';
	if (j > 0) 
	{
		if (i == functions.Highlighted)
		{
			altGsDriver.drawPipe.RectFlat(functions.Xpos, functions.Ypos+(i*FONT_HEIGHT)+1
				, functions.Xpos+Xsize, functions.Ypos+(i*FONT_HEIGHT)+FONT_HEIGHT
				, functions.Zpos+1, functions.Foreground);
			altFont.Print(functions.Xpos, functions.Xpos+Xsize,
				functions.Ypos+(i*FONT_HEIGHT), functions.Zpos+2, functions.Background,
			GSFONT_ALIGN_CENTRE, textline);
		}
		else
		{
			altFont.Print(functions.Xpos, (functions.Xpos+Xsize),
				functions.Ypos+(i*FONT_HEIGHT), functions.Zpos+1, functions.Foreground,
				GSFONT_ALIGN_CENTRE, textline);
		}
	}
 }
 altGsDriver.drawPipe.RectLine(functions.Xpos-1, functions.Ypos-1,
	functions.Xpos+Xsize+1, functions.Ypos+Ysize, functions.Zpos+2, functions.Foreground);
}

////////////////////////////////////////////////////////////////////////
// draw a simple text window
// a window, a scrollbar and the text content. scrolling is handled by
// the calling module, along with the buttons
void drawTextWindow(TextWindow textbox)
{
 int totallines, titlesize, maxlines, maxchars, i, j;
 int scrollX1, scrollY1, scrollX2, scrollY2;
 char *textptr;
 char parse;
 char textline[80];

 maxlines = (textbox.Ysize - FONT_HEIGHT)/FONT_HEIGHT;
 maxchars = (textbox.Xsize - (FONT_WIDTH*2))/FONT_WIDTH;
 titlesize = maxchars * FONT_WIDTH;
 totallines = 0;
 textptr = textbox.Content;
 while((parse = *textptr++) != '\0')
 {
	if (parse == '\n') totallines++;
 }
 if (totallines >= maxlines)
 {
	scrollX1 = (textbox.Xpos+textbox.Xsize)-(FONT_WIDTH*2)+1;
	scrollY1 = (((((textbox.Top-1) * 100) + (totallines / 2)) / totallines)
			* (textbox.Ysize-(3*FONT_HEIGHT)) / 100)
			+ (textbox.Ypos+(2*FONT_HEIGHT));
	scrollX2 = textbox.Xpos+textbox.Xsize-1;
	scrollY2 = (((maxlines * 100) + (totallines / 2)) / totallines)
			* (textbox.Ysize-(3*FONT_HEIGHT)) / 100;
 }
 else
 {
	scrollX1 = (textbox.Xpos+textbox.Xsize)-(FONT_WIDTH*2)+1;
	scrollY1 = textbox.Ypos+(FONT_HEIGHT*2);
	scrollX2 = textbox.Xpos+textbox.Xsize-1;
	scrollY2 = textbox.Ysize-(FONT_HEIGHT*3);
	maxlines = totallines;
 }
 altGsDriver.drawPipe.RectFlat(textbox.Xpos, textbox.Ypos,
	textbox.Xpos+textbox.Xsize, textbox.Ypos+textbox.Ysize,
	textbox.Zpos, textbox.Background);
 altGsDriver.drawPipe.RectFlat(textbox.Xpos+1, textbox.Ypos+1,
	textbox.Xpos+(textbox.Xsize-(FONT_WIDTH*2))-2, textbox.Ypos+(FONT_HEIGHT),
	textbox.Zpos+1, textbox.Backhead);
 altFont.Print(textbox.Xpos, textbox.Xpos+titlesize, textbox.Ypos, textbox.Zpos+2,
	textbox.Forehead, GSFONT_ALIGN_LEFT, textbox.Title);
 textptr = textbox.Content;
 for(i=1;i<textbox.Top;i++) while((parse = *textptr++) != '\n');
 for(i=1;i<=maxlines;i++)
 {
	j = 0;
	while((parse = *textptr++) != '\n')
	{
		if (parse == '\0') break;
		if (j<maxchars) { textline[j] = parse; j++; } // hopefully force to fit :P
	}
	textline[j] = '\0';
	if (j>0) altFont.Print(textbox.Xpos, (textbox.Xpos+textbox.Xsize)-FONT_WIDTH,
		textbox.Ypos+(i*FONT_HEIGHT), textbox.Zpos+1, textbox.Foreground,
		GSFONT_ALIGN_LEFT, textline);
 }
 altGsDriver.drawPipe.RectLine(textbox.Xpos, textbox.Ypos,
	textbox.Xpos+textbox.Xsize, textbox.Ypos+textbox.Ysize,
	textbox.Zpos+3, textbox.Foreground);
 altGsDriver.drawPipe.RectLine(textbox.Xpos, textbox.Ypos+FONT_HEIGHT,
	(textbox.Xpos+textbox.Xsize)-(FONT_WIDTH*2), textbox.Ypos+textbox.Ysize,
	textbox.Zpos+3, textbox.Foreground);
 altGsDriver.drawPipe.RectFlat(scrollX1, scrollY1, scrollX2, scrollY1+scrollY2, textbox.Zpos+2,
	textbox.Foreground);
 altGsDriver.drawPipe.Line(scrollX1, scrollY1, scrollX2, scrollY1, textbox.Zpos+3,
	GS_SET_RGBA(0xFF,0xFF,0xFF,0xFF));
 altGsDriver.drawPipe.Line(scrollX1, scrollY1, scrollX1, scrollY1+scrollY2, textbox.Zpos+3,
	GS_SET_RGBA(0xFF,0xFF,0xFF,0xFF));
 altGsDriver.drawPipe.Line(scrollX2, scrollY1, scrollX2, scrollY1+scrollY2, textbox.Zpos+3,
	GS_SET_RGBA(0x00,0x00,0x00,0xFF));
 altGsDriver.drawPipe.Line(scrollX1, scrollY1+scrollY2, scrollX2, scrollY1+scrollY2, textbox.Zpos+3,
	GS_SET_RGBA(0x00,0x00,0x00,0xFF));
}

////////////////////////////////////////////////////////////////////////
// draws a file browser window, with a highlight bar
// scrolling/selection is handled by
// the calling module, along with the buttons
void drawFilelistWindow(FilelistWindow textbox)
{
 int totallines, titlesize, maxlines, maxchars, i, j;
 int scrollX1, scrollY1, scrollX2, scrollY2;
 char fileline[MAX_FILENAME];
 char textline[80];

 maxlines = (textbox.Ysize - FONT_HEIGHT)/FONT_HEIGHT;
 maxchars = (textbox.Xsize - (FONT_WIDTH*2))/FONT_WIDTH;
 titlesize = maxchars * FONT_WIDTH;
 totallines = 0;
 i = 0;
 while(i<MAX_ENTRIES)
 {
	if (strcmp(textbox.Content[i].filename,"\0")) totallines++;
	else break;
	i++;
 }
 if (totallines >= maxlines)
 {
	scrollX1 = (textbox.Xpos+textbox.Xsize)-(FONT_WIDTH*2)+1;
	scrollY1 = (((((textbox.Top) * 100) + (totallines / 2)) / totallines)
			* (textbox.Ysize-(3*FONT_HEIGHT)) / 100)
			+ (textbox.Ypos+(2*FONT_HEIGHT));
	scrollX2 = textbox.Xpos+textbox.Xsize-1;
	scrollY2 = (((maxlines * 100) + (totallines / 2)) / totallines)
			* (textbox.Ysize-(3*FONT_HEIGHT)) / 100;
 }
 else
 {
	scrollX1 = (textbox.Xpos+textbox.Xsize)-(FONT_WIDTH*2)+1;
	scrollY1 = textbox.Ypos+(FONT_HEIGHT*2);
	scrollX2 = textbox.Xpos+textbox.Xsize-1;
	scrollY2 = textbox.Ysize-(FONT_HEIGHT*3);
 }
 altGsDriver.drawPipe.RectFlat(textbox.Xpos, textbox.Ypos,
	textbox.Xpos+textbox.Xsize, textbox.Ypos+textbox.Ysize,
	textbox.Zpos, textbox.Background);
 altGsDriver.drawPipe.RectFlat(textbox.Xpos+1, textbox.Ypos+1,
	textbox.Xpos+(textbox.Xsize-(FONT_WIDTH*2))-2, textbox.Ypos+(FONT_HEIGHT),
	textbox.Zpos+1, textbox.Backhead);
 altFont.Print(textbox.Xpos, textbox.Xpos+titlesize, textbox.Ypos, textbox.Zpos+2,
	textbox.Forehead, GSFONT_ALIGN_LEFT, textbox.Title);
 j = textbox.Top;
 i = 1;
 while(strcmp(textbox.Content[j].filename, "\0") && i <= maxlines)
 {
	if (textbox.Content[j].mode == FIO_S_IFDIR)
		snprintf (fileline, MAX_FILENAME-1, "%s (DIR)", textbox.Content[j].filename);
	else
		snprintf (fileline, MAX_FILENAME-1, "%s [%d bytes]", textbox.Content[j].filename, textbox.Content[j].size);
	snprintf (textline, maxchars, "%s", fileline);
	if (j==textbox.Highlighted)
	{
		altGsDriver.drawPipe.RectFlat(textbox.Xpos, textbox.Ypos+(i*FONT_HEIGHT)+1
			, textbox.Xpos+(maxchars*FONT_WIDTH), textbox.Ypos+(i*FONT_HEIGHT)+FONT_HEIGHT
			, textbox.Zpos+1, textbox.Foreground);
		altFont.Print(textbox.Xpos, (textbox.Xpos+textbox.Xsize)-FONT_WIDTH,
			textbox.Ypos+(i*FONT_HEIGHT), textbox.Zpos+2, textbox.Background,
			GSFONT_ALIGN_LEFT, textline);
	}
	else altFont.Print(textbox.Xpos, (textbox.Xpos+textbox.Xsize)-FONT_WIDTH,
		textbox.Ypos+(i*FONT_HEIGHT), textbox.Zpos+1, textbox.Foreground,
		GSFONT_ALIGN_LEFT, textline);
	i++;
	j++;
 }
 altGsDriver.drawPipe.RectLine(textbox.Xpos, textbox.Ypos,
	textbox.Xpos+textbox.Xsize, textbox.Ypos+textbox.Ysize,
	textbox.Zpos+3, textbox.Foreground);
 altGsDriver.drawPipe.RectLine(textbox.Xpos, textbox.Ypos+FONT_HEIGHT,
	(textbox.Xpos+textbox.Xsize)-(FONT_WIDTH*2), textbox.Ypos+textbox.Ysize,
	textbox.Zpos+3, textbox.Foreground);
 altGsDriver.drawPipe.RectFlat(scrollX1, scrollY1, scrollX2, scrollY1+scrollY2, textbox.Zpos+2,
	textbox.Foreground);
 altGsDriver.drawPipe.Line(scrollX1, scrollY1, scrollX2, scrollY1, textbox.Zpos+3,
	GS_SET_RGBA(0xFF,0xFF,0xFF,0xFF));
 altGsDriver.drawPipe.Line(scrollX1, scrollY1, scrollX1, scrollY1+scrollY2, textbox.Zpos+3,
	GS_SET_RGBA(0xFF,0xFF,0xFF,0xFF));
 altGsDriver.drawPipe.Line(scrollX2, scrollY1, scrollX2, scrollY1+scrollY2, textbox.Zpos+3,
	GS_SET_RGBA(0x00,0x00,0x00,0xFF));
 altGsDriver.drawPipe.Line(scrollX1, scrollY1+scrollY2, scrollX2, scrollY1+scrollY2, textbox.Zpos+3,
	GS_SET_RGBA(0x00,0x00,0x00,0xFF));
}

void drawPercentWindow(PercentWindow percentbox, char *operation)
{
 int pctval, pctbarX, Xsize, Ysize, maxchars;
 char titletxt[80];
 char percenttxt[8];

 maxchars = percentbox.Xsize / FONT_WIDTH;
 Xsize = percentbox.Xsize;
 Ysize = FONT_HEIGHT * 2;
 pctval = ((percentbox.done * 100) + (percentbox.total / 2)) / percentbox.total;
 pctbarX = (pctval * percentbox.Xsize) / 100;
 altGsDriver.drawPipe.RectFlat(percentbox.Xpos, percentbox.Ypos,
	percentbox.Xpos+Xsize, percentbox.Ypos+Ysize, percentbox.Zpos, 
	percentbox.Background);
 snprintf(titletxt, maxchars, "%s %s", operation, percentbox.Title);
 altFont.Print(percentbox.Xpos, percentbox.Xpos+Xsize, percentbox.Ypos, percentbox.Zpos+1,
	percentbox.Foreground, GSFONT_ALIGN_CENTRE, titletxt);
 snprintf(percenttxt, 8, "%d%%", pctval);
 altFont.Print(percentbox.Xpos, percentbox.Xpos+Xsize, percentbox.Ypos+FONT_HEIGHT,
	percentbox.Zpos+1, percentbox.Backhead, GSFONT_ALIGN_CENTRE, percenttxt);
 altGsDriver.drawPipe.RectFlat(percentbox.Xpos, percentbox.Ypos+FONT_HEIGHT,
	percentbox.Xpos+pctbarX, percentbox.Ypos+(FONT_HEIGHT*2), percentbox.Zpos+2, 
	percentbox.Forehead);
 altGsDriver.drawPipe.RectLine(percentbox.Xpos, percentbox.Ypos,
	percentbox.Xpos+Xsize, percentbox.Ypos+Ysize, percentbox.Zpos+2, percentbox.Foreground);
}

////////////////////////////////////////////////////////////////////////
// adds buttons to a window, just draws them really
// black and white are added to create a 3d appearance
// should probably use a better method
void drawButtons(int drawbuttons[][5], char drawtips[][30], int max)
{
 int i;

 i=0;
 while (drawbuttons[i][0] != -1)
 {
	if (max > 0 && i > max) break;
	altGsDriver.drawPipe.RectFlat(drawbuttons[i][0], drawbuttons[i][1],
		drawbuttons[i][2], drawbuttons[i][3], 4, altGS.WINFORECOL);
	altGsDriver.drawPipe.Line(drawbuttons[i][0]+1, drawbuttons[i][1]+1,
		drawbuttons[i][2]-1, drawbuttons[i][1]+1, 5, GS_SET_RGBA(0xFF,0xFF,0xFF,0xFF));
	altGsDriver.drawPipe.Line(drawbuttons[i][0]+1, drawbuttons[i][1]+1,
		drawbuttons[i][0]+1, drawbuttons[i][3]-1, 5, GS_SET_RGBA(0xFF,0xFF,0xFF,0xFF));
	altGsDriver.drawPipe.Line(drawbuttons[i][2]-1, drawbuttons[i][1]+1,
		drawbuttons[i][2]-1, drawbuttons[i][3]-1, 5, GS_SET_RGBA(0x00,0x00,0x00,0xFF));
	altGsDriver.drawPipe.Line(drawbuttons[i][0]+1, drawbuttons[i][3]-1,
		drawbuttons[i][2]-1, drawbuttons[i][3]-1, 5, GS_SET_RGBA(0x00,0x00,0x00,0xFF));
	altFont.Print(drawbuttons[i][0], drawbuttons[i][2], drawbuttons[i][1], 5,
		altGS.WINBACKCOL, GSFONT_ALIGN_CENTRE, drawtips[i]);
	i++;
 }
}

////////////////////////////////////////////////////////////////////////
// highlights a button, by replacing black lines with white lines
// and an alternate color for the label. very naughty, just like above
void drawActiveButton(int drawbuttons[][5], char drawtips[][30], int activebutton)
{
	altGsDriver.drawPipe.Line(drawbuttons[activebutton][2]-1, drawbuttons[activebutton][1]+1,
		drawbuttons[activebutton][2]-1, drawbuttons[activebutton][3]-1, 6, GS_SET_RGBA(0xFF,0xFF,0xFF,0xFF));
	altGsDriver.drawPipe.Line(drawbuttons[activebutton][0]+1, drawbuttons[activebutton][3]-1,
		drawbuttons[activebutton][2]-1, drawbuttons[activebutton][3]-1, 6, GS_SET_RGBA(0xFF,0xFF,0xFF,0xFF));
	altFont.Print(drawbuttons[activebutton][0], drawbuttons[activebutton][2], drawbuttons[activebutton][1], 6,
		altGS.WINBACKHEAD, GSFONT_ALIGN_CENTRE, drawtips[activebutton]);
}
