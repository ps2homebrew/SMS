/*
 * scroller.h - simple horizontal scroller, uses Pb demo library
 *
 * Copyright (c) 2004   raizor <raizor@c0der.net>
 *
 */

void Scroller_init(char* stxt, unsigned short *lookup,
				   PbTexture *p_tex, unsigned short y);

void Scroller_tick();