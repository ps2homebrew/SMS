#ifndef __StringList_H
# define __StringList_H

typedef struct StringListNode {

 char*                  m_pString;
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

} StringList;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

StringList* StringList_Init ( void );
# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __StringList_H */
