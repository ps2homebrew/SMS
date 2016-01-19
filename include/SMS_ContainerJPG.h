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
#ifndef __SMS_ContainerJPG_H
#define __SMS_ContainerJPG_H

#ifndef __SMS_Container_H
#include "SMS_Container.h"
#endif  /* __SMS_Container_H */

typedef struct SMS_ContainerJPEG {

 struct FileContext*    ( *m_pOpen ) ( const char*, void* );
 void*                  m_pOpenParam;
 struct SMS_List*       m_pFileList;
 struct SMS_JPEGViewer* m_pViewer;

} SMS_ContainerJPEG;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int SMS_GetContainerJPG ( SMS_Container* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_ContainerJPG_H */
