/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 200X ps2dev -> http://www.ps2dev.org
# Adopted for SMS in 2006 by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_MC_H
# define __SMS_MC_H

typedef struct SMS_MCTable {

 struct {

  unsigned char  m_Unk0;
  unsigned char  m_Sec;
  unsigned char  m_Min;
  unsigned char  m_Hour;
  unsigned char  m_Day;
  unsigned char  m_Month;
  unsigned short m_Year;

 } m_Create;

 struct {

  unsigned char  m_Unk1;
  unsigned char  m_Sec;
  unsigned char  m_Min;
  unsigned char  m_Hour;
  unsigned char  m_Day;
  unsigned char  m_Month;
  unsigned short m_Year;

 } m_Modify;

 unsigned int   m_FileSize;
 unsigned short m_FileAttr;
 unsigned short m_Unk2;
 unsigned int   m_Unk3[  2 ];
 unsigned char  m_Name[ 32 ];

} SMS_MCTable __attribute__(   (  aligned( 64 )  )   );

typedef struct SMS_MCIcon {

 unsigned char  m_Header[ 4 ];
 unsigned short m_Unk0;
 unsigned short m_Offset;
 unsigned       m_Unk1;
 unsigned       m_Trans;
 int            m_ClrBg   [ 4 ][ 4 ];
 float          m_LightDir[ 3 ][ 4 ];
 float          m_LightCol[ 3 ][ 4 ];
 float          m_LightAmb[ 4 ];
 unsigned short m_Title[  34 ];
 unsigned char  m_View [  64 ];
 unsigned char  m_Copy [  64 ];
 unsigned char  m_Del  [  64 ];
 unsigned char  m_Unk2 [ 512 ];

} SMS_MCIcon;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

int MC_Init    ( void                                               );
int MC_Sync    ( int*                                               );
int MC_GetInfo ( int, int, int*, int*, int*                         );
int MC_GetDir  ( int, int, const char*, unsigned, int, SMS_MCTable* );
int MC_Open    ( int, int, const char*, int                         );
int MC_Read    ( int, void*, int                                    );
int MC_Close   ( int                                                );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_MC_H */
