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
#ifndef __Timer_H
# define __Timer_H

volatile unsigned long int g_Timer;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void Timer_Init    ( void         );
void Timer_Wait    ( unsigned int );
void Timer_Destroy ( void         );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __Timer_H */
