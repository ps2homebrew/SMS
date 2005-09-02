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
#include "StringList.h"

#include <malloc.h>
#include <string.h>

static void StringList_Push ( StringList* apList, const char* apString ) {

 StringListNode* lpNode = ( StringListNode* )calloc (  1, sizeof ( StringListNode ) + strlen ( apString ) + 1  );

 lpNode -> m_pString = (  ( char* )lpNode  ) + sizeof ( StringListNode );
 strcpy ( lpNode -> m_pString, apString );

 lpNode -> m_pNext = apList -> m_pHead;

 if ( apList -> m_pHead )

  apList -> m_pHead -> m_pPrev = lpNode;

 else apList -> m_pTail = lpNode;

 apList -> m_pHead = lpNode;

}  /* end StringList_Push */

static void StringList_PushBack ( StringList* apList, const char* apString ) {

 StringListNode* lpNode = ( StringListNode* )calloc (  1, sizeof ( StringListNode ) + strlen ( apString ) + 1  );

 lpNode -> m_pString = (  ( char* )lpNode  ) + sizeof ( StringListNode );
 strcpy ( lpNode -> m_pString, apString );

 if ( apList -> m_pTail ) {

  lpNode -> m_pPrev            = apList -> m_pTail;
  apList -> m_pTail -> m_pNext = lpNode;
  apList -> m_pTail            = lpNode;

 } else apList -> m_pHead = apList -> m_pTail = lpNode;

}  /* end StringList_PushBack */

static void StringList_Pop ( StringList* apList ) {

 if ( apList -> m_pHead ) {

  StringListNode* lpNode = apList -> m_pHead -> m_pNext;

  free ( apList -> m_pHead );

  apList -> m_pHead = lpNode;

  if ( !lpNode ) apList -> m_pTail = NULL;

 }  /* end if */

}  /* end StringList_Pop */

static void StringList_PopBack ( StringList* apList ) {

 if ( apList -> m_pTail ) {

  StringListNode* lpNode = apList -> m_pTail -> m_pPrev;

  free ( apList -> m_pTail );

  apList -> m_pTail = lpNode;

  if ( !lpNode )

   apList -> m_pHead = lpNode;

  else lpNode -> m_pNext = NULL;

 }  /* end if */

}  /* end StringList_PopBack */

static void StringList_Destroy ( StringList* apList, int afDestroyList ) {

 StringListNode* lpNode = apList -> m_pHead;

 while ( lpNode ) {

  StringListNode* lpNext = lpNode -> m_pNext;

  free ( lpNode );

  lpNode = lpNext;

 }  /* end while */

 if ( afDestroyList )

  free ( apList );

 else apList -> m_pHead = apList -> m_pTail = NULL;

}  /* end StringList_Destroy */

static void StringList_Sort ( StringList* apList ) {

 StringListNode* lpList = apList -> m_pHead;
 StringListNode* p;
 StringListNode* q;
 StringListNode* e;
 StringListNode* lpTail;

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

}  /* end StringList_Sort */

StringList* StringList_Init ( void ) {

 StringList* retVal = ( StringList* )calloc (  1, sizeof ( StringList )  );

 retVal -> Push     = StringList_Push;
 retVal -> PushBack = StringList_PushBack;
 retVal -> Pop      = StringList_Pop;
 retVal -> PopBack  = StringList_PopBack;
 retVal -> Destroy  = StringList_Destroy;
 retVal -> Sort     = StringList_Sort;

 return retVal;

}  /* end StringList_Init */
