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

#define       atoi(x) strtol(x, NULL, 10)


#define TRUE	1
#define FALSE	0
#define MODNAME "ten_ftp_fs"
#define FS_REPNAME "ftp"

extern int ttyMount(void);



typedef struct{
	char FtpUser[256];
	char FtpPass[256];
	int bAuthed;
	int bBusy;
	int bData;
}tenFtpState;

extern tenFtpState	lFtpState[8];

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

int ten_strlen( char *s );
char *ten_strchr( char *s, int c);
int ten_strcmp( const char *c1, const char *c2 );

int TenTime();


#define DEBUG_LEVEL 2
// 1 = base info
// 2 = print every send&recv
