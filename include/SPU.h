#ifndef __SPU_H
# define __SPU_H

typedef struct SPUContext {

 void ( *PlayPCM   ) ( void* );
 void ( *SetVolume ) ( int   );
 void ( *Destroy   ) ( void  );

} SPUContext;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SPUContext* SPU_InitContext ( int, int, int );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SPU_H */
