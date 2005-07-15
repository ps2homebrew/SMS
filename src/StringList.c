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

StringList* StringList_Init ( void ) {

 StringList* retVal = ( StringList* )calloc (  1, sizeof ( StringList )  );

 retVal -> Push     = StringList_Push;
 retVal -> PushBack = StringList_PushBack;
 retVal -> Pop      = StringList_Pop;
 retVal -> PopBack  = StringList_PopBack;
 retVal -> Destroy  = StringList_Destroy;

 return retVal;

}  /* end StringList_Init */
