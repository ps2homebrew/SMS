/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
#               2005-2007 - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_MPEG_H
#define __SMS_MPEG_H

#ifndef __SMS_DSP_H
#include "SMS_DSP.h"
#endif  /* __SMS_DSP_H */

#ifndef __SMS_Bitio_H
#include "SMS_Bitio.h"
#endif  /* __SMS_Bitio_H */

#ifndef __SMS_Codec_H
#include "SMS_Codec.h"
#endif  /* __SMS_Codec_H */

#define SMS_RECT_SHAPE     0
#define SMS_BIN_SHAPE      1
#define SMS_BIN_ONLY_SHAPE 2
#define SMS_GRAY_SHAPE     3

#define SMS_FRAME_SKIPED    100
#define SMS_ASPECT_EXTENDED  15

#define SMS_ER_CAREFUL         1
#define SMS_ER_COMPLIANT       2
#define SMS_ER_AGGRESSIVE      3
#define SMS_ER_VERY_AGGRESSIVE 4

#define SMS_MV_DIR_BACKWARD 1
#define SMS_MV_DIR_FORWARD  2
#define SMS_MV_DIRECT       4

#define SMS_MV_TYPE_16X16 0
#define SMS_MV_TYPE_8X8   1
#define SMS_MV_TYPE_16X8  2
#define SMS_MV_TYPE_FIELD 3

#define SMS_MB_TYPE_INTRA4x4   0x0001
#define SMS_MB_TYPE_INTRA16x16 0x0002
#define SMS_MB_TYPE_INTRA_PCM  0x0004
#define SMS_MB_TYPE_16x16      0x0008
#define SMS_MB_TYPE_16x8       0x0010
#define SMS_MB_TYPE_8x16       0x0020
#define SMS_MB_TYPE_8x8        0x0040
#define SMS_MB_TYPE_INTERLACED 0x0080
#define SMS_MB_TYPE_DIRECT2    0x0100
#define SMS_MB_TYPE_ACPRED     0x0200
#define SMS_MB_TYPE_GMC        0x0400
#define SMS_MB_TYPE_SKIP       0x0800
#define SMS_MB_TYPE_P0L0       0x1000
#define SMS_MB_TYPE_P1L0       0x2000
#define SMS_MB_TYPE_P0L1       0x4000
#define SMS_MB_TYPE_P1L1       0x8000
#define SMS_MB_TYPE_L0         ( SMS_MB_TYPE_P0L0 | SMS_MB_TYPE_P1L0 )
#define SMS_MB_TYPE_L1         ( SMS_MB_TYPE_P0L1 | SMS_MB_TYPE_P1L1 )
#define SMS_MB_TYPE_L0L1       ( SMS_MB_TYPE_L0   | SMS_MB_TYPE_L1   )
#define SMS_MB_TYPE_QUANT      0x00010000
#define SMS_MB_TYPE_CBP        0x00020000
#define SMS_MB_TYPE_INTRA      SMS_MB_TYPE_INTRA4x4

#define SMS_IS_DIRECT( a )       (  (a) & SMS_MB_TYPE_DIRECT2    )
#define SMS_IS_SKIP( a )         (  (a) & SMS_MB_TYPE_SKIP       )
#define SMS_IS_INTRA( a )        (  (a) & 7                      )
#define SMS_IS_ACPRED( a )       (  (a) & SMS_MB_TYPE_ACPRED     )
#define SMS_IS_8X8( a )          (  (a) & SMS_MB_TYPE_8x8        )
#define SMS_IS_INTERLACED( a )   (  (a) & SMS_MB_TYPE_INTERLACED )
#define SMS_USES_LIST( a, list ) (  (a) & (  ( SMS_MB_TYPE_P0L0 | SMS_MB_TYPE_P1L0 ) << ( 2 * (list) )  )   )

#define SMS_SLICE_OK     0
#define SMS_SLICE_END   -2
#define SMS_SLICE_NOEND -3

#define SMS_VP_START  1
#define SMS_AC_ERROR  2
#define SMS_DC_ERROR  4
#define SMS_MV_ERROR  8
#define SMS_AC_END   16
#define SMS_DC_END   32
#define SMS_MV_END   64

#define SMS_PICT_TOP_FIELD    1
#define SMS_PICT_BOTTOM_FIELD 2
#define SMS_PICT_FRAME        3

#define MAX_PICTURE_COUNT     3
#define BITSTREAM_BUFFER_SIZE 1024 * 256

typedef struct SMS_ScanTable {

 const uint8_t* m_pScantable;
 uint8_t        m_RasterEnd[ 64 ];

} SMS_ScanTable;

typedef struct SMS_MPEGContext {

 char              m_Y_DCScale;
 char              m_C_DCScale;
 char              m_QScale;
 char              m_ChromaQScale;
 const uint8_t*    m_pChromaQScaleTbl;
 uint8_t*          m_pY_DCScaleTbl;
 uint8_t*          m_pC_DCScaleTbl;
 int               m_MBX;
 int               m_MBY;
 int               m_MBW;
 int               m_MBH;
 SMS_DSPContext    m_DSPCtx;
 SMS_ScanTable     m_InterScanTbl;
 SMS_ScanTable     m_IntraScanTbl;
 SMS_ScanTable     m_IntraHScanTbl;
 SMS_ScanTable     m_IntraVScanTbl;
 SMS_Frame         m_CurPic;
 SMS_Frame         m_NextPic;
 SMS_Frame         m_LastPic;
 int               m_MV          [  2 ][ 4 ][ 2 ];
 short             m_BlockLastIdx[  6 ];
 short             m_BlockSum;
 short             m_MCBlkIdx;
 SMS_MacroBlock*   m_pMCBlk[ 2 ];
 int               m_BlockIdx    [  6 ];
 int               m_LastMV      [  2 ][ 2 ][ 2 ];
 int               m_BlockWrap   [  6 ];
 int               m_SpriteOffset[  2 ][ 2 ];
 int               m_SpriteDelta [  2 ][ 2 ];
 int               m_FieldSelect [  2 ][ 2 ];
 int16_t           ( *m_pPFieldMVTbl[ 2 ][ 2 ] )[ 2 ];
 SMS_BitContext    m_BitCtx;
 SMS_BitContext    m_LastResyncBitCtx;
 int16_t           ( *m_pACVal[ 3 ] )[ 16 ];
 int16_t*          m_pDCVal[ 3 ];
 SMS_Rational      m_SampleAspectRatio;
 int64_t           m_Time;
 int64_t           m_LastNonBTime;
 int               m_SpriteShift [ 2 ];

 int  ( *DecodeMB            ) ( SMS_DCTELEM[ 6 ][ 64 ] );
 void ( *DCT_UnquantizeIntra ) ( SMS_DCTELEM*           );
 void ( *DCT_UnquantizeInter ) ( SMS_DCTELEM*           );
 void ( *MBCallback          ) ( void*                  );

 SMS_DCTELEM       ( *m_pBlock  )     [ 64 ];
 SMS_DCTELEM       ( *m_pBlocks )[ 6 ][ 64 ];
 SMS_CodecContext* m_pParentCtx;
 SMS_Frame*        m_pPic;
 SMS_Frame*        m_pCurPic;
 SMS_Frame*        m_pLastPic;
 SMS_Frame*        m_pNextPic;
 SMS_MacroBlock*   m_pDest;
 SMS_MacroBlock*   m_pDestCB;
 int*              m_pMBIdx2XY;
 int16_t*          m_pDCValBase;
 int16_t           ( *m_pACValBase  )[ 16 ];
 uint8_t*          m_pCBPTbl;
 uint8_t*          m_pPredDirTbl;
 uint8_t*          m_pMBIntraTbl;
 uint8_t*          m_pMBSkipTbl;
 uint8_t*          m_pBSBuf;
 uint8_t*          m_pCodedBlockBase;
 uint8_t*          m_pCodedBlock;
 int               m_BitRate;
 int               m_ACPred;
 int               m_MBStride;
 int               m_B8Stride;
 int               m_B4Stride;
 int               m_Width;
 int               m_Height;
 int               m_HEdgePos;
 int               m_VEdgePos;
 int               m_MBNum;
 int               m_MBSkiped;
 int               m_DivXVersion;
 int               m_VoType;
 int               m_VolCtlPar;
 int               m_AspectRatio;
 int               m_LowDelay;
 int               m_PicNr;
 int               m_Shape;
 int               m_TimeIncRes;
 int               m_TimeIncBits;
 int               m_TFrame;
 int               m_ProgSeq;
 int               m_ProgFrm;
 int               m_VolSpriteUsage;
 int               m_SpriteLeft;
 int               m_SpriteTop;
 int               m_SpriteWidth;
 int               m_SpriteHeight;
 int               m_nSpriteWarpPts;
 int               m_SpriteWarpAccuracy;
 int               m_RealSpriteWarpPts;
 int               m_SpriteBrightnessChange;
 int               m_LowLatencySprite;
 int               m_QuantPrec;
 int               m_MPEGQuant;
 int               m_QuarterSample;
 int               m_ResyncMarker;
 int               m_ResyncMBX;
 int               m_ResyncMBY;
 int               m_DataPartitioning;
 int               m_RVLC;
 int               m_NewPred;
 int               m_ReducedResVop;
 int               m_Scalability;
 int               m_HierachyType;
 int               m_EnhancementType;
 int               m_DivXVer;
 int               m_DivXBuild;
 int               m_DivXPack;
 int               m_XviDBuild;
 int               m_TimeBase;
 int               m_LastTimeBase;
 int               m_PicType;
 int               m_Flags;
 int               m_PartFrame;
 int               m_DataPart;
 int               m_NoRounding;
 int               m_IntraDCThreshold;
 int               m_TopFieldFirst;
 int               m_AltScan;
 int               m_nMBLeft;
 int               m_FCode;
 int               m_BCode;
 int               m_Bugs;
 int               m_BSBufSize;
 int               m_PaddingBugScore;
 uint16_t          m_LineSize;
 uint16_t          m_LineStride;
 uint16_t          m_SliceHeight;
 uint16_t          m_MSPerFrame;
 uint16_t          m_PPTime;
 uint16_t          m_PBTime;
 uint16_t          m_PPFieldTime;
 uint16_t          m_PBFieldTime;
 uint8_t           m_MBIntra;
 uint8_t           m_MVDir;
 uint8_t           m_MVType;
 uint8_t           m_MCSel;
 uint8_t           m_InterlacedDCT;
 uint8_t           m_FirstSliceLine;
 uint8_t           m_H263LongVectors;
 uint8_t           m_H263Pred;
 uint8_t           m_NextPFrameDamaged;
 uint8_t           m_PicStruct;
 uint8_t           m_FirstField;
 uint8_t           m_DCTblIdx;
 uint8_t           m_MVTblIdx;
 uint8_t           m_InterIntraPred;
 uint8_t           m_RLTblIdx;
 uint8_t           m_RLChromaTblIdx;
 uint8_t           m_UseSkipMBCode;
 uint8_t           m_PerMBRLTbl;
 uint8_t           m_H263AICDir;
 uint8_t           m_FlipFlopRnd;

} SMS_MPEGContext;

extern SMS_MPEGContext g_MPEGCtx;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void SMS_MPEGContext_Init          ( int, int                        );
void SMS_MPEGContext_Destroy       ( void                            );
void SMS_MPEG_FrameStart           ( void                            );
void SMS_MPEG_InitBlockIdx         ( void                            );
void SMS_MPEG_InitScanTable        ( SMS_ScanTable*, const uint8_t*  );
void SMS_MPEG_SetQScale            ( int                             );
void SMS_MPEG_FrameEnd             ( void                            );
void SMS_MPEG_DecodeMB             ( SMS_DCTELEM[ 12 ][ 64 ]         );
void SMS_MPEG_CleanIntraTblEntries ( void                            );
void SMS_MPEG_DummyCB              ( void*                           );
void SMS_MPEG_MCB                  ( void*                           );
void SMS_MPEG_SCB                  ( void*                           );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

static SMS_INLINE void SMS_MPEG_UpdateBlockIdx ( void ) {
 g_MPEGCtx.m_BlockIdx[ 0 ] += 2;
 g_MPEGCtx.m_BlockIdx[ 1 ] += 2;
 g_MPEGCtx.m_BlockIdx[ 2 ] += 2;
 g_MPEGCtx.m_BlockIdx[ 3 ] += 2;
 ++g_MPEGCtx.m_BlockIdx[ 4 ];
 ++g_MPEGCtx.m_BlockIdx[ 5 ];
 ++g_MPEGCtx.m_pDest;
}  /* end SMS_MPEG_UpdateBlockIdx */
#endif  /* __SMS_MPEG_H */
