#include "tenGlob.h"

int ten_strlen( char *s )
{
	int i=0;
	while( s[i]!=0 )
		i++;
	return i;
}

char *ten_strchr(char *s, int c)
{
	int i;
	int l;
	l=strlen( s );
	for( i=0;i<l;i++ )
	{
		if( s[i]==c )
			return s+i;
	}
	return 0;
}

int ten_strcmp( const char *c1, const char *c2 )
{
	int i;
	int l;

	if( strlen(c1)>strlen(c2) )
		return -1;
	if( strlen(c1)<strlen(c2) )
		return 1;

	l=strlen( c1 );
	for( i=0;i<l;i++ )
	{
		if( c1[i]>c2[i] )
			return -1;
		if( c1[i]<c2[i] )
			return 1;
	}
	return 0;
}


int WaitAnswer( int sid, char *name, char *buf, int bsiz )
{
	char tmp[256];
	int ret, wait;

#if DEBUG_LEVEL>=2
	printf( "wait for \"%s\" answer\n", name );
#endif

	ret = GetDataFromFtp( sid, buf, bsiz );
	if( ret < 0 )
		return -1;
	
	//wait for more

	wait=10;
	while( 1 )
	{
		if( IsDataFromFtpAvail( sid ) )
		{
			ret = GetDataFromFtp( sid, tmp, 250 );
			if( ret < 0 )
				return -1;
		}
		else
		{
			if( --wait > 0 )
			{
				DelayThread( 100 );
			}
			else
				break;
		}
	}
#if DEBUG_LEVEL>=2
	printf( "done\n" );
#endif

	return atoi( buf );
}

int FtpCommand( int serverId, char *cmd, char *param )
{
#if DEBUG_LEVEL>=2
	printf( "FtpCommand: %s %s\n", cmd, param );
#endif
	return FtpCommandRet( serverId, cmd, param, 0, 0 );
}

int VerifyConnected( int serverId );
extern int nFtpConns;

int FtpCommandRet( int sid, char *cmd, char *param, char *wantReturnBuf, int WantReturnSize )
{
	char tmp[512];
	int ret;

	if( sid<0 || sid>nFtpConns )
		return -1;

	if( !VerifyConnected( sid ) )
		return -1;

	if( lFtpState[sid].bAuthed==0 )
	{
#if DEBUG_LEVEL>=1
		printf( "bAuthed==0\n" );
#endif

		WaitAnswer( sid, "Server Welcome", tmp, 510 );

	//user
		sprintf( tmp, "USER %s\r\n", lFtpState[sid].FtpUser );
		ret = SendDataToFtp( sid, tmp, strlen(tmp)+1 );
		if( ret < 0 )
			return -2;

		ret = WaitAnswer( sid, "USER ***", tmp, 510 );
		if( ret < 0 )
			return -3;

		if( ret == 331 )
		{
		//pass needed
			sprintf( tmp, "PASS %s\r\n", lFtpState[sid].FtpPass );
			ret = SendDataToFtp( sid, tmp, strlen(tmp)+1 );
			if( ret < 0 )
				return -2;

			ret = WaitAnswer( sid, "PASS ***", tmp, 510 );
			if( ret < 0 )
				return -3;

			if( ret != 230 )
			{
				printf( "auth failed\n" );
				return -4;
			}
		}
		else if( ret != 230 )
		{
			printf( "login failed\n" );
			return -5;
		}

		lFtpState[sid].bAuthed=1;
	}

	sprintf( tmp, "%s %s\r\n", cmd, param );
	ret = SendDataToFtp( sid, tmp, strlen(tmp)+1 );
	if( ret < 0 )
		return -6;

#if DEBUG_LEVEL>=2
	printf( "cmd: \"%s\"\n", tmp );
#endif
	if( wantReturnBuf )
		ret = WaitAnswer( sid, "cmd", wantReturnBuf, WantReturnSize );
	else
		ret = WaitAnswer( sid, "cmd", tmp, 510 );
	if( ret < 0 )
		return -7;

	return ret;
}
