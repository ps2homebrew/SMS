/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2008 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_GUIClock_H
#define __SMS_GUIClock_H

typedef struct GUIClockParam {
 short m_X;
 short m_Y;
 short m_W;
 short m_H;
} GUIClockParam;

extern GUIClockParam g_Clock;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void SMS_GUIClockInitialize ( void                 );
void SMS_GUIClockStart      ( const GUIClockParam* );
void SMS_GUIClockStop       ( void                 );
void SMS_GUIClockSuspend    ( void                 );
void SMS_GUIClockResume     ( void                 );
void SMS_GUIClockRedraw     ( void                 );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __SMS_GUIClock_H */
