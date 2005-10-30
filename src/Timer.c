/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 200X by Hermes (his code was found at PS2Dev forums posted on Tue Jul 13, 2004 8:06 pm)
# Copyright (c) 2005 by Eugene Plotnikov (T1_XXXX stuff)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "Timer.h"

#include <kernel.h>

#define T0_COUNT *(  ( volatile u32* )0x10000000  )
#define T0_MODE  *(  ( volatile u32* )0x10000010  )
#define T0_COMP  *(  ( volatile u32* )0x10000020  )
#define T0_HOLD  *(  ( volatile u32* )0x10000030  )

#define T1_COUNT *(  ( volatile u32* )0x10000800  )
#define T1_MODE  *(  ( volatile u32* )0x10000810  )
#define T1_COMP  *(  ( volatile u32* )0x10000820  )
#define T1_HOLD  *(  ( volatile u32* )0x10000830  )

#ifndef NO_RTC
volatile unsigned long int g_Timer;
#endif  /* NO_RTC */

#define N_HANDLERS 2

static int s_WaitSema;
static int s_T0HandlerID;
static int s_T1HandlerID;

static void ( *TimerHandler[ 2 ] ) ( void );

static int T0_Handler ( int aCause ) { 
#ifndef NO_RTC
 g_Timer += 4;

 if (  !( g_Timer & 0x000000000000003F )  ) {

  if (  TimerHandler[ 0 ] ) TimerHandler[ 0 ] ();
  if (  TimerHandler[ 1 ] ) TimerHandler[ 1 ] ();

 }  /* end if */
#else
 TimerHandler ();
#endif  /* NO_RTC */
 T0_MODE |= 1024;

 return -1;

}  /* end T0_Handler */

static int T1_Handler ( int aCause ) {

 iSignalSema ( s_WaitSema );

 T1_MODE |= 1024;

 return -1;

}  /* end T1_Handler */

void Timer_Init ( void ) {

 ee_sema_t lSema;

 lSema.init_count = 0;
 lSema.max_count  = 1;
 s_WaitSema = CreateSema ( &lSema );

 TimerHandler[ 0 ] =
 TimerHandler[ 1 ] = NULL;
#ifndef NO_RTC
 T0_COMP  = ( u32 )(   4.0F / ( 256.0F / 147456.0F )  );
#else
 T0_COMP  = ( u32 )(  64.0F / ( 256.0F / 147456.0F )  );
#endif  /* NO_RTC */
 T0_COUNT = 0;
 T0_MODE  = 256 + 128 + 64 + 2;

 T1_COMP  = ( u32 )(  64.0F / ( 256.0F / 147456.0F )  );
 T1_MODE  = 256 + 128 + 64 + 2;

 s_T0HandlerID = AddIntcHandler (  9, T0_Handler, 0 );
 s_T1HandlerID = AddIntcHandler ( 10, T1_Handler, 0 );
#ifndef NO_RTC
 EnableIntc ( 9 );
#endif  /* NO_RTC */
}  /* end Timer_Init */

void Timer_Destroy ( void ) {

 DisableIntc ( 9 );
 RemoveIntcHandler (  9, s_T0HandlerID );
 RemoveIntcHandler ( 10, s_T1HandlerID );

 DeleteSema ( s_WaitSema );

 TimerHandler[ 0 ] =
 TimerHandler[ 1 ] = NULL;

}  /* end Timer_Destroy */

void Timer_Wait ( unsigned int aPeriod ) {

 unsigned long i, lnParts, lnRem;

 lnParts = aPeriod / 64;
 lnRem   = aPeriod % 64;

 if ( lnParts ) {

  T1_COUNT = 0;
  T1_COMP  = ( u32 )(  64.0F / ( 256.0F / 147456.0F )  );

  EnableIntc  ( 10 );
   for ( i = 0; i < lnParts; ++i ) WaitSema ( s_WaitSema );
  DisableIntc ( 10 );

 }  // end if

 if ( lnRem ) {

  T1_COUNT = 0;
  T1_COMP  = ( u32 )(  lnRem / ( 256.0F / 147456.0F )  );

  EnableIntc  ( 10 );
   WaitSema ( s_WaitSema );
  DisableIntc ( 10 );

 }  // end if

}  /* end Timer_Wait */

void Timer_iRegisterHandler ( int anIndex, void* apHandler ) {

 TimerHandler[ anIndex ] = apHandler;

}  /* end Timer_iRegisterHandler */

void* Timer_RegisterHandler ( int anIndex, void* apHandler ) {

 void* retVal;

 DIntr ();
  retVal = TimerHandler[ anIndex ];

  if ( apHandler != TimerHandler[ anIndex ] ) {

   TimerHandler[ anIndex ] = (  void ( * ) ( void )  )apHandler;
#ifdef NO_RTC
   if ( apHandler != NULL )

    EnableIntc ( 9 );

   else DisableIntc ( 9 );
#endif  /* NO_RTC */
  }  /* end if */
 EIntr ();

 return retVal;

}  /* end Timer_RegisterHandler */
