#ifndef _BITMAP_H
#define _BITMAP_H

#include "tamtypes.h"

typedef struct {
  int w, h;
  const u32 *data;
} BITMAP;

typedef struct {
  int w, h;
  u32 *data;
} BITMAP2;

typedef struct {
  int w, h;
  unsigned char *data;
} FONTBITMAP;

typedef struct {
  int nfonts;
  int h;
  FONTBITMAP data[256];
} FONT;

int text_len (FONT *font, char *text);
void render_bitmap (BITMAP *bitmap, int x, int y);
void render_text (FONT *font, int x, int y, char *text, unsigned int color);
void render_text_r (FONT *font, int x, int y, char *text, unsigned int color);
void render_text_c (FONT *font, int xk, int yk, int w, char *text, unsigned int color);

#endif
