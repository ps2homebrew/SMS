#include <tamtypes.h>
#include <ps2lib_err.h>
#include <sifrpc.h>
#include <io_common.h>

#include "types.h"
#include "stdio.h"
#include "sysclib.h"
#include "thbase.h"
#include "intrman.h"
#include "sysmem.h"
#include "sifman.h"
#include "sifcmd.h"

#include "iomanX.h"
#include "ioman_mod.h"

#include "ps2ip.h"




#define malloc(size) \
(AllocSysMemory(0,size,NULL))

#define free(ptr) \
(FreeSysMemory(ptr))


#define       atoi(x) strtol(x, NULL, 10)


#define TRUE	1
#define FALSE	0
#define MODNAME "ten_ftp_fs"
#define FS_REPNAME "ftp"

extern int ttyMount(void);


#define TEN_MAX_CONNS 32


typedef struct{
	char FtpUser[256];
	char FtpPass[256];
	int bAuthed;
	int bBusy;
	int bData;
	char *DirReadCacheData;
}tenFtpState;

extern tenFtpState	lFtpState[TEN_MAX_CONNS];

int GetFtpConn( char *p1, char *p2, char *p3 );
int FtpClose( int serverId );
int FtpEmpty( int serverId );

int SendDataToFtp( int serverId, char *data, int siz );
int GetDataFromFtp( int serverId, char *buf, int buf_siz );
int IsDataFromFtpAvail( int serverId );
int FtpCommandRet( int serverId, char *cmd, char *param, char *wantReturnBuf, int WantReturnSize );
int FtpCommand( int serverId, char *cmd, char *param );


void CheckTimeout();


int FtpDataConn( int serverId );
int FtpDataClose( int serverId );
int SendDataToFtpDataConn( int serverId, char *data, int siz );
int GetDataFromFtpDataConn( int serverId, char *buf, int buf_siz );
int IsDataFromFtpDataConnAvail( int serverId );
int IsDataSendToFtpDataConnAvail( int serverId );

int ten_strlen( char *s );
char *ten_strchr( char *s, int c);
int ten_strcmp( const char *c1, const char *c2 );

int TenTime();


#define DEBUG_LEVEL 0
// 1 = base info
// 2 = print every send&recv











//file layer:

//opens connection
int TenFtp_Init( char *ip_port, char *user, char *pass ); 

//open file / directory listing
// bForceDirectory >0  fetch directory
// bForceDirectory <0  fetch file
// bForceDirectory ==0  parse path and decide based on trailing '/' or not
// bWrite = file mode: if >0 use STOR instead of RETR
// JumpTo = jumps to specified file position if JumpTo > 0
int TenFtp_Open( char *name, int bForceDirectory, int bWrite, int JumpTo );

//fetch file size
int TenFtp_Size( char *name );

//check if data conn is availible
int TenFtp_CheckDataConn( int handle );

//close file & data connection
// handle = handle returned by TenFtp_Open
int TenFtp_Close( int handle );

//create or remove dir
// iname = full path
// bRem == 1 : remove 
//    else create
int TenFtp_MkRmDir( char *iname, int bRem );


void SplitPath( char **PathBuf, char **FileBuf, char *name );
int RemAndConnectAddress( char **oname, char *name );

