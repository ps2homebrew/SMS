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
#ifndef __SMS_FileMapping_H
#define __SMS_FileMapping_H

struct FileContext;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void  SMS_FileMappingInit    ( void                );
void  SMS_FileMappingDestroy ( void                );
void* SMS_FileMappingMap     ( struct FileContext* );
void  SMS_FileMappingUnMap   ( void*               );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __SMS_FileMapping_H */
