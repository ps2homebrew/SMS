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
#ifndef __SPU_H
# define __SPU_H

typedef struct SPUContext {

 void ( *PlayPCM ) ( char* );
 void ( *Mute    ) ( int   );
 void ( *Destroy ) ( void  );

} SPUContext;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SPUContext* SPU_InitContext ( int, int );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SPU_H */
