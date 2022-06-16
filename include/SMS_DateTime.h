/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_DateTime_H
#define __SMS_DateTime_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int  SMS_TimeZone   ( void  );
int  SMS_SummerTime ( void  );
int  SMS_TimeFormat ( void  );
void SMS_LocalTime  ( void* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __SMS_DateTime_H */
