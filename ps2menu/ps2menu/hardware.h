// hw.h

#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#ifdef __cplusplus
extern "C" {
#endif


#include	"ps2.h"

extern	u64		DMABuffer[];
extern	PU8		pScreen;
extern	int		g_nScreen_X;		// These can go UP TO 512
extern	int		g_nScreen_Y;
extern	int		g_nFiltered;		// set to 1 to bi-linear filter the screen

extern	int		g_nDisplayWidth;	// These are used to pick a screen mode
extern	int		g_nDisplayHeight;

extern	int		g_nClearScreen;		// set to 1 to clear screen after FLIP
extern	U32		g_nClearColour;		// clear screen colour


void	ClearScreen( U32 ARGB );
void	SetupScreen( int nDisplayBuffer );
void	RenderQuad2( void );
void	RenderQuad( int xx );

void	UploadImage24( U32 *src, U32 Xsize, U32 Ysize, U32 VAdd );
void	UploadPalette( void* Address, U32 Xsize, U32 Ysize, U32 VAdd );
void	UploadScreen( U32 Xsize, U32 Ysize, U32 VAdd );
void	UpdateScreen( void );
void	SetPaletteEntry( U32 ARGB, U32 index );
int     Load( char* pszName, PU8 pBuffer );
void 	LoadSoundModules(void);









#ifdef __cplusplus
}
#endif

#endif
