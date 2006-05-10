#ifndef __SMS_Timer_H
# define __SMS_Timer_H

extern unsigned long g_Timer;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void  SMS_TimerInit    ( void );
void  SMS_TimerDestroy ( void );
void  SMS_TimerSet     (  unsigned int, unsigned long, void ( * ) ( void* ), void*  );
void  SMS_iTimerSet    (  unsigned int, unsigned long, void ( * ) ( void* ), void*  );
void* SMS_TimerReset   ( unsigned int, void* );
void  SMS_iTimerReset  ( unsigned int );
void  SMS_TimerWait    ( unsigned long );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_Timer_H */
