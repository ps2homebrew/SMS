/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __IPU_H
# define __IPU_H

struct GSContext;
struct SMS_MacroBlock;
struct SMS_FrameBuffer;

# ifdef _WIN32

typedef struct IPUContext {

 unsigned int      m_MBWidth;
 unsigned int      m_MBHeight;
 unsigned int      m_Width;
 unsigned int      m_UVWidth;
 unsigned int      m_Height;
 unsigned int      m_UVHeight;
 unsigned int      m_Linesize;
 struct GSContext* m_pGSCtx;

 void ( *Destroy ) ( void );
 void ( *Display ) ( struct SMS_FrameBuffer* );
 void ( *Sync    ) ( void );

} IPUContext;

# else  /* PS2 */

#  define IPU_REG_CMD  (  ( volatile unsigned long int* )0x10002000  )
#  define IPU_REG_CTRL (  ( volatile unsigned int*      )0x10002010  )
#  define IPU_REG_BP   (  ( volatile unsigned int*      )0x10002020  )
#  define IPU_REG_TOP  (  ( volatile unsigned long int* )0x10002030  )
#  define IPU_FIFO_I   (  ( volatile unsigned long int* )0x10007000  )
#  define IPU_FIFO_O   (  ( volatile unsigned long int* )0x10007010  )

#  define IPU_SET_CMD( CODE, OPTION ) \
 *IPU_REG_CMD = ( u64 )(   (  ( CODE & 0x00000000F ) << 28  ) | (  ( OPTION ) & 0x0FFFFFFF  )   )

#  define IPU_CMD_BCLR( BP ) \
 IPU_SET_CMD( 0, BP & 0x7F )

#  define IPU_CMD_IDEC( FB, QSC, DTD, SGN, DTE, OFM ) \
 IPU_SET_CMD(  1, ( FB & 0x1F ) | ( QSC & 0x1F ) << 20 | ( DTD & 1 ) << 24 | ( SGN & 1 ) << 25 | ( DTE & 1 ) << 26 | ( OFM & 1 ) << 27  )

#  define IPU_CMD_FDEC( FB ) \
 IPU_SET_CMD( 4, FB & 0x3F )

#  define IPU_CMD_SETTH( TH0, TH1 ) \
 IPU_SET_CMD(    9, (  ( TH0 ) & 0xFF ) | (   (  ( TH1 ) & 0xFF  ) << 16   )    )

#  define IPU_CMD_CSC( MBC, DTE, OFM ) \
 IPU_SET_CMD(    7, (   ( MBC & 0x7FF ) | (  ( DTE & 1 ) << 26  ) | (  ( OFM & 1 ) << 27  )   )    )

#  define IPU_TOP() (  ( u32 )*IPU_REG_TOP  )
#  define IPU_CMD() (  ( u32 )*IPU_REG_CMD  )

#  define IPU_WAIT() while (  *IPU_REG_CTRL & 0x80000000 )
#  define IPU_RESET() *IPU_REG_CTRL = ( 1 << 30 ); IPU_WAIT(); IPU_CMD_BCLR( 0 );  IPU_WAIT()

typedef struct IPUContext {

 unsigned long int       m_DMAGIFDraw[ 14 ] __attribute__(   ( aligned( 16 )  )   );
 unsigned long int       m_DMAGIFTX  [  6 ] __attribute__(   ( aligned( 16 )  )   );
 unsigned int            m_DestX;
 unsigned int            m_DestY;
 unsigned int            m_Slice;
 unsigned int            m_MB;
 unsigned int            m_nMBSlice;
 unsigned int            m_nMBSlices;
 unsigned int            m_MBStride;
 unsigned int            m_QWCToIPUSlice;
 unsigned int            m_QWCFromIPUSlice;
 unsigned int            m_DMAHandlerID_IPU;
 unsigned int            m_DMAHandlerID_GIF;
# ifdef VB_SYNC
 unsigned int            m_VBlankStartHandlerID;
 unsigned int            m_VBlankEndHandlerID;
 unsigned int            m_fDraw;
 unsigned int            m_fBlank;
# endif  /* VB_SYNC */
 unsigned int            m_VRAM;
 unsigned int            m_TBW;
 unsigned int            m_TW;
 unsigned int            m_TH;
 unsigned int            m_SyncS;
 unsigned int            m_GIFlag;
 struct SMS_FrameBuffer* m_pBuffer;
 unsigned char*          m_pResult;
 unsigned char*          m_pCurRes;
 struct SMS_MacroBlock*  m_pMB;
 
 void ( *Sync       ) ( void                    );
 void ( *Display    ) ( struct SMS_FrameBuffer* );
 void ( *Destroy    ) ( void                    );
 void ( *SetTEX     ) ( void                    );
 void ( *GIFHandler ) ( void                    );

} IPUContext;
#endif  /* _WIN32 */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

IPUContext* IPU_InitContext ( struct GSContext*, int, int );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __IPU_H */
