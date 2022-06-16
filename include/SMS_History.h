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
#ifndef __SMS_History_H
#define __SMS_History_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void SMS_HistoryLoad   ( void                );
long SMS_HistoryLook   ( const char*, void** );
void SMS_HistoryAdd    ( const char*, long   );
int  SMS_HistoryRemove ( const char*         );
void SMS_HistorySave   ( void                );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_History_H */
