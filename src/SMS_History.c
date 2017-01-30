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
#include "SMS.h"
#include "SMS_History.h"
#include "SMS_List.h"
#include "SMS_MC.h"

#include <fileio.h>
#include <string.h>

#define HIST_SIZE 32

static SMS_List* s_pHst;

static char s_pHistory[] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "SMS/SMS.hst";

void SMS_HistoryLoad ( void ) {

 int lFD = MC_OpenS ( g_MCSlot, 0, s_pHistory, O_RDONLY );

 s_pHst = SMS_ListInit ();

 if ( lFD < 0 ) return;

 while ( 1 ) {

  unsigned short lSize;
  SMS_ListNode*  lpNode;

  if (  MC_ReadS ( lFD, &lSize, 2 ) == 2  ) {
   lpNode = SMS_ListPushBackBuf ( s_pHst, lSize + 1 );
   MC_ReadS (  lFD, _STR( lpNode ), lSize  );
   MC_ReadS (  lFD, &lpNode -> m_Param, 8  );
  } else break;

 }  /* end while */

 MC_CloseS ( lFD );

}  /* end SMS_HistoryLoad */

long SMS_HistoryLook ( const char* apPath, void** appNode ) {

 long          retVal = -1;
 SMS_ListNode* lpNode = s_pHst -> m_pHead;

 while ( lpNode ) {
  if (   !strcmp (  apPath, _STR( lpNode )  )   ) {
   retVal = lpNode -> m_Param;
   if ( appNode ) appNode[ 0 ] = lpNode;
   break;
  }  /* end if */
  lpNode = lpNode -> m_pNext;
 }  /* end while */

 return retVal;

}  /* end SMS_HistoryLook */

void SMS_HistoryAdd ( const char* apPath, long aPTS ) {

 SMS_ListNode* lpNode;

 if (   SMS_HistoryLook (  apPath, ( void** )&lpNode  ) != -1   )
  lpNode -> m_Param = aPTS;
 else {
  SMS_ListPushBack ( s_pHst, apPath ) -> m_Param = aPTS;
  if ( s_pHst -> m_Size > HIST_SIZE ) SMS_ListPop ( s_pHst );
 }  /* end else */

}  /* end SMS_HistoryAdd */

void SMS_HistorySave ( void ) {

 int  lFD;
 char lPath[ 32 ];

 lPath[ 0 ] = 'm';
 lPath[ 1 ] = 'c';
 lPath[ 2 ] = '0' + g_MCSlot;
 lPath[ 3 ] = ':';
 lPath[ 4 ] = '/';
 strcpy ( &lPath[ 5 ], s_pHistory );

 lFD = fioOpen ( lPath, O_CREAT | O_WRONLY );

 if ( lFD >= 0 ) {

  SMS_ListNode* lpNode = s_pHst -> m_pHead;

  while ( lpNode ) {
   unsigned short lLen = strlen (  _STR( lpNode )  );
   fioWrite (  lFD, &lLen,              2 );
   fioWrite (  lFD, _STR( lpNode ), lLen  );
   fioWrite (  lFD, &lpNode -> m_Param, 8 );
   lpNode = lpNode -> m_pNext;
  }  /* end while */

  fioClose ( lFD );

 }  /* end if */

}  /* end SMS_HistorySave */

int SMS_HistoryRemove ( const char* apPath ) {

 SMS_ListNode* lpNode;

 if (   SMS_HistoryLook (  apPath, ( void** )&lpNode  ) == -1   ) return 0;

 SMS_ListRemove ( s_pHst, lpNode );

 return 1;

}  /* end SMS_HistoryRemove */
