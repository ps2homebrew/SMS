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
#ifndef __SMS_List_H
# define __SMS_List_H

typedef struct SMS_ListNode {

 char*                m_pString;
 unsigned long        m_Param;
 struct SMS_ListNode* m_pNext;
 struct SMS_ListNode* m_pPrev;

} SMS_ListNode;

typedef struct SMS_List {

 SMS_ListNode* m_pHead;
 SMS_ListNode* m_pTail;
 unsigned int  m_Size;

} SMS_List;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SMS_List*     SMS_ListInit     ( void                     );
void          SMS_ListPush     ( SMS_List*, const char*   );
SMS_ListNode* SMS_ListPushBack ( SMS_List*, const char*   );
void          SMS_ListPop      ( SMS_List*                );
void          SMS_ListPopBack  ( SMS_List*                );
void          SMS_ListSort     ( SMS_List*                );
void          SMS_ListDestroy  ( SMS_List*, int           );
void          SMS_ListRemove   ( SMS_List*, SMS_ListNode* );
SMS_ListNode* SMS_ListFind     ( SMS_List*, const char*   );
SMS_ListNode* SMS_ListFindI    ( SMS_List*, const char*   );
SMS_ListNode* SMS_ListAt       ( SMS_List*, int           );
void          SMS_ListAppend   ( SMS_List*, SMS_List*     );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __StringList_H */
