/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_DirTree_H
#define __SMS_DirTree_H

typedef struct SMS_DirNode {

 struct SMS_DirNode* m_pNext;
 u64                 m_Param;

} SMS_DirNode;

typedef struct SMS_Dir {

 SMS_DirNode* m_pHead;
 SMS_DirNode* m_pTail;

} SMS_Dir;

typedef struct SMS_DirTree {

 SMS_Dir       m_Root;
 u64           m_Size;
 u64           m_nDirs;
 u64           m_nFiles;
 int           m_Error;
 void*         m_pParam;
 int           ( *DirCB ) ( const char* );
 int           ( *BrkCB ) ( void        );
 void*         m_pUserData;

} SMS_DirTree;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

SMS_DirTree* SMS_DirTreeInit    ( const char* );
void         SMS_DirTreeScan    ( SMS_DirTree*, SMS_Dir*, const char* );
void         SMS_DirTreeWalk    ( SMS_DirTree*, SMS_Dir*, const char*, void ( *Callback ) ( SMS_DirTree*, const char*, int, unsigned )  );
void         SMS_DirTreeDestroy ( void* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_DirTree_H */
