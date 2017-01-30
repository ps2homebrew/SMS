#ifndef __ums_H
# define __ums_H

# include <ioman.h>

# define MAX_DEV 4

struct USBMDevice;

typedef struct CBWPack {
 unsigned int  m_Sign;
 unsigned int  m_Tag;
 unsigned int  m_DataLen;
 unsigned char m_Flags;
 unsigned char m_LUN;
 unsigned char m_CmdLen;
 unsigned char m_CmdData[ 16 ];
} CBWPack;

typedef struct USBMFSData {
 int  ( *open    ) ( struct USBMDevice*, iop_file_t*, const char*, int );
 int  ( *close   ) ( iop_file_t*                                       );
 int  ( *read    ) ( iop_file_t*, void*, int                           );
 int  ( *seek    ) ( iop_file_t*, unsigned int, int                    );
 int  ( *dopen   ) ( struct USBMDevice*, iop_file_t*, const char*      );
 int  ( *dclose  ) ( iop_file_t*                                       );
 int  ( *dread   ) ( iop_file_t*, void*                                );
 int  ( *Destroy ) ( void*                                             );
 void* m_pData;
} USBMFSData;

typedef struct USBMCacheRec {
 unsigned int m_Sector;
 int          m_Tax;
} USBMCacheRec;

typedef struct USBMDevice {

 int            m_UnitID;
 int            m_DevID;
 int            m_InpEP;
 int            m_OutEP;
 int            m_CtlEP;
 int            m_InpEPAddr;
 int            m_OutEPAddr;
 int            m_InpPktSize;
 int            m_OutPktSize;
 int            m_IntNum;
 int            m_IntAlt;
 int            m_CfgID;
 int            m_SectorSize;
 int            m_MaxLBA;
 int            m_MaxLUN;
 char           m_LUName[ 16 ][ 19 ];
 int            m_Sync;
 int            m_Lock;
 int            m_ThreadID;
 int            m_Status;
 int            m_nBytes;
 unsigned char  m_fEE;
 int            m_Tag;
 CBWPack        m_CBW;
 USBMCacheRec   m_Cache[ 4 ];
 unsigned char* m_pCacheBuf;
 unsigned char* m_pSector;
 unsigned int   m_IdxLimit;
 USBMFSData     m_FS;

} USBMDevice;

typedef struct _u16 {
 unsigned short m_Val __attribute__(  ( packed )  );
} _u16;

typedef struct _u32 {
 unsigned int m_Val __attribute__(  ( packed )  );
} _u32;

extern USBMDevice g_DevList[ MAX_DEV ];

# ifdef _DEBUG
void Log      ( const char* );
void FlushLog ( void        );
# endif  /* _DEBUG */

#endif  /* __ums_H */
