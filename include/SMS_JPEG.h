/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_JPEG_H
#define __SMS_JPEG_H

typedef enum _jpeg_clr_fmt {
 JCF_Invalid = -1, JCF_8x8, JCF_16x8, JCF_8x16, JCF_16x16
} _jpeg_clr_fmt;

typedef struct _jpeg_comp_desc {

 int           m_Pred;
 unsigned char m_CompID;
 unsigned char m_HSF;
 unsigned char m_VSF;
 unsigned char m_HDIV;
 unsigned char m_VDIV;
 char          m_QT;
 char          m_DCHT;
 char          m_ACHT;

} _jpeg_comp_desc;

typedef struct SMS_JPEGContext {

 unsigned int               m_DCTbl[ 2 ][  64 ];
 unsigned int               m_ACTbl[ 2 ][ 384 ];
 unsigned int*              m_pHTbl[ 4 ];
 unsigned char              m_QTable[ 4 ][ 64 ];
 unsigned int               m_nMCU;
 unsigned int               m_Width;
 unsigned int               m_Height;
 unsigned int               m_MCUW;
 unsigned int               m_MCUH;
 unsigned int               m_MW;
 unsigned int               m_MH;
 unsigned int               m_RW;
 unsigned int               m_RH;
 unsigned int               m_MR;
 unsigned int               m_MC;
 unsigned int               m_nComp;
 unsigned int               m_MCUIdx  [ 6 ];
 _jpeg_comp_desc            m_CompDesc[ 3 ];
 _jpeg_clr_fmt              m_Format;
 unsigned char*             m_pData;
 unsigned char*             m_pDataEnd;
 unsigned int               m_BitBuf;
 unsigned int               m_nBits;
 struct SMS_RescaleContext* m_pRC;
 short*                     m_pBlkIn;
 short*                     m_pBlkOut;
 short*                     m_pBlkEnd;
 unsigned int               m_nBytesPM;
 unsigned int               m_MCUIncr;
 unsigned int*              m_pDst;
 unsigned int               m_VU1Mem;
 unsigned int               m_nAlloc;
 unsigned char*             m_pBitmap;
 unsigned int               m_nBitmapQWC;
 unsigned int               m_nBitmapBytes;
 void*                      m_pPrgParam;
 unsigned int               m_PrgPerc;

 void ( *pack     ) ( void*, int, int     );
 void ( *progress ) ( void*, unsigned int );

} SMS_JPEGContext;

typedef struct SMS_JPEGViewer {

 unsigned long              m_BitBltPack[ 64 ];
 unsigned long              m_ImagePack [  8 ];
 unsigned long              m_BordPackZ [ 14 ];
 unsigned long              m_BordPack  [ 22 ];
 unsigned long              m_DrawPack  [  8 ];
 unsigned long              m_XYZ       [  2 ][ 2 ];
 SMS_JPEGContext*           m_pCtx;
 struct SMS_List*           m_pItems;
 struct SMS_ListNode*       m_pCurrent;
 unsigned int               m_Width;
 unsigned int               m_Height;
 unsigned int               m_StsX;
 unsigned int               m_StsY;
 unsigned int               m_StsW;
 unsigned int               m_StsH;
 unsigned long*             m_pDefPic;
 unsigned long*             m_pStsOutline;
 unsigned long*             m_pStsBkgnd;
 unsigned long*             m_pStsText;
 unsigned long*             m_pStsPgBg;
 unsigned long*             m_pStsPgFg;
 unsigned short*            m_pStsPgX;
 float                      m_PixPP;
 unsigned int               m_TextCoord[ 4 ];
 unsigned int               m_Mode;
 unsigned char              m_ClrDepthOrg;
 struct SMS_ContainedrJPEG* m_pCont;
} SMS_JPEGViewer;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

SMS_JPEGContext* SMS_JPEGInit    (  void ( * ) ( void*, unsigned int ), void*  );
void             SMS_JPEGDestroy ( SMS_JPEGContext*                            );
int              SMS_JPEGLoad    ( SMS_JPEGContext*, void*, unsigned int       );

SMS_JPEGViewer* SMS_JPEGViewerInit    ( void                              );
void            SMS_JPEGViewerDestroy ( SMS_JPEGViewer*                   );
void            SMS_JPEGViewerPerform ( SMS_JPEGViewer*, struct SMS_List* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_JPEG_H */
