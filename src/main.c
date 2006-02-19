/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GUI.h"
#include "SMS_CDDA.h"
#include "SMS_CDVD.h"
#include "SMS_EE.h"
#include "SMS_IOP.h"
#include "SMS_DSP.h"
#include "SMS_Locale.h"

int main ( void ) {

 SMS_IOPReset ();
 SMS_EEInit   ();
 SMS_DSPInit  ();

 GUI_Initialize ( 1 );
 GUI_Status ( STR_INITIALIZING_SMS.m_pStr );

 CDDA_Init   ();
 CDVD_Init   ();
 SMS_IOPInit ();

 GUI_Run ();

 return 0;

}  /* end main */
