/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
#               2005 - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_MPEG_H
# define __SMS_MPEG_H

# ifndef __SMS_DSP_H
#  include "SMS_DSP.h"
# endif  /* __SMS_DSP_H */

# ifndef __SMS_Bitio_H
#  include "SMS_Bitio.h"
# endif  /* __SMS_Bitio_H */

# ifndef __SMS_Codec_H
#  include "SMS_Codec.h"
# endif  /* __SMS_Codec_H */

# define SMS_RECT_SHAPE     0
# define SMS_BIN_SHAPE      1
# define SMS_BIN_ONLY_SHAPE 2
# define SMS_GRAY_SHAPE     3

# define SMS_FRAME_SKIPED    100
# define SMS_ASPECT_EXTENDED  15

# define SMS_ER_CAREFUL         1
# define SMS_ER_COMPLIANT       2
# define SMS_ER_AGGRESSIVE      3
# define SMS_ER_VERY_AGGRESSIVE 4

# define SMS_MV_DIR_BACKWARD 1
# define SMS_MV_DIR_FORWARD  2
# define SMS_MV_DIRECT       4

# define SMS_MV_TYPE_16X16 0
# define SMS_MV_TYPE_8X8   1
# define SMS_MV_TYPE_16X8  2
# define SMS_MV_TYPE_FIELD 3

# define SMS_MB_TYPE_INTRA4x4   0x0001
# define SMS_MB_TYPE_INTRA16x16 0x0002
# define SMS_MB_TYPE_INTRA_PCM  0x0004
# define SMS_MB_TYPE_16x16      0x0008
# define SMS_MB_TYPE_16x8       0x0010
# define SMS_MB_TYPE_8x16       0x0020
# define SMS_MB_TYPE_8x8        0x0040
# define SMS_MB_TYPE_INTERLACED 0x0080
# define SMS_MB_TYPE_DIRECT2    0x0100
# define SMS_MB_TYPE_ACPRED     0x0200
# define SMS_MB_TYPE_GMC        0x0400
# define SMS_MB_TYPE_SKIP       0x0800
# define SMS_MB_TYPE_P0L0       0x1000
# define SMS_MB_TYPE_P1L0       0x2000
# define SMS_MB_TYPE_P0L1       0x4000
# define SMS_MB_TYPE_P1L1       0x8000
# define SMS_MB_TYPE_L0         ( SMS_MB_TYPE_P0L0 | SMS_MB_TYPE_P1L0 )
# define SMS_MB_TYPE_L1         ( SMS_MB_TYPE_P0L1 | SMS_MB_TYPE_P1L1 )
# define SMS_MB_TYPE_L0L1       ( SMS_MB_TYPE_L0   | SMS_MB_TYPE_L1   )
# define SMS_MB_TYPE_QUANT      0x00010000
# define SMS_MB_TYPE_CBP        0x00020000
# define SMS_MB_TYPE_INTRA      SMS_MB_TYPE_INTRA4x4

# define SMS_IS_DIRECT( a )       (  (a) & SMS_MB_TYPE_DIRECT2    )
# define SMS_IS_SKIP( a )         (  (a) & SMS_MB_TYPE_SKIP       )
# define SMS_IS_INTRA( a )        (  (a) & 7                      )
# define SMS_IS_ACPRED( a )       (  (a) & SMS_MB_TYPE_ACPRED     )
# define SMS_IS_8X8( a )          (  (a) & SMS_MB_TYPE_8x8        )
# define SMS_IS_INTERLACED( a )   (  (a) & SMS_MB_TYPE_INTERLACED )
# define SMS_USES_LIST( a, list ) (  (a) & (  ( SMS_MB_TYPE_P0L0 | SMS_MB_TYPE_P1L0 ) << ( 2 * (list) )  )   )

# define SMS_SLICE_OK     0
# define SMS_SLICE_END   -2
# define SMS_SLICE_NOEND -3

# define SMS_VP_START  1
# define SMS_AC_ERROR  2
# define SMS_DC_ERROR  4
# define SMS_MV_ERROR  8
# define SMS_AC_END   16
# define SMS_DC_END   32
# define SMS_MV_END   64

# define SMS_PICT_TOP_FIELD    1
# define SMS_PICT_BOTTOM_FIELD 2
# define SMS_PICT_FRAME        3

typedef struct SMS_ScanTable {

 const uint8_t* m_pScantable;
 uint8_t        m_Permutated[ 64 ];
 uint8_t        m_RasterEnd [ 64 ];

} SMS_ScanTable;

typedef struct SMS_MPEGContext {

 SMS_DSPContext    m_DSPCtx;
 SMS_ScanTable     m_InterScanTbl;
 SMS_ScanTable     m_IntraScanTbl;
 SMS_ScanTable     m_IntraHScanTbl;
 SMS_ScanTable     m_IntraVScanTbl;
 uint16_t          m_IntraMatrix      [ 64 ];
 uint16_t          m_ChromaIntraMatrix[ 64 ];
 uint16_t          m_InterMatrix      [ 64 ];
 uint16_t          m_ChromaInterMatrix[ 64 ];
 SMS_Frame         m_CurPic;
 SMS_Frame         m_NextPic;
 SMS_Frame         m_LastPic;
 int               m_MV          [  2 ][ 4 ][ 2 ];
 int               m_BlockLastIdx[ 12 ];
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
 SMS_MacroBlock*   m_pMacroBlock[ 2 ];
 int64_t           m_Time;
 int64_t           m_LastNonBTime;
 int               m_SpriteShift [ 2 ];

 int  ( *DecodeMB            ) ( SMS_DCTELEM[ 6 ][ 64 ] );
 void ( *DCT_UnquantizeIntra ) ( SMS_DCTELEM*, int, int );
 void ( *DCT_UnquantizeInter ) ( SMS_DCTELEM*, int, int );

 SMS_DCTELEM       ( *m_pBlock  )     [ 64 ];
 SMS_DCTELEM       ( *m_pBlocks )[ 6 ][ 64 ];
 SMS_CodecContext* m_pParentCtx;
 SMS_Frame*        m_pPic;
 SMS_Frame*        m_pCurPic;
 SMS_Frame*        m_pLastPic;
 SMS_Frame*        m_pNextPic;
 SMS_MacroBlock*   m_pDest;
 SMS_MacroBlock*   m_pMCBuffer;
 SMS_MacroBlock*   m_pCache;
 int*              m_pMBIdx2XY;
 int16_t*          m_pDCValBase;
 int16_t           ( *m_pACValBase  )[ 16 ];
 uint8_t*          m_pY_DCScaleTbl;
 uint8_t*          m_pC_DCScaleTbl;
 uint8_t*          m_pCBPTbl;
 uint8_t*          m_pPredDirTbl;
 uint8_t*          m_pErrStatTbl;
 uint8_t*          m_pMBIntraTbl;
 uint8_t*          m_pMBSkipTbl;
 uint8_t*          m_pPrevPicTypes;
 uint8_t*          m_pBSBuf;
 uint8_t*          m_pEdgeEmuBufY;
 uint8_t*          m_pEdgeEmuBufCb;
 uint8_t*          m_pEdgeEmuBufCr;
 uint8_t*          m_pMCYBuf;
 uint8_t*          m_pMCCbBuf;
 uint8_t*          m_pMCCrBuf;
 uint8_t*          m_pCodedBlockBase;
 uint8_t*          m_pCodedBlock;
 const uint8_t*    m_pChromaQScaleTbl;
 int               m_fDirtyCache;
 int               m_IdxRes;
 int               m_Y_DCScale;
 int               m_C_DCScale;
 int               m_ACPred;
 int               m_MBX;
 int               m_MBY;
 int               m_MBW;
 int               m_MBH;
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
 int               m_ChromaQScale;
 int               m_QScale;
 int               m_nMBLeft;
 int               m_FCode;
 int               m_BCode;
 int               m_Bugs;
 int               m_MBIntra;
 int               m_MVDir;
 int               m_MVType;
 int               m_MCSel;
 int               m_InterlacedDCT;
 int               m_BlockIdx[ 6 ];
 int               m_FirstSliceLine;
 int               m_H263LongVectors;
 int               m_H263Pred;
 int               m_PaddingBugScore;
 int               m_Dropable;
 int               m_NextPFrameDamaged;
 int               m_CodedPicNr;
 int               m_ErrorCount;
 int               m_PicStruct;
 int               m_FirstField;
 int               m_BSBufSize;
 int               m_DCTblIdx;
 int               m_MVTblIdx;
 int               m_InterIntraPred;
 int               m_RLTblIdx;
 int               m_RLChromaTblIdx;
 int               m_UseSkipMBCode;
 int               m_PerMBRLTbl;
 int               m_H263AICDir;
 int               m_SliceHeight;
 int               m_FlipFlopRnd;
 int               m_BitRate;
 uint16_t          m_PPTime;
 uint16_t          m_PBTime;
 uint16_t          m_PPFieldTime;
 uint16_t          m_PBFieldTime;

} SMS_MPEGContext;

extern SMS_MPEGContext g_MPEGCtx;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_MPEGContext_Init          ( int, int                                  );
void SMS_MPEGContext_Destroy       ( void                                      );
int  SMS_MPEG_FrameStart           ( void                                      );
void SMS_MPEG_InitBlockIdx         ( void                                      );
void SMS_MPEG_InitScanTable        ( uint8_t*, SMS_ScanTable*, const uint8_t*  );
void SMS_MPEG_SetQScale            ( int                                       );
void SMS_MPEG_FrameEnd             ( void                                      );
void SMS_MPEG_DecodeMB             ( SMS_DCTELEM[ 12 ][ 64 ]                   );
void SMS_MPEG_CleanIntraTblEntries ( void                                      );
int  SMS_MPEGContext_FindUnusedPic ( void                                      );
void SMS_MPEG_CleanBuffers         ( void                                      );

# ifdef __cplusplus
}
# endif  /* __cplusplus */

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
