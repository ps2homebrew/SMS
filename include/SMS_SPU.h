/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SPU_H
# define __SPU_H

typedef struct SMSound {

 unsigned int m_Sound;
 unsigned int m_Size;

} SMSound;

typedef struct SPUContext {

 float m_BufTime;

 void ( *PlayPCM   ) ( void* );
 void ( *SetVolume ) ( int   );
 void ( *Destroy   ) ( void  );
 void ( *Silence   ) ( void  );

} SPUContext;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void        SPU_Initialize   ( void          );
SPUContext* SPU_InitContext  ( int, int, int );
void        SPU_LoadData     ( void*, int    );
void        SPU_PlaySound    ( SMSound*, int );
int         SPU_Index2Volume ( int           );
void        SPU_Shutdown     ( void          );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SPU_H */
