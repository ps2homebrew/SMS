#ifndef _WATER_
#define _WATER_
#include "PbVu0m.h"

#define GRID_SIZE_X  32
#define GRID_SIZE_Y  32

#define CLIP_NEAR  0.0
#define CLIP_FAR   1000.0

#define REFMAP_SCALER 2.0f
#define REFMAP_DEFAULT_X 0.5f
#define REFMAP_DEFAULT_Y 0.5f

#define RIPPLE_DAMPEN     4.0
#define RIPPLE_LENGTH     2048
#define RIPPLE_CYCLES     10
#define RIPPLE_AMPLITUDE  0.5f 
#define RIPPLE_STEP	  20 
#define RIPPLE_COUNT	  7 

#define OPTION_REFLECTION 1
#define OPTION_RIPPLE 2
#define OPTION_QUALITY 4
#define OPTION_IGNOREKEYS 8
#define OPTION_BUFFER 16
#define OPTION_MULTITEXTURE 32

extern int user_options;

void water_init();
void water_tick(PbTexture *texture,PbTexture *textureenv);
void ripple_init(void);
void do_ripple(int x, int y, int frame);
void ripple_dynamics(void);
void water_draw_256(PbTexture *texture,PbTexture *textureenv);
void water_primsetup_solid(PbTexture* pTex);
void water_primsetup_blend( PbTexture* pTex, u64 alpha);
void water_giflist_packed_bg(PbFvec *texmult);
void water_giflist_packed_refl(PbFvec *texmult );

#endif
