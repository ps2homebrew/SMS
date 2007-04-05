/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_DXSB_H
# define __SMS_DXSB_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

# ifndef __SMS_RingBuffer_H
#  include "SMS_RingBuffer.h"
# endif  /* __SMS_RingBuffer_H */

# ifndef __SMS_DMA_H
#  include "SMS_DMA.h"
# endif  /* __SMS_DMA_H */

# ifndef __SMS_GS_H
#  include "SMS_GS.h"
# endif  /* __SMS_GS_H */

typedef struct SMS_DXSBDrawPack {
 DMATag         m_DMATagUpload;
 unsigned long  m_Pad0;
 GIFTag         m_GIFTagUpload;
 GSRegBITBLTBUF m_BitBlt;
 unsigned long  m_BitBltID;
 GSRegTRXPOS    m_TrxPos;
 unsigned long  m_TrxPosID;
 GSRegTRXREG    m_TrxReg;
 unsigned long  m_TrxRegID;
 GSRegTRXDIR    m_TrxDir;
 unsigned long  m_TrxDirID;
 GIFTag         m_GIFTagData;
 DMATag         m_DMATagData;
 unsigned long  m_Pad1;
 DMATag         m_DMATagDraw;
 unsigned long  m_Pad2;
 GIFTag         m_GIFTagDraw0;
 GSRegTEXFLUSH  m_TexFlush;
 unsigned long  m_TexFlushID;
 GSRegPRIM      m_Prim;
 unsigned long  m_PrimID;
 GSRegTEST      m_TestOn;
 unsigned long  m_TestOnID;
 GIFTag         m_GIFTagDraw1;
 GSRegRGBAQ     m_RGBAQ;
 GSRegTEX0      m_Tex0;
 GSRegUV        m_UVLeftTop;
 GSRegXYZ       m_XYZLeftTop;
 GSRegUV        m_UVRightBottom;
 GSRegXYZ       m_XYZRightBottom;
 GIFTag         m_GIFTagDraw2;
 GSRegTEST      m_TestOff;
 unsigned long  m_TestOffID;
} SMS_DXSBDrawPack __attribute__(   (  aligned( 64 )  )   );

typedef struct SMS_DXSBErasePack {
 GIFTag         m_GIFTag;
 GSRegTEST      m_TestOff;
 unsigned long  m_TestOffID;
 GSRegPRIM      m_Prim;
 unsigned long  m_PrimID;
 GSRegRGBAQ     m_RGBAQ;
 unsigned long  m_RGBAQID;
 GSRegXYZ       m_XYZLeftTop;
 unsigned long  m_XYZLeftTopID;
 GSRegXYZ       m_XYZRightBottom;
 unsigned long  m_XYZRightBottomID;
 GSRegTEST      m_TestOn;
 unsigned long  m_TestOnID;
} SMS_DXSBErasePack __attribute__(   ( aligned( 64 )  )   );

# define SMS_DXSB_DP_BB_VRAM( p ) (   *( unsigned short* )(  ( p ) +  36  )   )
# define SMS_DXSB_DP_BB_TBW( p )  (   *( unsigned char*  )(  ( p ) +  38  )   )
# define SMS_DXSB_DP_TX_W( p )    (   *( unsigned short* )(  ( p ) +  64  )   )
# define SMS_DXSB_DP_TX_H( p )    (   *( unsigned short* )(  ( p ) +  68  )   )
# define SMS_DXSB_DP_QWC_GIF( p ) (   *( unsigned short* )(  ( p ) +  96  )   )
# define SMS_DXSB_DP_QWC_DMA( p ) (   *( unsigned short* )(  ( p ) + 112  )   )
# define SMS_DXSB_DP_PTR_DMA( p ) (   *( unsigned int*   )(  ( p ) + 116  )   )
# define SMS_DXSB_DP_TEX0( p )    (   *( unsigned long*  )(  ( p ) + 232  )   )
# define SMS_DXSB_DP_XYZ_LT( p )  (   *( unsigned long*  )(  ( p ) + 248  )   )
# define SMS_DXSB_DP_UV_R( p )    (   *( unsigned short* )(  ( p ) + 256  )   )
# define SMS_DXSB_DP_UV_B( p )    (   *( unsigned short* )(  ( p ) + 258  )   )
# define SMS_DXSB_DP_XYZ_RB( p )  (   *( unsigned long*  )(  ( p ) + 264  )   )

# define SMS_DXSB_EP_XYZ_LTE( p ) (   *( unsigned long*  )(  ( p ) + 64   )   )
# define SMS_DXSB_EP_XYZ_RBE( p ) (   *( unsigned long*  )(  ( p ) + 80   )   )

typedef struct SMS_DXSBFrame {

 long           m_StartPTS;
 long           m_EndPTS;
 unsigned char* m_pPixmap;
 unsigned short m_Width;
 unsigned short m_Height;
 int            m_FrameType;
 float          m_Left;
 float          m_Top;
 float          m_Right;
 float          m_Bottom;
 unsigned short m_QWCPixmap;
 unsigned short m_RWidth;
 unsigned short m_RHeight;
 char           m_Pad[ 14 ];

} SMS_DXSBFrame;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void  SMS_DXSB_Init   ( int, int, int*                 );
int   SMS_DXSB_Decode ( SMS_AVPacket*, SMS_RingBuffer* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_DXSB_H */
