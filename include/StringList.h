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
#ifndef __StringList_H
# define __StringList_H

typedef struct StringListNode {

 char*                  m_pString;
 void*                  m_pParam;
 struct StringListNode* m_pNext;
 struct StringListNode* m_pPrev;

} StringListNode;

typedef struct StringList {

 StringListNode* m_pHead;
 StringListNode* m_pTail;

 void  ( *Push     ) ( struct StringList*, const char* );
 void  ( *PushBack ) ( struct StringList*, const char* );
 void  ( *Pop      ) ( struct StringList*              );
 void  ( *PopBack  ) ( struct StringList*              );
 void  ( *Destroy  ) ( struct StringList*, int         );
 void  ( *Sort     ) ( struct StringList*              );

} StringList;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

StringList* StringList_Init ( void );
# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __StringList_H */
