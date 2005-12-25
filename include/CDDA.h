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
#ifndef __CDDA_H
# define __CDDA_H

typedef enum {
 DiskType_CD, DiskType_DVD, DiskType_CDDA, DiskType_DVDV, DiskType_Unknown, DiskType_Detect
} DiskType;

typedef enum {
 MediaMode_CD  = 1,
 MediaMode_DVD = 2
} MediaMode;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

typedef struct CDDA_Duration {

 unsigned char m_Minute __attribute__(  ( packed )  );
 unsigned char m_Second __attribute__(  ( packed )  );
 unsigned char m_Frame  __attribute__(  ( packed )  );

} CDDA_Duration;

typedef struct CDDA_TrackData {

 unsigned short m_Unknown1 __attribute__(  ( packed )  );
 unsigned char  m_TrackNr  __attribute__(  ( packed )  );
 unsigned int   m_Unknown2 __attribute__(  ( packed )  );
 CDDA_Duration  m_Duration __attribute__(  ( packed )  );

} CDDA_TrackData;

typedef struct CDDA_TOC {

 unsigned short m_Unknown1       __attribute__(  ( packed )  );
 unsigned char  m_Info1          __attribute__(  ( packed )  );
 unsigned int   m_Unknown2       __attribute__(  ( packed )  );
 unsigned char  m_StartTrack     __attribute__(  ( packed )  );
 unsigned int   m_Unknown3       __attribute__(  ( packed )  );
 unsigned char  m_Info2          __attribute__(  ( packed )  );
 unsigned int   m_Unknown4       __attribute__(  ( packed )  );
 unsigned char  m_EndTrack       __attribute__(  ( packed )  );
 unsigned int   m_Unknown5       __attribute__(  ( packed )  );
 unsigned char  m_Info3          __attribute__(  ( packed )  );
 unsigned int   m_Unknown6       __attribute__(  ( packed )  );
 CDDA_Duration  m_DiskDuration   __attribute__(  ( packed )  );
 CDDA_TrackData m_Tracks[   99 ] __attribute__(  ( packed )  );
 unsigned char  m_Pad   [ 1044 ] __attribute__(  ( packed )  );

} CDDA_TOC;

extern int g_CDDASpeed;
extern int g_DVDVSupport;

int      CDDA_Init         ( void                     );
void     CDDA_Exit         ( void                     );
int      CDDA_ReadTOC      ( CDDA_TOC*                );
int      CDDA_RawRead      ( int, int, unsigned char* );
int      CDDA_Synchronize  ( void                     );
DiskType CDDA_DiskType     ( void                     );
int      CDDA_Stop         ( void                     );
void     CDDA_Standby      ( void                     );
void     CDDA_Pause        ( void                     );
void     CDDA_DiskReady    ( void                     );
int      CDDA_SetMediaMode ( MediaMode                );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __CDDA_H */
