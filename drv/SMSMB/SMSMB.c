/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006-2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# Note: I've created this stuff using different sources (
#       mainly specs, but also some source code (I don't
#       remember which one anymore)). If someone feel abused
#       with aforementioned copyright note then jus let me
#       know ;)
#
*/
#include <irx.h>
#include <errno.h>
#include <loadcore.h>
#include <ps2ip.h>
#include <intrman.h>
#include <sifcmd.h>
#include <sifman.h>
#include <sysclib.h>
#include <thbase.h>
#include <thsemap.h>
#include <ioman_mod.h>

#include "../../include/SMS_SMB.h"
#include "../SMSUTILS/smsutils.h"

#define SMB_MAX_SERVERS 2
#define SMB_MAX_MOUNT   2
#define SMB_MAX_FILES   4

#define SMB_DEF_IDF                0x424D53FF
#define SMB_MAX_XMIT               16384

#define NB_SESSION_MESSAGE         0x00
#define NB_SESSION_REQ             0x81
#define NB_SESSION_RESP_OK         0x82
#define NB_SESSION_RESP_KEEP_ALIVE 0x85

#define SMB_NEGOTIATE_PROTOCOL     0x72
#define SMB_DIALECT_ID             0x02
#define SMB_SEC_USER_MASK          0x01
#define SMB_SEC_ENCRYPT_MASK       0x02
#define SMB_SESSSETUPX             0x73

#define SMB_TREE_CONNECT           0x70
#define SMB_TREE_CONNECTX          0x75
#define SMB_TREE_DISCONNECT        0x71

#define SMB_TRANSACT               0x25
#define SMB_TRANSACT2              0x32
#define SMB_TRANSACT2_FINDFIRST    0x01
#define SMB_TRANSACT2_FINDNEXT     0x02

#define SMB_FFF_CLOSE_AFTER_FIRST  0x0001
#define SMB_FFF_CLOSE_IF_END       0x0002
#define SMB_FFF_REQUIRE_RESUME_KEY 0x0004
#define SMB_FFF_CONTINUE_BIT       0x0008

#define SMB_ATTR_RDONLY            0x0001
#define SMB_ATTR_HIDDEN            0x0002
#define SMB_ATTR_SYSTEM            0x0004
#define SMB_ATTR_VOLID             0x0008
#define SMB_ATTR_DIR               0x0010
#define SMB_ATTR_ARCH              0x0020

#define SMB_OPEN                   0x02
#define SMB_CLOSE                  0x04
#define SMB_LSEEK                  0x12
#define SMB_READ                   0x0A
#define SMB_READ_RAW               0x1A
#define SMB_ECHO                   0x2B

#define CAP_RAW_MODE               0x0001

#define CVAL( b, p ) (  ( unsigned char* )( b )  )[ p ]
#define IVAL( p, o ) (  ( U32* )&p[ o ]  ) -> m_Val
#define PVAL( b, p ) ( unsigned int )CVAL( b, p )
#define SVAL( p, o ) (  ( U16* )&p[ o ]  ) -> m_Val
#define ALIGN( x )   (   (  ( x ) + 3  ) & ~3   )

#define NB_PKT_LEN( p ) (    PVAL( p, 3 ) | (  PVAL( p, 2 ) << 8  ) | (  (  PVAL( p, 1 ) & 0x01 ) << 16  )    )
#define NB_SET_PKT_LEN( p, l ) ( p )[ 1 ] = (  ( l ) >> 16 ) & 1;    \
                               ( p )[ 2 ] = (  ( l ) >>  8 ) & 0xFF; \
                               ( p )[ 3 ] = ( l ) & 0xFF;
#define NB_PKT_TYPE( p ) CVAL( p, 0 )

typedef struct SMBFileInfo {

 short         m_CreaDate    __attribute__(  ( packed )  );
 short         m_CreaTime    __attribute__(  ( packed )  );
 short         m_LastAccDate __attribute__(  ( packed )  );
 short         m_LastAccTime __attribute__(  ( packed )  );
 short         m_LastModDate __attribute__(  ( packed )  );
 short         m_LastModTime __attribute__(  ( packed )  );
 unsigned int  m_Size        __attribute__(  ( packed )  );
 unsigned int  m_AllocSize   __attribute__(  ( packed )  );
 short         m_Attr        __attribute__(  ( packed )  );
 unsigned char m_NameLen     __attribute__(  ( packed )  );
 char          m_Name[ 1 ]   __attribute__(  ( packed )  );

} SMBFileInfo __attribute__(  ( packed )  );

typedef struct SMBFindContext {

 struct SMBMountContext* m_pCtx;

 short           m_DirHandle;
 unsigned char   m_MaskLen;
 char            m_Mask[ 256 ];

} SMBFindContext;

typedef struct SMBFindFirstParam {

 short m_Handle     __attribute__(  ( packed )  );
 short m_Count      __attribute__(  ( packed )  );
 short m_EOS        __attribute__(  ( packed )  );
 short m_ErrOffset  __attribute__(  ( packed )  );
 short m_NameOffset __attribute__(  ( packed )  );

} SMBFindFirstParam __attribute__(  ( packed )  );

typedef struct SMBFindNextParam {

 short m_Count      __attribute__(  ( packed )  );
 short m_EOS        __attribute__(  ( packed )  );
 short m_ErrOffset  __attribute__(  ( packed )  );
 short m_NameOffset __attribute__(  ( packed )  );

} SMBFindNextParam __attribute__(  ( packed )  );

typedef struct SMBNetShareEnumParam {

 unsigned short m_ErrorCode    __attribute__(   ( packed )  );
 unsigned short m_Converter    __attribute__(   ( packed )  );
 unsigned short m_EntryCount   __attribute__(   ( packed )  );
 unsigned short m_TotalEntries __attribute__(   ( packed )  );

} SMBNetShareEnumParam __attribute__(   ( packed )  );

typedef struct SMBFileContext {

 unsigned char m_ReadPkt[ 56 ] __attribute__(   (  aligned( 4 )  )   );
 unsigned char m_RespPkt[ 52 ] __attribute__(   (  aligned( 4 )  )   );

 struct SMBMountContext* m_pCtx;

 int ( *Read ) ( struct SMBFileContext*, void*, int );

 int          m_SD;
 unsigned int m_Pos;
 unsigned int m_Size;
 short        m_FD;

} SMBFileContext;

typedef struct SMBMountContext {

 struct SMBServerContext* m_pCtx;

 SMBFileContext m_FileCtx[ SMB_MAX_FILES ];

 union {
  unsigned int m_Data __attribute__(  ( packed )  );
  struct {
   unsigned short m_ID     __attribute__(  ( packed )  );
   unsigned short m_MaXMit __attribute__(  ( packed )  );
  } __attribute__(  ( packed )  );
 };

} SMBMountContext;

typedef struct SMBServerContext {

 SMBLoginInfo    m_LoginInfo;
 SMBMountContext m_MountCtx[ SMB_MAX_MOUNT ];
 int             m_LoginSema;
 int             m_Socket;
 int             m_SessionKey;
 int             m_MaXMit;
 int             m_MaxRaw;
 unsigned short  m_UID;
 unsigned short  m_Security;
 unsigned short  m_MaxMPX;
 unsigned short  m_MaxVC;
 unsigned short  m_RawSupp;
 unsigned short  m_SrvTZ;
 unsigned short  m_EncrKeyLen;
 unsigned char   m_EncrKey[ 8 ];
 unsigned char   m_PDom[ 80 ];
 unsigned char   m_fEncrPwd;
 unsigned char   m_fNT;

} SMBServerContext;

static int _DrvNOP    ( void                                  );
static int _DrvInit   ( iop_io_device_t*                      );
static int _DrvDeInit ( iop_io_device_t*                      );
static int _DrvDOpen  ( iop_io_file_t*, const char*           );
static int _DrvDClose ( iop_io_file_t*                        );
static int _DrvDRead  ( iop_io_file_t*, void*                 );
static int _DrvOpen   ( iop_io_file_t*, const char*, int, ... );
static int _DrvClose  ( iop_io_file_t*                        );
static int _DrvRead   ( iop_io_file_t*, void*, int            );
static int _DrvLSeek  ( iop_io_file_t*, unsigned long, int    );
static int _DrvIOCtl  ( iop_io_file_t*, unsigned long, void*  );

static iop_io_device_ops_t s_Driver = {
 (  int ( * ) ( iop_io_device_t*                                 )  )_DrvInit,    /* io_init    */
 (  int ( * ) ( iop_io_device_t*                                 )  )_DrvDeInit,  /* io_deinit  */
 (  int ( * ) ( iop_io_file_t*, ...                              )  )_DrvNOP,     /* io_format  */
 (  int ( * ) ( iop_io_file_t*, const char *, int, ...           )  )_DrvOpen,    /* io_open    */
 (  int ( * ) ( iop_io_file_t*                                   )  )_DrvClose,   /* io_close   */
 (  int ( * ) ( iop_io_file_t*, void*, int                       )  )_DrvRead,    /* io_read    */
 (  int ( * ) ( iop_io_file_t*, void*, int                       )  )_DrvNOP,     /* io_write   */
 (  int ( * ) ( iop_io_file_t*, unsigned long, int               )  )_DrvLSeek,   /* io_lseek   */
 (  int ( * ) ( iop_io_file_t*, unsigned long, void*             )  )_DrvIOCtl,   /* io_ioctl   */
 (  int ( * ) ( iop_io_file_t*, const char*                      )  )_DrvNOP,     /* io_remove  */
 (  int ( * ) ( iop_io_file_t*, const char*                      )  )_DrvNOP,     /* io_mkdir   */
 (  int ( * ) ( iop_io_file_t*, const char*                      )  )_DrvNOP,     /* io_rmdir   */
 (  int ( * ) ( iop_io_file_t*, const char*                      )  )_DrvDOpen,   /* io_dopen   */
 (  int ( * ) ( iop_io_file_t*                                   )  )_DrvDClose,  /* io_dclose  */
 (  int ( * ) ( iop_io_file_t*, void*                            )  )_DrvDRead,   /* io_dread   */
 (  int ( * ) ( iop_io_file_t*, const char*, void*               )  )_DrvNOP,     /* io_getstat */
 (  int ( * ) ( iop_io_file_t*, const char*, void*, unsigned int )  )_DrvNOP      /* io_chstat  */
};

intrman_IMPORTS_start
 I_CpuSuspendIntr
 I_CpuResumeIntr
intrman_IMPORTS_end

ioman_mod_IMPORTS_start
 I_io_AddDrv
ioman_mod_IMPORTS_end

ps2ip_IMPORTS_start
 I_inet_addr
 I_lwip_close
 I_lwip_connect
 I_lwip_recv
 I_lwip_send
 I_lwip_socket
ps2ip_IMPORTS_end

sifcmd_IMPORTS_start
 I_sceSifSendCmd
sifcmd_IMPORTS_end

sifman_IMPORTS_start
 I_sceSifSetDma
 I_sceSifDmaStat
sifman_IMPORTS_end

smsutils_IMPORTS_start
 I_mips_memcpy
 I_mips_memset
smsutils_IMPORTS_end

sysclib_IMPORTS_start
 I_strcat
 I_strcpy
 I_strlen
 I_strncpy
 I_toupper
sysclib_IMPORTS_end

thsemap_IMPORTS_start
 I_CreateSema
 I_DeleteSema
 I_SignalSema
 I_WaitSema
thsemap_IMPORTS_end

thbase_IMPORTS_start
 I_CreateThread
 I_DelayThread
 I_StartThread
 I_ExitDeleteThread
thbase_IMPORTS_end

#define FLAG_DOPEN            0x10000000
#define FLAG_DOPEN_1          0x20000000
#define FLAG_LOGIN_0_PROGRESS 0x01000000
#define FLAG_LOGIN_1_PROGRESS 0x02000000
#define FLAG_LOGIN_0          0x04000000
#define FLAG_LOGIN_1          0x08000000

static unsigned int      s_Flags;
static SMBServerContext  s_ServerCtx[ SMB_MAX_SERVERS                 ];
static SMBMountContext*  s_MountCtx [ SMB_MAX_SERVERS * SMB_MAX_MOUNT ];
static SMBFindContext    s_FindCtx;
static io_dirent_t       s_FileInfo;
static unsigned int      s_LastError;
static unsigned char     s_SEnumBuff[ 8192 ];

typedef struct U32 {
 int m_Val __attribute__(  ( packed )  );
} U32;

typedef struct U16 {
 short m_Val __attribute__(  ( packed )  );
} U16;

static void _nb_name ( const char* apInp, char* apOut ) {

 char lChr, lUN, lLN;
 int  i, lLen  = strlen ( apInp );

 for ( i = 0; i < 16; ++i ) {
  if ( i >= lLen ) {
   lUN = 'C';
   lLN = 'A';
  } else {
   lChr = apInp[ i ];
   lUN  = ( char )(  ( int )( lChr >> 4 ) + ( int )'A'  );
   lLN  = ( char )(  ( int )( lChr & 15 ) + ( int )'A'  );
  }  /* end else */
  apOut[ 0 ] = lUN;
  apOut[ 1 ] = lLN;
  apOut += 2;
 }  /* end for */
 apOut[ 0 ] = '\x00';

}  /* end _nb_name */

static void _nb_uni2ascii ( unsigned char* apDst, const unsigned char* apSrc, int aMaxLen ) {

 int i;

 for ( i = 0; i < aMaxLen; ++i, apSrc += 2 ) {

  if ( !apSrc[ 0 ] && !apSrc[ 1 ] ) break;

  apDst[ i ] = apSrc[ 0 ];

 }  /* end for */

}  /* end _nb_uni2ascii */

static void strupper ( char* apStr) {

 while ( *apStr ) {
  *apStr = toupper ( *apStr );
  ++apStr;
 }  /* end while */

}  /* end strupper */

static int _smb_bs_subst ( char* apPtr ) {

 char* lpPtr = apPtr;

 while ( *apPtr ) {

  if ( *apPtr == '/' ) *apPtr = '\\';
  ++apPtr;

 }  /* end while */

 return apPtr - lpPtr;

}  /* end _smb_bs_subst */

static void _smb_encr_permute ( char* apOut, const char* apIn, const char* aPtr, int aN ) {

 int i;

 for ( i = 0; i < aN; ++i ) apOut[ i ] = apIn[  aPtr[ i ] - 1  ];

}  /* end _smb_encr_permute */

static void _smb_encr_concat ( char* apOut, const char* apIn1, const char* apIn2, int aL1, int aL2 ) {

 while ( aL1-- ) *apOut++ = *apIn1++;
 while ( aL2-- ) *apOut++ = *apIn2++;

}  /* end _smb_encr_concat */

static void _smb_encr_xor ( char* apOut, const char* apIn1, const char* apIn2, int aN ) {

 int i;

 for ( i = 0; i < aN; ++i ) apOut[ i ] = apIn1[ i ] ^ apIn2[ i ];

}  /* end _smb_encr_xor */

static void _smb_encr_lshift ( char* apD, int aCount, int aN ) {

 char lOut[ 64 ];
 int  i;

 for ( i = 0; i < aN; ++i ) lOut[ i ] = apD[ ( i + aCount ) % aN ];
 for ( i = 0; i < aN; ++i ) apD [ i ] = lOut[ i ];

}  /* end _smb_encr_lshift */

static void _smb_encr_do_hash ( char* apOut, const char* apIn, const char* apKey ) {

 static char s_lPerm1[ 56 ] = {
  57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
  10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,
  63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
  14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4
 };
 static char s_lSC[ 16 ] = {
  1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
 };
 static char s_lPerm2[ 48 ] = {
  14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
  23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,
  41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
  44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
 };
 static char s_lPerm3[ 64 ] = {
  58, 50, 42, 34, 26, 18, 10,  2, 60, 52, 44, 36, 28, 20, 12, 4,
  62, 54, 46, 38, 30, 22, 14,  6, 64, 56, 48, 40, 32, 24, 16, 8,
  57, 49, 41, 33, 25, 17,  9,  1, 59, 51, 43, 35, 27, 19, 11, 3,
  61, 53, 45, 37, 29, 21, 13,  5, 63, 55, 47, 39, 31, 23, 15, 7
 };
 static char s_lPerm4[ 48 ] = {
  32,  1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,
   8,  9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
  16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
  24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32,  1
 };

 static char s_lSBox[ 8 ][ 4 ][ 16 ] = {
  {
   { 14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7 },
   {  0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8 },
   {  4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0 },
   { 15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13 }
  },
  {
   { 15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10 },
   {  3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5 },
   {  0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15 },
   { 13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9 }
  },
  {
   { 10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8 },
   { 13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1 },
   { 13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7 },
   {  1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12 }
  },
  {
   {  7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15 },
   { 13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9 },
   { 10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4 },
   {  3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14 }
  },
  {
   {  2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9 },
   { 14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6 },
   {  4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14 },
   { 11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3 }
  },
  {
   { 12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11 },
   { 10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8 },
   {  9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6 },
   {  4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13 }
  },
  {
   {  4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1 },
   { 13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6 },
   {  1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2 },
   {  6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12 }
  },
  {
   { 13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7 },
   {  1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2 },
   {  7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8 },
   {  2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11 }
  }
 };
 static char s_lPerm5[ 32 ] = {
  16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,  5, 18, 31, 10,
   2,  8, 24, 14, 32, 27,  3,  9, 19, 13, 30,  6, 22, 11,  4, 25
 };
 static char s_lPerm6[ 64 ] = {
  40,  8, 48, 16, 56, 24, 64, 32, 39,  7, 47, 15, 55, 23, 63, 31,
  38,  6, 46, 14, 54, 22, 62, 30, 37,  5, 45, 13, 53, 21, 61, 29,
  36,  4, 44, 12, 52, 20, 60, 28, 35,  3, 43, 11, 51, 19, 59, 27,
  34,  2, 42, 10, 50, 18, 58, 26, 33,  1, 41,  9, 49, 17, 57, 25
 };

 int  i, j, k;
 char lPK1[ 56 ];
 char lC[ 28 ];
 char lD[ 28 ];
 char lCD[ 56 ];
 char lKI[ 16 ][ 48 ];
 char lPD1[ 64 ];
 char lL[ 32 ], lR[ 32 ];
 char lRL[ 64 ];

 _smb_encr_permute ( lPK1, apKey, s_lPerm1, 56 );

 for ( i = 0; i < 28; ++i ) lC[ i ] = lPK1[ i      ];
 for ( i = 0; i < 28; ++i ) lD[ i ] = lPK1[ i + 28 ];

 for ( i = 0; i < 16; ++i ) {

  _smb_encr_lshift ( lC, s_lSC[ i ], 28 );
  _smb_encr_lshift ( lD, s_lSC[ i ], 28 );

  _smb_encr_concat ( lCD, lC, lD, 28, 28 );
  _smb_encr_permute ( lKI[ i ], lCD, s_lPerm2, 48 ); 

 }  /* end for */

 _smb_encr_permute ( lPD1, apIn, s_lPerm3, 64 );

 for ( j = 0; j < 32; ++j ) {
  lL[ j ] = lPD1[ j      ];
  lR[ j ] = lPD1[ j + 32 ];
 }  /* end for */

 for ( i = 0; i < 16; ++i ) {

  char lER[ 48 ];
  char lERK[ 48 ];
  char lB[ 8 ][ 6 ];
  char lCB[ 32 ];
  char lPCB[ 32 ];
  char lR2[ 32 ];

  _smb_encr_permute ( lER, lR, s_lPerm4, 48 );
  _smb_encr_xor ( lERK, lER, lKI[ i ], 48 );

  for ( j = 0; j < 8; ++j )
   for ( k = 0; k < 6; ++k ) lB[ j ][ k ] = lERK[ j * 6 + k ];

  for ( j = 0; j < 8; ++j ) {

   int lM = ( lB[ j ][ 0 ] << 1 ) | lB[ j ][ 5 ];
   int lN = ( lB[ j ][ 1 ] << 3 ) | ( lB[ j ][ 2 ] << 2 ) | ( lB[ j ][ 3 ] << 1 ) | lB[ j ][ 4 ]; 

   for ( k = 0; k < 4; ++k ) lB[ j ][ k ] = (  s_lSBox[ j ][ lM ][ lN ] & (  1 << ( 3 - k )  )   ) ? 1 : 0; 

  }  /* end for */

  for ( j = 0; j < 8; ++j )
   for ( k = 0; k < 4; ++k ) lCB[ j * 4 + k ] = lB[ j ][ k ];

  _smb_encr_permute ( lPCB, lCB, s_lPerm5, 32 );
  _smb_encr_xor ( lR2, lL, lPCB, 32 );

  for ( j = 0; j < 32; ++j ) lL[ j ] = lR [ j ];
  for ( j = 0; j < 32; ++j ) lR[ j ] = lR2[ j ];

 }  /* end for */

 _smb_encr_concat ( lRL, lR, lL, 32, 32 );
 _smb_encr_permute ( apOut, lRL, s_lPerm6, 64 );

}  /* end _smb_encr_do_hash */

static void inline _smb_encr_str2key ( const unsigned char* apStr, unsigned char* apKey ) {

 int i;

 apKey[ 0 ] = apStr[ 0 ] >> 1;
 apKey[ 1 ] = (  ( apStr[ 0 ] & 0x01 ) << 6  ) | ( apStr[ 1 ] >> 2 );
 apKey[ 2 ] = (  ( apStr[ 1 ] & 0x03 ) << 5  ) | ( apStr[ 2 ] >> 3 );
 apKey[ 3 ] = (  ( apStr[ 2 ] & 0x07 ) << 4  ) | ( apStr[ 3 ] >> 4 );
 apKey[ 4 ] = (  ( apStr[ 3 ] & 0x0F ) << 3  ) | ( apStr[ 4 ] >> 5 );
 apKey[ 5 ] = (  ( apStr[ 4 ] & 0x1F ) << 2  ) | ( apStr[ 5 ] >> 6 );
 apKey[ 6 ] = (  ( apStr[ 5 ] & 0x3F ) << 1  ) | ( apStr[ 6 ] >> 7 );
 apKey[ 7 ] = apStr[ 6 ] & 0x7F;

 for ( i = 0; i < 8; ++i ) apKey[ i ] = apKey[ i ] << 1;

}  /* end _smb_encr_str2key */

static void _smb_encr_hash ( unsigned char* apOut, const unsigned char* apIn, const unsigned char* apKey ) {

 int i;
 char lOutb[ 64 ];
 char lInb[ 64 ];
 char lKeyb[ 64 ];
 unsigned char lKey2[ 8 ];

 _smb_encr_str2key ( apKey, lKey2 );

 for ( i = 0; i < 64; ++i ) {
  lInb [ i ] = (    apIn [ i / 8 ] & (   1 << (  7 - ( i % 8 )  )   )    ) ? 1 : 0;
  lKeyb[ i ] = (    lKey2[ i / 8 ] & (   1 << (  7 - ( i % 8 )  )   )    ) ? 1 : 0;
  lOutb[ i ] = 0;
 }  /* end for */

 _smb_encr_do_hash ( lOutb, lInb, lKeyb );

 for ( i = 0; i < 8; ++i ) apOut[ i ] = 0;

 for ( i = 0; i < 64; ++i )
  if ( lOutb[ i ] ) apOut[ i / 8 ] |= (   1  << (  7 - ( i % 8 )  )   );

}  /* end _smb_encr_hash */

static void _smb_encr16 ( const unsigned char* ap14, unsigned char* ap16 ) {

 static unsigned char s_lSP8[ 8 ] = {
  0x4B, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25
 };

 _smb_encr_hash ( ap16,     s_lSP8, ap14     );
 _smb_encr_hash ( ap16 + 8, s_lSP8, ap14 + 7 );

}  /* end _smb_encr16 */

static void _smb_encr24 (
             const unsigned char* ap21,
             const unsigned char* apC8,
             unsigned char*       ap24
            ) {

 _smb_encr_hash ( ap24,      apC8, ap21      );
 _smb_encr_hash ( ap24 +  8, apC8, ap21 +  7 );
 _smb_encr_hash ( ap24 + 16, apC8, ap21 + 14 );

}  /* end _smb_encr24 */

static void _smb_date_dos2ps2 ( short aDate, short aTime, unsigned char* apBuff ) {

 apBuff[ 1 ] = ( aTime & 0x1F ) << 1;
 apBuff[ 2 ] = ( aTime >> 5 ) & 0x3F;
 apBuff[ 3 ] = aTime >> 11;
 apBuff[ 4 ] = aDate & 0x1F;
 apBuff[ 5 ] = (  ( aDate >> 5 ) & 0x0F  ) - 1;
 SVAL( apBuff, 6 ) = ( aDate >> 9 ) + 1980;

}  /* end _smb_date_dos2ps2 */

static void _smb_file_info_dos2ps2 ( io_dirent_t* apPS2Info, SMBFileInfo* apDOSInfo ) {

 int lLen = apDOSInfo -> m_NameLen;

 apPS2Info -> stat.mode = apDOSInfo -> m_Attr & SMB_ATTR_DIR ? FIO_SO_IFDIR : FIO_SO_IFREG;
 apPS2Info -> stat.attr = 0;
 apPS2Info -> stat.size = apDOSInfo -> m_Size;
 _smb_date_dos2ps2 ( apDOSInfo -> m_CreaDate,    apDOSInfo -> m_CreaTime,    apPS2Info -> stat.ctime );
 _smb_date_dos2ps2 ( apDOSInfo -> m_LastAccDate, apDOSInfo -> m_LastAccTime, apPS2Info -> stat.atime );
 _smb_date_dos2ps2 ( apDOSInfo -> m_LastModDate, apDOSInfo -> m_LastModTime, apPS2Info -> stat.mtime );
 apPS2Info -> stat.hisize = 0;

 strncpy ( apPS2Info -> name, apDOSInfo -> m_Name, lLen );
 apPS2Info -> name[ lLen ] = '\x00';

}  /* end _smb_file_info_dos2ps2 */

static int _nb_send_packet ( int aSD, unsigned char* apBuff, int aLen ) {

 s_LastError           = 0;
 apBuff[ 0 ]           = NB_SESSION_MESSAGE;
 *( int* )&apBuff[ 4 ] = SMB_DEF_IDF;
 NB_SET_PKT_LEN( apBuff, aLen );
 aLen += 4;

 return lwip_send ( aSD, apBuff, aLen, 0 ) == aLen;

}  /* end _nb_send_packet */

static int _nb_read_packet ( int aSD, unsigned char* apBuff, int aLen ) {

 int retVal      = 0;
 int lfKeepAlive = 1;
 int lnRead;
 int lPktLen;

 while ( lfKeepAlive ) {

  lnRead = lwip_recv ( aSD, apBuff, 4, 0 );

  if ( !lnRead ) return 0;

  lfKeepAlive = NB_PKT_TYPE( apBuff ) == NB_SESSION_RESP_KEEP_ALIVE;

 }  /* end while */

 lPktLen = NB_PKT_LEN( apBuff );
 retVal += lnRead;
 apBuff += lnRead;
 aLen   -= lnRead;

 if ( lnRead < 4 ) return retVal;

 if ( lPktLen > aLen ) lPktLen = aLen;

 while ( lPktLen ) {

  lnRead = lwip_recv ( aSD, apBuff, lPktLen, 0 );

  if ( !lnRead ) break;

  lPktLen -= lnRead;
  apBuff  += lnRead;
  retVal  += lnRead;

 }  /* end while */

 return retVal;

}  /* end _nb_read_packet */

static void SMB_Encrypt (
        const unsigned char* apPasswd, unsigned char* apKey, unsigned char* apRes
       ) {

 unsigned char lP14[ 15 ];
 unsigned char lP21[ 21 ];

 mips_memset ( lP14, '\x00', 15 );
 mips_memset ( lP21, '\x00', 21 );

 strncpy ( lP14, apPasswd, 14 );
 strupper ( lP14 );

 _smb_encr16 ( lP14, lP21 ); 
 _smb_encr24 ( lP21, apKey, apRes );

}  /* end SMB_Encrypt */

static int SMB_Negotiate ( SMBServerContext* apCtx ) {

 static char* s_SMBProts[] = {
  "LANMAN1.0",
  "LM1.2X002",
  "Samba",
  "NT LM 0.12",
  "NT LANMAN 1.0",
  NULL
 };

 int i, lPktLen, lRespLen, lProtLen, retVal = 0;

 for ( i = 0, lProtLen = 0; s_SMBProts[ i ]; ++i ) lProtLen += strlen ( s_SMBProts[ i ] ) + 2;

 lPktLen  = lProtLen + 35;
 lRespLen = ( lPktLen < 128 ? 128 : lPktLen ) + 4;

 {  /* begin block */

  unsigned char lBuff[ lRespLen ] __attribute__(   (  aligned( 4 )  )   );
  char*         lpPtr = &lBuff[ 39 ];
  int           lSD   = apCtx -> m_Socket;

  mips_memset ( lBuff, 0, 39 );
  lBuff[ 8 ]        = SMB_NEGOTIATE_PROTOCOL;
  SVAL( lBuff, 37 ) = lProtLen;

  for ( i = 0; s_SMBProts[ i ]; ++i ) {
   lpPtr[ 0 ] = SMB_DIALECT_ID;
   strcpy ( lpPtr + 1, s_SMBProts[ i ] );
   lpPtr += strlen ( s_SMBProts[ i ] ) + 2;
  }  /* end for */

  if (  _nb_send_packet ( lSD, lBuff, lPktLen )  ) {

   lRespLen = _nb_read_packet ( lSD, lBuff, lRespLen );

   if ( lRespLen && !lBuff[ 9 ] ) {

    unsigned short lProtOffset = SVAL( lBuff, 37 );

    if ( lProtOffset != 0xFFFF ) {

     apCtx -> m_fNT = lProtOffset > 1;

     switch (  CVAL( lBuff, 36 )  ) {

      case  1: retVal = 1; break;
      case 13: {

       lProtOffset = SVAL( lBuff, 39 );

       apCtx -> m_fEncrPwd   = lProtOffset & SMB_SEC_ENCRYPT_MASK;
       apCtx -> m_Security   = lProtOffset & SMB_SEC_USER_MASK;
       apCtx -> m_MaXMit     = SVAL( lBuff, 41 );
       apCtx -> m_MaxMPX     = SVAL( lBuff, 43 );
       apCtx -> m_MaxVC      = SVAL( lBuff, 45 );
       apCtx -> m_RawSupp    = SVAL( lBuff, 47 );
       apCtx -> m_SessionKey = IVAL( lBuff, 49 );
       apCtx -> m_SrvTZ      = SVAL( lBuff, 57 );
       apCtx -> m_EncrKeyLen = SVAL( lBuff, 59 );
       mips_memcpy ( apCtx -> m_EncrKey, &lBuff[ 65 ], 8 );
       strncpy (  apCtx -> m_PDom, &lBuff[ 65 + apCtx -> m_EncrKeyLen ], sizeof ( apCtx -> m_PDom )  );

       retVal = 1;

      } break;

      case 17: {

       lProtOffset = SVAL( lBuff, 39 );

       apCtx -> m_fEncrPwd   = lProtOffset & SMB_SEC_ENCRYPT_MASK;
       apCtx -> m_Security   = lProtOffset & SMB_SEC_USER_MASK;
       apCtx -> m_MaxMPX     = SVAL( lBuff, 40 );
       apCtx -> m_MaxVC      = SVAL( lBuff, 42 );
       apCtx -> m_MaXMit     = IVAL( lBuff, 44 );
       apCtx -> m_MaxRaw     = IVAL( lBuff, 48 );
       apCtx -> m_SessionKey = IVAL( lBuff, 52 );
       apCtx -> m_RawSupp    = IVAL( lBuff, 56 ) & CAP_RAW_MODE;
       apCtx -> m_SrvTZ      = SVAL( lBuff, 68 );
       apCtx -> m_EncrKeyLen = CVAL( lBuff, 70 );
       mips_memcpy ( apCtx -> m_EncrKey, &lBuff[ 73 ], 8 );
       _nb_uni2ascii (
        apCtx -> m_PDom, &lBuff[ 73 + apCtx -> m_EncrKeyLen ], sizeof ( apCtx -> m_PDom )
       );

       retVal = 1;

      } break;

     }  /* end switch */

    }  /* end if */

   } else s_LastError = SVAL( lBuff, 11 );

  }  /* end if */

 }  /* end block */

 return retVal;

}  /* end SMB_Negotiate */

static int SMB_Read ( SMBFileContext* apFileCtx, void* apBuffer, int anBytes ) {

 unsigned char* lpPkt     = apFileCtx -> m_ReadPkt;
 unsigned char* lpRespPkt = apFileCtx -> m_RespPkt;
 int            lSD       = apFileCtx -> m_SD;
 unsigned int   lPos      = apFileCtx -> m_Pos;
 int            lTotal    = 0;

 while ( anBytes ) {

  SVAL( lpPkt, 39 ) = anBytes;
  IVAL( lpPkt, 41 ) = lPos;

  if (  _nb_send_packet ( lSD, lpPkt, 48 )  ) {

   if (  _nb_read_packet ( lSD, lpRespPkt, 52 ) && !lpRespPkt[ 9 ]  ) {

    int lnRem = SVAL( lpRespPkt, 37 );

    while ( lnRem ) {

     int lnRead = lwip_recv ( lSD, apBuffer, lnRem, 0 );

     if ( lnRead <= 0 ) break;

     apBuffer = ( char* )apBuffer + lnRead;
     anBytes -= lnRead;
     lnRem   -= lnRead;
     lTotal  += lnRead;
     lPos    += lnRead;

    }  /* end while */

   }  /* end if */

  }  /* end while */

  apFileCtx -> m_Pos = lPos;

 }  /* end if */

 return lTotal;

}  /* end SMB_Read */

static int SMB_ReadRaw ( SMBFileContext* apFileCtx, void* apBuffer, int anBytes ) {

 unsigned char* lpPkt  = apFileCtx -> m_ReadPkt;
 int            lSD    = apFileCtx -> m_SD;
 unsigned int   lPos   = apFileCtx -> m_Pos;
 int            lTotal = 0;

 IVAL( lpPkt, 39 ) = lPos;
 SVAL( lpPkt, 43 ) = anBytes;

 if (   _nb_send_packet (  lSD, lpPkt, sizeof ( apFileCtx -> m_ReadPkt )  )   ) {

  int lnRem;
  int lnRead;
again:
  lnRead = lwip_recv (  lSD, ( void* )&lnRem, 4, 0  );

  if ( lnRead == 4 ) {

   if (  NB_PKT_TYPE( &lnRem ) == NB_SESSION_RESP_KEEP_ALIVE  ) goto again;

   lnRem = NB_PKT_LEN( &lnRem );

   while (   lnRem > 0 && (  lnRead = lwip_recv ( lSD, apBuffer, lnRem, 0 )  ) > 0   ) {

    apBuffer  = ( char* )apBuffer + lnRead;
    lnRem    -= lnRead;
    lTotal   += lnRead;
    lPos     += lnRead;

   }  /* end while */

  }  /* end if */

  apFileCtx -> m_Pos = lPos;

 }  /* end if */

 return lTotal;

}  /* end SMB_ReadRaw */

static int SMB_Login ( SMBServerContext* apCtx ) {

 int   retVal = 0;
 int   lPwdLen;
 int   lUsrLen;
 int   lDomLen;
 int   lParLen;
 int   lPktLen;
 char* lpPtr;
 char  lPwd[ 128 ];
 char  lPkt[ 256 ] __attribute__(   (  aligned( 4 )  )   );

 mips_memset (  lPkt, 0, sizeof ( lPkt )  );

 if ( apCtx -> m_fEncrPwd ) {
  lPwdLen = 24;
  SMB_Encrypt ( apCtx -> m_LoginInfo.m_Password, apCtx -> m_EncrKey, lPwd ); 
 } else {
  lPwdLen = strlen ( apCtx -> m_LoginInfo.m_Password );
  strcpy ( lPwd, apCtx -> m_LoginInfo.m_Password );
 }  /* end else */

 lParLen = (  lUsrLen = strlen ( apCtx -> m_LoginInfo.m_UserName )  ) + lPwdLen +
           (  lDomLen = strlen ( apCtx -> m_PDom )  ) + 8;

 lPkt[  8 ]       = SMB_SESSSETUPX;
 lPkt[ 37 ]       = '\xFF';
 SVAL( lPkt, 41 ) = SMB_MAX_XMIT;

 if ( !apCtx -> m_fNT ) {

  ++lParLen;
  lPkt[ 36 ]       = 10;
  lPkt[ 43 ]       = 2;
  SVAL( lPkt, 51 ) = lPwdLen + 1;
  SVAL( lPkt, 57 ) = lParLen;

  mips_memcpy ( lpPtr = &lPkt[ 59 ], lPwd, lPwdLen );
  strcpy ( lpPtr += lPwdLen + 1, apCtx -> m_LoginInfo.m_UserName );
  strcpy ( lpPtr += lUsrLen + 1, apCtx -> m_PDom );
  strcpy ( lpPtr +  lDomLen + 1, "PS2OS" );

  lPktLen = 55 + lParLen;

 } else {

  lParLen         +=  4;
  lPkt[ 36 ]       = 13;
  SVAL( lPkt, 51 ) = lPwdLen;
  SVAL( lPkt, 63 ) = lParLen;

  mips_memcpy ( lpPtr = &lPkt[ 65 ], lPwd, lPwdLen );
  strcpy ( lpPtr += lPwdLen, apCtx -> m_LoginInfo.m_UserName );
  strcpy ( lpPtr += lUsrLen + 1, apCtx -> m_PDom );
  strcpy ( lpPtr += lDomLen + 1, "PS2OS" );
  strcpy ( lpPtr + 6, "PS2" );

  lPktLen = 61 + lParLen;

 }  /* end else */

 if (  _nb_send_packet ( lParLen = apCtx -> m_Socket, lPkt, lPktLen )  ) {

  lPktLen = _nb_read_packet ( lParLen, lPkt, 256 );

  if ( lPktLen && !lPkt[ 9 ] ) {

   int ( *Read ) ( SMBFileContext*, void*, int ) = apCtx -> m_RawSupp ? SMB_ReadRaw : SMB_Read;

   apCtx -> m_UID = SVAL( lPkt, 32 );
   retVal         = 1;

   for ( lPwdLen = 0; lPwdLen < SMB_MAX_MOUNT; ++lPwdLen )
    for ( lPktLen = 0; lPktLen < SMB_MAX_FILES; ++lPktLen )
     apCtx -> m_MountCtx[ lPwdLen ].m_FileCtx[ lPktLen ].Read = Read;

  } else s_LastError = SVAL( lPkt, 11 );

 }  /* end if */

 return retVal;

}  /* end SMB_Login */

static int SMB_Mount ( SMBServerContext* apServerCtx, SMBMountContext* apMountCtx, const char* apShare ) {

 unsigned char lPkt[ 768 ] __attribute__(   (  aligned( 4 )  )   );
 int           lLen, lSD, retVal = 0;

 mips_memset (  lPkt, 0, sizeof ( lPkt )  );

 lPkt[  8 ]             = SMB_TREE_CONNECTX;
 *( short* )&lPkt[ 32 ] = apServerCtx -> m_UID;
 lPkt[ 36 ]             =    4;
 lPkt[ 37 ]             = 0xFF;
 SVAL( lPkt, 43 )       =    1;
 strcpy ( &lPkt[ 48 ], "\\\\"  );
 strcat ( &lPkt[ 48 ], apServerCtx -> m_LoginInfo.m_ServerName  );
 strcat ( &lPkt[ 48 ], "\\"    );
 strcat ( &lPkt[ 48 ], apShare );
 SVAL( lPkt, 45 )       = (  lLen = strlen ( &lPkt[ 48 ] )  ) + 8;
 strcpy (  &lPkt[ 49 + lLen ], "?????" );
 lLen += 51;

 if (  _nb_send_packet ( lSD = apServerCtx -> m_Socket, lPkt, lLen )  ) {

  lLen = _nb_read_packet ( lSD, lPkt, 768 );

  if ( lLen && !lPkt[ 9 ] ) {

   apMountCtx -> m_pCtx = apServerCtx;
   apMountCtx -> m_ID   = *( unsigned short* )&lPkt[ 28 ];
   retVal               = 1;

  } else s_LastError = SVAL( lPkt, 11 );

 }  /* end if */

 return retVal;

}  /* end SMB_Mount */

static void SMB_UMount ( SMBMountContext* apCtx ) {

 unsigned char lPkt[ 256 ] __attribute__(   (  aligned( 4 )  )   );
 int           lPktLen = 40;
 int           lSD;

 mips_memset (  lPkt, 0, sizeof ( lPkt )  );

 lPkt[ 8 ]              = SMB_TREE_DISCONNECT;
 *( short* )&lPkt[ 28 ] = apCtx -> m_ID;
 *( short* )&lPkt[ 32 ] = apCtx -> m_pCtx -> m_UID;

 if (  _nb_send_packet ( lSD = apCtx -> m_pCtx -> m_Socket, lPkt, lPktLen )  ) _nb_read_packet ( lSD, lPkt, 256 );

 apCtx -> m_pCtx = NULL;

}  /* end SMB_UMount */

static int SMB_FindFirst ( SMBMountContext* apMountCtx, const char* apPath ) {

 unsigned char lPkt[ 1024 ] __attribute__(   (  aligned( 4 )  )   );
 int           lPathLen    = strlen ( apPath );
 int           lParamLen   = lPathLen + 13;
 int           lDataOffset = ALIGN( 72 + lParamLen );
 int           lBCC        = lDataOffset + 69;
 int           lPktLen     = lBCC        + 69;
 int           retVal      = 0;

 mips_memset (  lPkt, 0, sizeof ( lPkt )  );

 lPkt[  8 ]             = SMB_TRANSACT2;
 lPkt[ 14 ]             = 1;
 *( short* )&lPkt[ 28 ] = apMountCtx -> m_ID;
 *( short* )&lPkt[ 32 ] = apMountCtx -> m_pCtx -> m_UID;
 lPkt[ 36 ]             = 15;
 SVAL( lPkt, 37 )       = lParamLen;
 lPkt[ 41 ]             = 64;
 SVAL( lPkt, 43 )       = 768;
 SVAL( lPkt, 55 )       = lParamLen;
 lPkt[ 57 ]             = 68;
 SVAL( lPkt, 61 )       = lDataOffset - 4;
 lPkt[ 63 ]             = 1;
 lPkt[ 65 ]             = SMB_TRANSACT2_FINDFIRST;
 SVAL( lPkt, 67 )       = lBCC;
 lPkt[ 70 ]             = 'D';
 lPkt[ 71 ]             = ' ';
 *( short* )&lPkt[ 72 ] = SMB_ATTR_SYSTEM | SMB_ATTR_HIDDEN | SMB_ATTR_DIR;
 *( short* )&lPkt[ 74 ] = 1;
 *( short* )&lPkt[ 76 ] = SMB_FFF_CLOSE_IF_END | SMB_FFF_REQUIRE_RESUME_KEY;
 *( short* )&lPkt[ 78 ] = 1;
 strcpy ( &lPkt[ 84 ], apPath );

 if (  _nb_send_packet ( lParamLen = apMountCtx -> m_pCtx -> m_Socket, lPkt, lPktLen )  ) {

  lPktLen = _nb_read_packet ( lParamLen, lPkt, 1024 );

  if ( lPktLen && !lPkt[ 9 ] ) {

   SMBFindFirstParam* lpParam = ( SMBFindFirstParam* )&lPkt[ SVAL( lPkt, 45 ) + 4 ];

   if ( lpParam -> m_Count ) {

    SMBFileInfo* lpData  = ( SMBFileInfo* )&lPkt[ SVAL( lPkt, 51 ) + 8 ];

    if ( lpParam -> m_NameOffset ) apPath = (  ( char* )lpData  ) + lpParam -> m_NameOffset + 1;

    s_FindCtx.m_pCtx      = apMountCtx;
    s_FindCtx.m_DirHandle = lpParam -> m_Handle;
    s_FindCtx.m_MaskLen   = strlen ( apPath );
    strncpy (  s_FindCtx.m_Mask, apPath, sizeof ( s_FindCtx.m_Mask ) - 1  );
    s_FindCtx.m_Mask[ sizeof ( s_FindCtx.m_Mask ) - 1 ] = '\x00';

    _smb_file_info_dos2ps2 ( &s_FileInfo, lpData );

    retVal = 1;

   } else s_LastError = -ENOENT;

  } else s_LastError = SVAL( lPkt, 11 );

 }  /* end if */

 return retVal;

}  /* end SMB_FindFirst */

static int SMB_FindNext ( short afClose, io_dirent_t* apDirEnt ) {

 unsigned char lPkt[ 1024 ] __attribute__(   (  aligned( 4 )  )   );
 int           lParamLen   = s_FindCtx.m_MaskLen + 13;
 int           lDataOffset = ALIGN( 72 + lParamLen );
 int           lBCC        = lDataOffset + 69;
 int           lPktLen     = lBCC        + 69;
 int           retVal      = 0;

 mips_memset (  lPkt, 0, sizeof ( lPkt )  );

 lPkt[  8 ]             = SMB_TRANSACT2;
 lPkt[ 14 ]             = 1;
 *( short* )&lPkt[ 28 ] = s_FindCtx.m_pCtx -> m_ID;
 *( short* )&lPkt[ 32 ] = s_FindCtx.m_pCtx -> m_pCtx -> m_UID;
 lPkt[ 36 ]             = 15;
 SVAL( lPkt, 37 )       = lParamLen;
 lPkt[ 41 ]             = 64;
 SVAL( lPkt, 43 )       = 768;
 SVAL( lPkt, 55 )       = lParamLen;
 lPkt[ 57 ]             = 68;
 SVAL( lPkt, 61 )       = lDataOffset - 4;
 lPkt[ 63 ]             = 1;
 lPkt[ 65 ]             = SMB_TRANSACT2_FINDNEXT;
 lPkt[ 70 ]             = 'D';
 lPkt[ 71 ]             = ' ';
 *( short* )&lPkt[ 72 ] = s_FindCtx.m_DirHandle;
 *( short* )&lPkt[ 74 ] = 1;
 *( short* )&lPkt[ 76 ] = 1;
 *( short* )&lPkt[ 82 ] = SMB_FFF_CONTINUE_BIT | SMB_FFF_CLOSE_IF_END | SMB_FFF_REQUIRE_RESUME_KEY | afClose;
 strcpy ( &lPkt[ 84 ], s_FindCtx.m_Mask );

 if (  _nb_send_packet ( lParamLen = s_FindCtx.m_pCtx -> m_pCtx -> m_Socket, lPkt, lPktLen )  ) {

  lPktLen = _nb_read_packet ( lParamLen, lPkt, 1024 );

  if ( lPktLen && !lPkt[ 9 ] ) {

   SMBFindNextParam* lpParam = ( SMBFindNextParam* )&lPkt[ SVAL( lPkt, 45 ) + 4 ];
   SMBFileInfo*      lpData  = ( SMBFileInfo*      )&lPkt[ SVAL( lPkt, 51 ) + 8 ];

   retVal = lpParam -> m_Count;

   if ( retVal && lpParam -> m_NameOffset ) {

    unsigned char* lpMask = (  ( unsigned char* )lpData  ) + lpParam -> m_NameOffset;

    s_FindCtx.m_MaskLen = *lpMask;
    strncpy (  s_FindCtx.m_Mask, lpMask, sizeof ( s_FindCtx.m_Mask ) - 1  );

   }  /* end if */

   if ( retVal ) _smb_file_info_dos2ps2 ( apDirEnt, lpData );

  } else s_LastError = SVAL( lPkt, 11 );

 }  /* end if */

 return retVal;

}  /* end SMB_FindNext */

static int SMB_Open ( SMBMountContext* apMountCtx, const char* apFileName, short* apHandle ) {

 int            retVal   = 0;
 int            lNameLen = strlen ( apFileName );
 int            lPktLen  = lNameLen + 46;
 unsigned char  lPkt[ 512 ] __attribute__(   (  aligned( 4 )  )   );

 mips_memset (  lPkt, 0, sizeof ( lPkt )  );

 lPkt[  8 ]             = SMB_OPEN;
 *( short* )&lPkt[ 28 ] = apMountCtx -> m_ID;
 *( short* )&lPkt[ 32 ] = apMountCtx -> m_pCtx -> m_UID;
 lPkt[ 36 ]             = 2;
 SVAL( lPkt, 41 )       = lNameLen + 2;
 lPkt[ 43 ]             = 4;
 strcpy ( &lPkt[ 44 ], apFileName );

 if (  _nb_send_packet ( lNameLen = apMountCtx -> m_pCtx -> m_Socket, lPkt, lPktLen )  ) {

  lPktLen = _nb_read_packet ( lNameLen, lPkt, 512 );

  if ( lPktLen && !lPkt[ 9 ] ) {
   *apHandle = SVAL( lPkt, 37 );
   retVal    = 1;
  } else s_LastError = SVAL( lPkt, 11 );

 }  /* end if */

 return retVal;

}  /* end SMB_Open */

static void SMB_Close ( SMBFileContext* apFileCtx ) {

 unsigned char    lPkt[ 256 ] __attribute__(   (  aligned( 4 )  )   );
 SMBMountContext* lpMountCtx = apFileCtx -> m_pCtx;
 int              lPktLen    = 44;
 int              lSD;

 mips_memset (  lPkt, 0, sizeof ( lPkt )  );

 lPkt[  8 ]             = SMB_CLOSE;
 *( short* )&lPkt[ 28 ] = lpMountCtx -> m_ID;
 *( short* )&lPkt[ 32 ] = lpMountCtx -> m_pCtx -> m_UID;
 lPkt[ 36 ]             = 3;
 SVAL( lPkt, 37 )       = apFileCtx -> m_FD;

 if (  _nb_send_packet ( lSD = apFileCtx -> m_SD, lPkt, lPktLen )  ) _nb_read_packet ( lSD, lPkt, 256 );

 apFileCtx -> m_pCtx = NULL;

}  /* end SMB_Close */

static int SMB_ShareEnum ( SMBServerContext* apCtx, SMBShareInfo** appShareInfo, int* apRemOffset ) {

 int             retVal = 0;
 int             lSD;
 SMBMountContext lIPCTree;

 if (  SMB_Mount ( apCtx, &lIPCTree, "IPC$" )  ) {

  mips_memset (  s_SEnumBuff, 0, sizeof ( s_SEnumBuff )  );

  s_SEnumBuff[  8 ]             = SMB_TRANSACT;
  *( short* )&s_SEnumBuff[ 28 ] = lIPCTree.m_ID;
  *( short* )&s_SEnumBuff[ 32 ] = apCtx -> m_UID;
  s_SEnumBuff[ 36 ]             =   14;
  s_SEnumBuff[ 37 ]             =   19;
  SVAL( s_SEnumBuff, 41 )       = 1024;
  SVAL( s_SEnumBuff, 43 )       = SMB_SENUM_SIZE;
  s_SEnumBuff[ 55 ]             =   19;
  s_SEnumBuff[ 57 ]             =   76;
  s_SEnumBuff[ 61 ]             =   95;
  s_SEnumBuff[ 65 ]             =   32;
  strcpy ( &s_SEnumBuff[ 67 ], "\\PIPE\\LANMAN" );
  strcpy ( &s_SEnumBuff[ 82 ], "WrLeh"          );
  strcpy ( &s_SEnumBuff[ 88 ], "B13BWz"         );
  s_SEnumBuff[ 95 ]             = 1;
  SVAL( s_SEnumBuff, 97 )       = SMB_SENUM_SIZE;

  if (  _nb_send_packet ( lSD = apCtx -> m_Socket, s_SEnumBuff, 95 )  ) {

   lSD = _nb_read_packet ( lSD, s_SEnumBuff, 4096 );

   if ( lSD && !s_SEnumBuff[ 9 ] ) {

    SMBNetShareEnumParam* lpParam = ( SMBNetShareEnumParam* )&s_SEnumBuff[ SVAL( s_SEnumBuff, 45 ) + 4 ];

    if (  !( lSD = lpParam -> m_ErrorCode )  ) {

     *apRemOffset  = lpParam -> m_Converter;
     *appShareInfo = ( SMBShareInfo* )&s_SEnumBuff[ SVAL( s_SEnumBuff, 51 ) + 4 ];

     retVal = lpParam -> m_EntryCount;

    } else s_LastError = lSD;

   } else s_LastError = SVAL( s_SEnumBuff, 11 );

  } else s_LastError = SMB_ERROR_COMM;

  SMB_UMount ( &lIPCTree );

 }  /* end if */

 return retVal;

}  /* end SMB_ShareEnum */

static int SMB_Echo ( SMBServerContext* apCtx ) {

 int           retVal = 0;
 int           lSD;
 unsigned char lPkt[ 64 ] __attribute__(   ( aligned( 4 )  )   );

 mips_memset (  lPkt, 0, sizeof ( lPkt )  );

 lPkt[  8 ]             = SMB_ECHO;
 *( short* )&lPkt[ 32 ] = apCtx -> m_UID;
 lPkt[ 36 ]             = 1;
 SVAL( lPkt, 37 )       = 1;
 SVAL( lPkt, 39 )       = 1;
 lPkt[ 41 ]             = 'S';

 if (  _nb_send_packet ( lSD = apCtx -> m_Socket, lPkt, 42 )  ) {

  lPkt[ 41 ] = '\x00';
  lSD        = _nb_read_packet ( lSD, lPkt, 64 );

  if ( lSD && !lPkt[ 9 ] && lPkt[ 41 ] =='S' ) retVal = 1;

 }  /* end if */

 return retVal;

}  /* end SMB_Echo */

static int _DrvNOP ( void ) {
 return -ENOTSUP;
}  /* end _DrvNOP */

static int _DrvInit ( iop_io_device_t* apDev ) {

 return 0;

}  /* end _DrvInit */

static int _DrvDeInit ( iop_io_device_t* apDev ) {

 int i;

 for ( i = 0; i < SMB_MAX_SERVERS; ++i )

  if ( s_ServerCtx[ i ].m_Socket >= 0 ) lwip_close ( s_ServerCtx[ i ].m_Socket );

 return 0;

}  /* end _DrvDeInit */

static int _DrvDOpen ( iop_io_file_t* apFile, const char* apName ) {

 int retVal;
 int lUnit = apFile -> unit;

 if ( !lUnit && !apName[ 0 ] ) {

  if ( s_Flags & FLAG_DOPEN ) return -EACCES;

  s_Flags |= FLAG_DOPEN;
  apFile -> privdata = ( void* )( retVal = FLAG_DOPEN );

 } else if (  !( s_Flags & FLAG_DOPEN_1 )  ) {

  --lUnit;

  if ( lUnit >= 0 && lUnit < SMB_MAX_SERVERS * SMB_MAX_MOUNT && s_MountCtx[ lUnit ] -> m_pCtx &&
       s_MountCtx[ lUnit ] -> m_pCtx -> m_Socket >= 0
  ) {

   int lLen;

   strcpy ( s_SEnumBuff, apName );

   lLen = _smb_bs_subst ( s_SEnumBuff );

   if ( s_SEnumBuff[ lLen - 1 ] != '\\' ) s_SEnumBuff[ lLen++ ] = '\\';

   s_SEnumBuff[ lLen++ ] = '*';
   s_SEnumBuff[ lLen   ] = '\x00';

   if (  SMB_FindFirst ( s_MountCtx[ lUnit ], s_SEnumBuff )   ) {
    s_Flags           |= FLAG_DOPEN_1;
    apFile -> privdata = ( void* )( retVal = FLAG_DOPEN_1 );
   } else retVal = -ENMFILE;

  } else retVal = -EBADF;

 } else retVal = -EACCES;

 return retVal;

}  /* end _DrvDOpen */

static int _DrvDClose ( iop_io_file_t* apFile ) {

 int retVal;

 if (  apFile -> privdata == ( void* )FLAG_DOPEN  ) {

  s_Flags &= ~FLAG_DOPEN;
  retVal   = 0;

 } else if ( apFile -> privdata == ( void* )FLAG_DOPEN_1 ) {

  if ( s_FindCtx.m_pCtx ) {
   SMB_FindNext ( 1, &s_FileInfo );
   s_FindCtx.m_pCtx = NULL;
  }  /* end if */

  s_Flags &= ~FLAG_DOPEN_1;
  retVal   = 0;

 } else retVal = -ENOSYS;

 return retVal;
 
}  /* end _DrvDClose */

static int _DrvDRead ( iop_io_file_t* apFile, void* apParam ) {

 int retVal;

 if ( s_Flags & FLAG_DOPEN_1 ) {

  if ( s_FindCtx.m_pCtx ) {

   io_dirent_t* lpDirEnt = ( io_dirent_t* )apParam;

   *lpDirEnt = s_FileInfo;

   if (  !SMB_FindNext ( 0, &s_FileInfo )  ) s_FindCtx.m_pCtx = NULL;

   retVal = 1;

  } else retVal = -ENMFILE;

 } else retVal = -EBADR;

 return retVal;

}  /* end _DrvDRead */

static int _DrvOpen ( iop_io_file_t* apFile, const char* apName, int aMode, ... ) {

 int retVal, lUnit = apFile -> unit - 1;

 if ( aMode != O_RDONLY ) return -EACCES;

 if ( lUnit >= 0 && lUnit < SMB_MAX_SERVERS * SMB_MAX_MOUNT && s_MountCtx[ lUnit ] -> m_pCtx &&
      s_MountCtx[ lUnit ] -> m_pCtx -> m_Socket >= 0
 ) {

  int              i;
  SMBMountContext* lpMountCtx = s_MountCtx[ lUnit ];

  for ( i = 0; i < SMB_MAX_FILES; ++i )
   if ( !lpMountCtx -> m_FileCtx[ i ].m_pCtx ) break;

  if ( i < SMB_MAX_FILES ) {

   SMBFileContext* lpFileCtx = &lpMountCtx -> m_FileCtx[ i ];

   _smb_bs_subst (  ( char* )apName  );

   if (  SMB_Open ( lpMountCtx, apName, &lpFileCtx -> m_FD )  ) {

    unsigned char  lPkt[ 256 ] __attribute__(   (  aligned( 4 )  )   );
    int            lSD, lPktLen = 46;
    unsigned char* lpPkt        = lpFileCtx -> m_ReadPkt;

    lpFileCtx -> m_pCtx = lpMountCtx;
    lpFileCtx -> m_SD   = lpMountCtx -> m_pCtx -> m_Socket;
    lpFileCtx -> m_Pos  = 0;

    mips_memset (  lpPkt, 0, sizeof ( lpFileCtx -> m_ReadPkt )  );

    *( short* )&lpPkt[ 28 ] = lpMountCtx -> m_ID;
    *( short* )&lpPkt[ 32 ] = lpMountCtx -> m_pCtx -> m_UID;
    SVAL( lpPkt, 37 )       = lpFileCtx -> m_FD;

    if ( lpMountCtx -> m_pCtx -> m_RawSupp ) {

     lpPkt[  8 ] = SMB_READ_RAW;
     lpPkt[ 36 ] = 8;

    } else {

     lpPkt[  8 ] = SMB_READ;
     lpPkt[ 36 ] = 5;

    }  /* end else */

    mips_memset (  lPkt, 0, sizeof ( lPkt )  );

    lPkt[  8 ]             = SMB_LSEEK;
    *( short* )&lPkt[ 28 ] = lpMountCtx -> m_ID;
    *( short* )&lPkt[ 32 ] = lpMountCtx -> m_pCtx -> m_UID;
    lPkt[ 36 ]             = 4;
    SVAL( lPkt, 37 )       = lpFileCtx -> m_FD;
    SVAL( lPkt, 39 )       = 2;

    if (  _nb_send_packet ( lSD = lpFileCtx -> m_SD, lPkt, lPktLen )  ) {

     lPktLen             = _nb_read_packet ( lSD, lPkt, 256 );
     lpFileCtx -> m_Size = ( lPktLen && !lPkt[ 9 ] ) ? IVAL( lPkt, 37 ) : 0;

    }  /* end if */

    apFile -> privdata = lpFileCtx;
    retVal             = 0;

   } else retVal = -EIO;

  } else retVal = -ENFILE;

 } else retVal = -EBADF;

 return retVal;

}  /* end _DrvOpen */

static int _DrvClose ( iop_io_file_t* apFile ) {

 if ( apFile -> privdata ) SMB_Close (  ( SMBFileContext* )apFile -> privdata  );

 return 0;

}  /* end _DrvClose */

static int _DrvRead ( iop_io_file_t* apFile, void* apBuffer, int anBytes ) {

 SMBFileContext* lpFileCtx = ( SMBFileContext* )apFile -> privdata;

 return lpFileCtx -> Read ( lpFileCtx, apBuffer, anBytes );

}  /* end _DrvRead */

static int _DrvLSeek ( iop_io_file_t* apFile, unsigned long aPos, int aDisp ) {

 SMBFileContext* lpFileCtx = ( SMBFileContext* )apFile -> privdata;
 unsigned int    lSize     = lpFileCtx -> m_Size;

 if ( aDisp == 1 )
  aPos += lpFileCtx -> m_Pos;
 else if ( aDisp == 2 ) aPos = lSize;

 if ( aPos > lSize ) aPos = lSize;

 lpFileCtx -> m_Pos = aPos;

 return aPos;

}  /* end _DrvLSeek */

static void LoginThread ( void* apParam ) {

 struct sockaddr_in lAddr;
 int                lSD;
 SMBServerContext*  lpSrvCtx  = ( SMBServerContext* )apParam;
 int                lErrCode  = 0;
 int                lnCommErr = 0;
 int                lLoginSem = lpSrvCtx -> m_LoginSema;

 lAddr.sin_family      = AF_INET;
 lAddr.sin_addr.s_addr = inet_addr ( lpSrvCtx -> m_LoginInfo.m_ServerIP );
 lAddr.sin_port        = htons ( 139 );
retry:
 lSD = lwip_socket ( PF_INET, SOCK_STREAM, IPPROTO_TCP );

 if ( lSD >= 0 ) while ( 1 ) {

  if (  !lwip_connect (  lSD, ( struct sockaddr* )&lAddr, sizeof ( struct sockaddr_in )  )  ) {

   unsigned char lBuff[ 72 ] __attribute__(   (  aligned( 4 )  )   );

   mips_memset (  lBuff, 0, sizeof ( lBuff )  );

   lBuff[  0 ] = NB_SESSION_REQ;
   NB_SET_PKT_LEN( lBuff, 68 );
   lBuff[  4 ] = ' ';
   _nb_name ( lpSrvCtx -> m_LoginInfo.m_ServerName, &lBuff[  5 ] );
   lBuff[ 38 ] = ' ';
   _nb_name ( lpSrvCtx -> m_LoginInfo.m_ClientName, &lBuff[ 39 ] );

   if (  lwip_send ( lSD, lBuff, 72, 0 ) == 72  ) {

    if (  lwip_recv (  lSD, lBuff, sizeof ( lBuff ), 0  ) > 0  ) {

     if ( lBuff[ 0 ] == NB_SESSION_RESP_OK ) {

      lpSrvCtx -> m_Socket = lSD;

      if (  !SMB_Negotiate  ( lpSrvCtx )  ) {
       lErrCode             = SMB_ERROR_NEGOTIATE;
       lpSrvCtx -> m_Socket = 0x80000000;
      } else if (  !SMB_Login ( lpSrvCtx )  ) {
       lErrCode             = SMB_ERROR_LOGIN;
       lpSrvCtx -> m_Socket = 0x80000000;
      }  /* end if */

     } else if ( ++lnCommErr == 5 || lLoginSem >= 0 ) lErrCode = SMB_ERROR_COMM;

    } else {
again:
     lwip_close ( lSD );

     if ( lLoginSem < 0 && !lErrCode ) {
      DelayThread ( 5000000 );
      goto retry;
     } else break;

    }  /* end else */

   } else if ( ++lnCommErr == 5 || lLoginSem >= 0 ) lErrCode = SMB_ERROR_COMM;

   if ( lpSrvCtx -> m_Socket >= 0 ) break;

   goto again;

  } else if ( lLoginSem >= 0 ) goto again;

  DelayThread ( 5000000 );

 }  /* end while */

 if ( lLoginSem >= 0 )
  SignalSema ( lLoginSem );
 else {

  int lCmdData[ 7 ] __attribute__(   (  aligned( 64 )  )   );
  int lIndex = lpSrvCtx - &s_ServerCtx[ 0 ];

  lCmdData[ 3 ] = 0;
  lCmdData[ 4 ] = lpSrvCtx -> m_Socket >= 0 ? lIndex : 0x80000000;
  lCmdData[ 5 ] = lErrCode;
  lCmdData[ 6 ] = s_LastError;

  lSD = sceSifSendCmd ( 18, lCmdData, 28, NULL, NULL, 0 );

  s_Flags &= ~( FLAG_LOGIN_0_PROGRESS << lIndex );
  s_Flags |= FLAG_LOGIN_0 << lIndex;

  while (  sceSifDmaStat ( lSD ) >= 0  ) DelayThread ( 100 );

 }  /* end if */

 ExitDeleteThread ();

}  /* end LoginThread */

static int _DrvIOCtl ( iop_io_file_t* apFile, unsigned long aCmd, void* apParam ) {

 int retVal = -EBADRQC;

 if (  apFile -> privdata != ( void* )FLAG_DOPEN  ) return -EBADF;

 switch ( aCmd ) {

  case SMB_IOCTL_LOGIN: {

   int               lSrvIdx = 0;
   SMBLoginInfo*     lpInfo  = ( SMBLoginInfo* )apParam;
   SMBServerContext* lpSrvCtx;
   iop_thread_t      lThread;

   if ( s_Flags & FLAG_LOGIN_0_PROGRESS ||
        s_Flags & FLAG_LOGIN_0
   ) {

    if (  !( s_Flags & FLAG_LOGIN_1_PROGRESS ||
             s_Flags & FLAG_LOGIN_1
           )
    ) lSrvIdx = 1;
    else return -EMLINK;

   }  /* end if */

   lpSrvCtx = &s_ServerCtx[ lSrvIdx ];
   lpSrvCtx -> m_LoginInfo = *lpInfo;
   lpSrvCtx -> m_Socket    = 0x80000000;

   if ( !lpInfo -> m_fAsync ) {

    iop_sema_t lSema;
    lSema.attr    = 0;
    lSema.initial = 0;
    lpSrvCtx -> m_LoginSema = CreateSema ( &lSema );

   } else {

    lpSrvCtx -> m_LoginSema = 0x80000000;
    s_Flags |= FLAG_LOGIN_0_PROGRESS << lSrvIdx;

   }  /* end else */

   lThread.attr      = TH_C;
   lThread.thread    = LoginThread;
   lThread.stacksize = 3072;
   lThread.priority  = 64;
   StartThread (  CreateThread ( &lThread ), lpSrvCtx  );

   if ( !lpInfo -> m_fAsync ) {

    WaitSema ( lpSrvCtx -> m_LoginSema );
    DeleteSema ( lpSrvCtx -> m_LoginSema );

    if (   (  retVal = lpSrvCtx -> m_Socket >= 0 ? lSrvIdx : -ENOTCONN   ) >= 0  ) s_Flags |= FLAG_LOGIN_0 << lSrvIdx;

   } else retVal = -EINPROGRESS;

  } break;

  case SMB_IOCTL_LOGOUT: {

   int lSrvIdx = *( int* )apParam;

   if (   lSrvIdx >= 0 && lSrvIdx < 2 && (  s_Flags & ( FLAG_LOGIN_0 << lSrvIdx )  ) &&
          s_ServerCtx[ lSrvIdx ].m_Socket >= 0
   ) {

    int               i, j;
    SMBServerContext* lpSrvCtx = &s_ServerCtx[ lSrvIdx ];

    for ( i = 0; i < SMB_MAX_MOUNT; ++i ) {

     SMBMountContext* lpMountCtx = &lpSrvCtx -> m_MountCtx[ i ];

     if ( lpMountCtx -> m_pCtx ) {

      for ( j = 0; j < SMB_MAX_FILES; ++j ) {

       SMBFileContext* lpFileCtx = &lpMountCtx -> m_FileCtx[ i ];

       if ( lpFileCtx -> m_pCtx ) SMB_Close ( lpFileCtx );

      }  /* end for */

      for ( j = 0; j < SMB_MAX_SERVERS * SMB_MAX_MOUNT; ++j )
       if ( s_MountCtx[ j ] == lpMountCtx ) {
        s_MountCtx[ j ] = NULL;
        break;
       }  /* end if */

      SMB_UMount ( lpMountCtx );

     }  /* end if */

    }  /* end for */

    s_Flags &= ~( FLAG_LOGIN_0 << lSrvIdx );
    lwip_close ( lpSrvCtx -> m_Socket );
    lpSrvCtx -> m_Socket = 0x80000000;
    retVal               = 0;

   } else retVal = -ENOTCONN;

  } break;

  case SMB_IOCTL_MOUNT: {

   SMBMountInfo* lpInfo = ( SMBMountInfo* )apParam;
   int           lUnit  = lpInfo -> m_Unit;

   if (   lUnit >= 0 && lUnit < 2 && (  s_Flags & ( FLAG_LOGIN_0 << lUnit )  ) && s_ServerCtx[ lUnit ].m_Socket >= 0   ) {

    int i;

    for ( i = 0; i < SMB_MAX_SERVERS * SMB_MAX_MOUNT; ++i )
     if ( !s_MountCtx[ i ] ) break;

    if ( i < SMB_MAX_SERVERS * SMB_MAX_MOUNT ) {

     int               j;
     SMBServerContext* lpSrvCtx = &s_ServerCtx[ lUnit ];

     for ( j = 0; j < SMB_MAX_MOUNT; ++j )
      if ( !lpSrvCtx -> m_MountCtx[ j ].m_pCtx ) break;

     if ( j < SMB_MAX_MOUNT ) {

      SMBMountContext* lpMountCtx = &lpSrvCtx -> m_MountCtx[ j ];

      if (  SMB_Mount ( lpSrvCtx, lpMountCtx, lpInfo -> m_Path )  ) {

       s_MountCtx[ i ] = lpMountCtx;
       retVal          = i + 1;

      } else retVal = -ENOSHARE;

     } else retVal = -ENFILE;

    } else retVal = -ENFILE;

   } else retVal = -ENOTCONN;

  } break;

  case SMB_IOCTL_UMOUNT: {

   int lMountUnit = *( int* )apParam - 1;

   if ( lMountUnit >= 0 && lMountUnit < SMB_MAX_SERVERS * SMB_MAX_MOUNT ) {

    SMBMountContext* lpMountCtx = s_MountCtx[ lMountUnit ];

    if ( lpMountCtx -> m_pCtx ) {

     s_MountCtx[ lMountUnit ] = NULL;
     retVal                   = 0;

     SMB_UMount ( lpMountCtx );

    } else retVal = -ENODEV; 

   } else retVal = -EBADF;

  } break;

  case SMB_IOCTL_SENUM: {

   int lSrvIdx = *( int* )apParam;

   if (   lSrvIdx >= 0 && lSrvIdx < 2 && (  s_Flags & ( FLAG_LOGIN_0 << lSrvIdx )  ) &&
          s_ServerCtx[ lSrvIdx ].m_Socket >= 0
   ) {

    SMBShareInfo* lpInfo;
    int           lRemOffset;
    int           lCount = SMB_ShareEnum ( &s_ServerCtx[ lSrvIdx ], &lpInfo, &lRemOffset );

    if ( lCount > 0 ) {

     int              i, lDest = (  ( int* )apParam  )[ 1 ];
     SifDmaTransfer_t lDMA;

     lDMA.src  = lpInfo;
     lDMA.dest = ( void* )lDest;
     lDMA.size = SMB_SENUM_SIZE;
     lDMA.attr = 0;

     for ( i = 0; i < lCount; ++i )
      lpInfo[ i ].m_pRemark = ( unsigned char* )(
       lDest + ( unsigned int )lpInfo[ i ].m_pRemark - lRemOffset
      );

     CpuSuspendIntr ( &lDest );
      i = sceSifSetDma ( &lDMA, 1 );
     CpuResumeIntr ( lDest );

     while (  sceSifDmaStat ( i ) >= 0  ) DelayThread ( 100 );
 
     retVal = lCount;

    } else retVal = -s_LastError;

   } else retVal = -ENOTCONN;

  } break;

  case SMB_IOCTL_ECHO: {

   int lSrvIdx = *( int* )apParam;

   retVal = (   lSrvIdx >= 0 && lSrvIdx < 2 && (  s_Flags & ( FLAG_LOGIN_0 << lSrvIdx )  ) &&
            s_ServerCtx[ lSrvIdx ].m_Socket >= 0 && SMB_Echo ( &s_ServerCtx[ lSrvIdx ] )   ) ? 0 : -ENOTCONN;

  } break;

 }  /* end switch */

 return retVal;

}  /* end _DrvIOCtl */

int _start ( int argc, char** argv ) {

 static iop_io_device_t s_lSMBDriver = {
  "smb", IOP_DT_FS, 1, "SMB driver", &s_Driver
 };

 io_AddDrv ( &s_lSMBDriver );

 return MODULE_RESIDENT_END;

}  /* end _start */
