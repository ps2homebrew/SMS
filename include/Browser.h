#ifndef __Browser_H
# define __Browser_H

struct GUIContext;
struct FileContext;
struct StringList;
struct CDDAContext;

typedef struct BrowserContext {

 struct FileContext* ( *Browse  ) ( char* );
 void                ( *Destroy ) ( void  );

 struct GUIContext*  m_pGUICtx;
 struct StringList*  m_pPath;
 struct CDDAContext* m_pCDDACtx;
 int                 m_HDDPD;
 int                 m_PartIdx;
 char*               m_pActivePartition;
 char*               m_pFirstPartition;
 int                 m_nAlloc;
 int                 m_nFAlloc;
 int                 m_CurDev;

} BrowserContext;

BrowserContext* BrowserContext_Init ( struct GUIContext* );
#endif  /* __Browser_H */
