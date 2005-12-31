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

struct SMS_MacroBlock;
struct SMS_FrameBuffer;

# ifdef _WIN32

typedef struct IPUContext {

 unsigned int m_MBWidth;
 unsigned int m_MBHeight;
 unsigned int m_Width;
 unsigned int m_UVWidth;
 unsigned int m_Height;
 unsigned int m_UVHeight;
 unsigned int m_Linesize;

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

 unsigned long int      m_DMAGIFDraw[ 20 ] __attribute__(   ( aligned( 16 )  )   );
 unsigned long int      m_DMAVIFDraw[  8 ] __attribute__(   ( aligned( 16 )  )   );
 unsigned long int      m_DMAViFDraw[  8 ] __attribute__(   ( aligned( 16 )  )   );
 unsigned long int      m_DMAVIPDraw[  8 ] __attribute__(   ( aligned( 16 )  )   );
 unsigned long int      m_DMAGIFTX  [  4 ] __attribute__(   ( aligned( 16 )  )   );
 unsigned int           m_ImgLeft   [  5 ];
 unsigned int           m_ImgTop    [  5 ];
 unsigned int           m_ImgRight  [  5 ];
 unsigned int           m_ImgBottom [  5 ];
 unsigned int           m_TxtLeft   [  5 ];
 unsigned int           m_TxtTop    [  5 ];
 unsigned int           m_TxtRight  [  5 ];
 unsigned int           m_TxtBottom [  5 ];
 unsigned long int      m_DestY;
 unsigned int           m_Slice;
 unsigned int           m_MB;
 unsigned int           m_nMBSlice;
 unsigned int           m_nMBSlices;
 unsigned int           m_MBStride;
 unsigned int           m_QWCToIPUSlice;
 unsigned int           m_QWCFromIPUSlice;
 unsigned int           m_DMAHandlerID_IPU;
 unsigned int           m_DMAHandlerID_GIF;
# ifdef VB_SYNC
 unsigned int           m_VBlankStartHandlerID;
 unsigned int           m_VBlankEndHandlerID;
 unsigned int           m_fDraw;
 unsigned int           m_fBlank;
# endif  /* VB_SYNC */
 unsigned int           m_VRAM;
 unsigned int           m_TBW;
 unsigned int           m_TW;
 unsigned int           m_TH;
 unsigned int           m_SyncS;
 unsigned int           m_GIFlag;
 unsigned int           m_Width;
 unsigned int           m_Height;
 unsigned int           m_ScrLeft;
 unsigned int           m_ScrTop;
 unsigned int           m_ScrRight;
 unsigned int           m_ScrBottom;
 unsigned int           m_VIFQueueSize;
 unsigned int           m_ViFQueueSize;
 unsigned int           m_VIPQueueSize;
 unsigned int           m_ModeIdx;
 unsigned char*         m_pResult;
 unsigned long int*     m_pDMAPacket;
 struct SMS_MacroBlock* m_pMB;
 
 void ( *Sync         ) ( void         );
 void ( *Display      ) ( void*        );
 void ( *Destroy      ) ( void         );
 void ( *SetTEX       ) ( void         );
 void ( *GIFHandler   ) ( void         );
 void ( *Reset        ) ( void         );
 void ( *QueuePacket  ) ( int, void*   );
 void ( *PQueuePacket ) ( int, void*   );
 void ( *iQueuePacket ) ( int, void*   );
 void ( *Suspend      ) ( void         );
 void ( *Resume       ) ( void         );
 void ( *Repaint      ) ( void         );
 void ( *ChangeMode   ) ( unsigned int );
 void ( *Pan          ) ( int          );
 void ( *Flush        ) ( void         );

} IPUContext;
#endif  /* _WIN32 */
extern IPUContext g_IPUCtx;
# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

IPUContext* IPU_InitContext ( int, int );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __IPU_H */
