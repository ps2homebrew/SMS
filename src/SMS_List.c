/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Sort function (c) 2001 Simon Tatham.
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_List.h"

#include <malloc.h>
#include <string.h>

void SMS_ListPush ( SMS_List* apList, const char* apString ) {

 SMS_ListNode* lpNode = ( SMS_ListNode* )calloc (  1, sizeof ( SMS_ListNode ) + strlen ( apString ) + 1  );

 lpNode -> m_pString = (  ( char* )lpNode  ) + sizeof ( SMS_ListNode );
 strcpy ( lpNode -> m_pString, apString );

 lpNode -> m_pNext = apList -> m_pHead;

 if ( apList -> m_pHead )

  apList -> m_pHead -> m_pPrev = lpNode;

 else apList -> m_pTail = lpNode;

 apList -> m_pHead = lpNode;
 ++apList -> m_Size;

}  /* end SMS_ListPush */

SMS_ListNode* SMS_ListPushBack ( SMS_List* apList, const char* apString ) {

 SMS_ListNode* lpNode = ( SMS_ListNode* )calloc (  1, sizeof ( SMS_ListNode ) + strlen ( apString ) + 1  );

 lpNode -> m_pString = (  ( char* )lpNode  ) + sizeof ( SMS_ListNode );
 strcpy ( lpNode -> m_pString, apString );

 if ( apList -> m_pTail ) {

  lpNode -> m_pPrev            = apList -> m_pTail;
  apList -> m_pTail -> m_pNext = lpNode;
  apList -> m_pTail            = lpNode;

 } else apList -> m_pHead = apList -> m_pTail = lpNode;

 ++apList -> m_Size;

 return apList -> m_pTail;

}  /* end SMS_ListPushBack */

void SMS_ListPop ( SMS_List* apList ) {

 if ( apList -> m_pHead ) {

  SMS_ListNode* lpNode = apList -> m_pHead -> m_pNext;

  free ( apList -> m_pHead );

  apList -> m_pHead = lpNode;

  if ( !lpNode )

   apList -> m_pTail = NULL;

  else lpNode -> m_pPrev = NULL;

  --apList -> m_Size;

 }  /* end if */

}  /* end SMS_ListPop */

void SMS_ListPopBack ( SMS_List* apList ) {

 if ( apList -> m_pTail ) {

  SMS_ListNode* lpNode = apList -> m_pTail -> m_pPrev;

  free ( apList -> m_pTail );

  apList -> m_pTail = lpNode;

  if ( !lpNode )

   apList -> m_pHead = lpNode;

  else lpNode -> m_pNext = NULL;

  --apList -> m_Size;

 }  /* end if */

}  /* end SMS_ListPopBack */

void SMS_ListDestroy ( SMS_List* apList, int afDestroyList ) {

 SMS_ListNode* lpNode = apList -> m_pHead;

 while ( lpNode ) {

  SMS_ListNode* lpNext = lpNode -> m_pNext;

  free ( lpNode );

  lpNode = lpNext;

 }  /* end while */

 if ( afDestroyList )

  free ( apList );

 else {

  apList -> m_pHead =
  apList -> m_pTail = NULL;
  apList -> m_Size  = 0;

 }  /* end else */

}  /* end SMS_ListDestroy */

void SMS_ListSort ( SMS_List* apList ) {

 SMS_ListNode* lpList = apList -> m_pHead;
 SMS_ListNode* p;
 SMS_ListNode* q;
 SMS_ListNode* e;
 SMS_ListNode* lpTail;

 int i, lInSize, lnMerges, lPSize, lQSize;

 if ( !lpList ) return;

 lInSize = 1;

 while ( 1 ) {

  p        = lpList;
  lpList   = NULL;
  lpTail   = NULL;
  lnMerges = 0;

  while ( p ) {

   ++lnMerges;
   q = p;
   lPSize = 0;

   for ( i = 0; i < lInSize; ++i ) {

    ++lPSize;

    q = q -> m_pNext;

    if ( !q ) break;

   }  /* end for */

   lQSize = lInSize;

   while ( lPSize > 0 || ( lQSize > 0 && q )  ) {

    if ( lPSize == 0 ) {

     e = q;
     q = q -> m_pNext;
     --lQSize;
		    
    } else if ( lQSize == 0 || !q ) {

     e = p;
     p = p -> m_pNext;
     --lPSize;

    } else if (  strcmp ( p -> m_pString, q -> m_pString ) <= 0  ) {

     e = p;
     p = p -> m_pNext;
     --lPSize;

    } else {

     e = q;
     q = q -> m_pNext;
     --lQSize;

    }  /* end else */

    if ( lpTail ) {

     lpTail -> m_pNext = e;

    } else {

     lpList = e;

    }  /* end else */

    e -> m_pPrev = lpTail;
    lpTail       = e;

   }  /* end while */

   p = q;

  }  /* end while */

  lpTail -> m_pNext = NULL;

  if ( lnMerges <= 1 ) {

   apList -> m_pHead = lpList;
   apList -> m_pTail = lpTail;
   break;

  }  /* end if */

  lInSize *= 2;

 }  /* end while */

}  /* end SMS_ListSort */

void SMS_ListRemove ( SMS_List* apList, SMS_ListNode* apNode ) {

 if ( apNode == apList -> m_pHead )

  SMS_ListPop ( apList );

 else if ( apNode == apList -> m_pTail )

  SMS_ListPopBack ( apList );

 else {

  apNode -> m_pPrev -> m_pNext = apNode -> m_pNext;
  apNode -> m_pNext -> m_pPrev = apNode -> m_pPrev;

  free ( apNode );

  --apList -> m_Size;

 }  /* end else */

}  /* end SMS_ListRemove */

SMS_ListNode* SMS_ListAt ( SMS_List* apList, int anIdx ) {

 SMS_ListNode* lpNode;

 if ( anIdx >= apList -> m_Size ) return NULL;

 lpNode = apList -> m_pHead;

 while ( anIdx ) {

  lpNode = lpNode -> m_pNext;
  --anIdx;

 }  /* end while */

 return lpNode;

}  /* end SMS_ListAt */

static SMS_ListNode* _find ( SMS_List* apList, const char* apStr, int ( *compare ) ( const char*, const char* )  ) {

 SMS_ListNode* lpNode = apList -> m_pHead;

 while ( lpNode ) {

  if (  !compare ( lpNode -> m_pString, apStr )  ) break;

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 return lpNode;

}  /* end _find */

SMS_ListNode* SMS_ListFind ( SMS_List* apList, const char* apStr ) {

 return _find ( apList, apStr, strcmp );

}  /* end SMS_ListFind */

SMS_ListNode* SMS_ListFindI ( SMS_List* apList, const char* apStr ) {

 return _find ( apList, apStr, stricmp );

}  /* end SMS_ListFindI */

void SMS_ListAppend ( SMS_List* apDst, SMS_List* apSrc ) {

 SMS_ListNode* lpNode = apSrc -> m_pHead;

 while ( lpNode ) {

  SMS_ListPushBack ( apDst, lpNode -> m_pString ) -> m_Param = lpNode -> m_Param;

  lpNode = lpNode -> m_pNext;

 }  /* end while */

}  /* end SMS_ListAppend */

SMS_List* SMS_ListInit ( void ) {

 return ( SMS_List* )calloc (  1, sizeof ( SMS_List )  );

}  /* end StringList_Init */
