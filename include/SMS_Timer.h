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
#ifndef __SMS_Timer_H
# define __SMS_Timer_H

volatile unsigned long int g_Timer;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void  Timer_Init             ( void         );
void  Timer_Wait             ( unsigned int );
void  Timer_Destroy          ( void         );
void* Timer_RegisterHandler  ( int, void*   );
void  Timer_iRegisterHandler ( int, void*   );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_Timer_H */
