#include "tenGlob.h"

extern int LastFtp;


int TenFtp_Init( char *p1, char *p2, char *p3 )
{
#if DEBUG_LEVEL>=1
	printf( "TenFtp_Init: %s %s %s\n", p1, p2, p3 );
#endif
	return (LastFtp=GetFtpConn( p1, p2, p3 ));
}

int RemAndConnectAddress( char **oname, char *name )
{
#if DEBUG_LEVEL>=1
	printf( "RemAndConnectAddress %s \n", name );
#endif

	int sConn = LastFtp;

	if( ten_strchr( name, '@' ) && name[0]=='/' && name[1]=='/' )
	{//its a addr/user/pw form
		char *c1,*c2,*c3;
		while( name[0]=='/' )
			name++;

		c1=ten_strchr( name, ':' );
		c1[0]=0;
		c1++;

		c2=ten_strchr( c1, '@' );
		c2[0]=0;
		c2++;

		c3=ten_strchr( c2, '/' );
		c3[0]=0;
		c3++;

		sConn = TenFtp_Init( c2/*address*/, name/*user*/, c1/*pass*/ );

		name = c3;
	}

#if DEBUG_LEVEL>=1
	printf( "RemAndConnectAddress2 %s %d \n", name, sConn );
#endif

	*oname=name;

	return sConn;
}

void SplitPath( char **PathBuf, char **FileBuf, char *name )
{
	while( name[0]=='/' )
		name++;

	*PathBuf=name;
	*FileBuf=strrchr( name, '/' );

	if( *FileBuf )
	{
		(*FileBuf)[0]=0;
		(*FileBuf)++;
	}
	else
	{//no path
		(*FileBuf)=(*PathBuf);
		(*PathBuf)="";
	}
}

int TenFtp_Open( char *iname, int bForceDirectory, int bWrite, int JumpTo )
{
	char *PathBuf;
	char *FileBuf;
	char tmp[64];
	int ret;
	int bDir;
	char *name;
	
	int sConn = LastFtp;

	sConn = RemAndConnectAddress( &name, iname );

	if( sConn < 0 )
		return -1;

	if( lFtpState[sConn].bBusy )
		return -1;

	lFtpState[sConn].bBusy=1;

	SplitPath( &PathBuf, &FileBuf, name );

#if DEBUG_LEVEL>=1
	printf( "PathBuf FileBuf %s %s ", PathBuf, FileBuf );
#endif

	//change dir:
	ret = FtpCommand( sConn, "CWD", "/" );
	if( ret != 250 )
		return -2;

	if( PathBuf[0] )
	{
		ret = FtpCommand( sConn, "CWD", PathBuf );
		if( ret != 250 )
			return -3;
	}

	bDir=1;
	if( strlen(FileBuf) )
		bDir=0;
	if( bForceDirectory>0 )
		bDir=1;
	if( bForceDirectory<0 )
		bDir=0;

	if( !bDir )
	{//we want a file
		ret = FtpCommand( sConn, "TYPE", "I" );
		if( ret != 200 )
			return -4;
	}
	else
	{//we want a listing
		ret = FtpCommand( sConn, "TYPE", "A" );
		if( ret != 200 )
			return -5;
	}

	FtpDataConn( sConn );

	if( !bDir )
	{//we want a file

		if( JumpTo )
		{//read
			sprintf( tmp, "%d", JumpTo );
			ret = FtpCommand( sConn, "REST", tmp );
			if( ret != 350 )
				return -4;
		}

		if( !bWrite )
		{//read
			ret = FtpCommand( sConn, "RETR", FileBuf );
			if( ret != 150 )
				return -4;
		}
		else
		{//write
			ret = FtpCommand( sConn, "STOR", FileBuf );
			if( ret != 150 )
				return -4;
		}
	}
	else
	{//we want a listing
		ret = FtpCommand( sConn, "LIST", "-al" );
		if( ret != 150 )
			return -5;
	}	
	return sConn;
}


int TenFtp_Close( int sConn )
{
	if( sConn < 0 )
		return -1;

	if( ! lFtpState[sConn].bBusy )
		return -2;

	FtpEmpty( sConn );

	lFtpState[sConn].bBusy=0;

	if( lFtpState[sConn].DirReadCacheData )
		free( lFtpState[sConn].DirReadCacheData );
	lFtpState[sConn].DirReadCacheData=0;

	if( ! lFtpState[sConn].bData )
		return -3;

	FtpDataClose( sConn );

	FtpEmpty( sConn );

	return 1;
}


int TenFtp_CheckDataConn( int sConn )
{
	if( sConn < 0 )
	{
		printf( "sConn < 0\n" );
		return -1;
	}

//	printf( "lFtpState[sConn].bBusy...\n" );
	if( ! lFtpState[sConn].bBusy )
	{
		printf( "! lFtpState[sConn].bBusy\n" );
		return -2;
	}

//	printf( "lFtpState[sConn].bData...\n" );
	if( ! lFtpState[sConn].bData )
	{
		printf( "! lFtpState[sConn].bData\n" );
		return -3;
	}
	return 1;
}


int TenFtp_MkRmDir( char *iname, int bRem )
{
	int ret, sConn;

	char *PathBuf;
	char *FileBuf;
	char *name;

	sConn = RemAndConnectAddress( &name, iname );

	if( sConn < 0 )
		return -1;

#if DEBUG_LEVEL>=1
	printf( "tenftp: mkdir %s\n",  name );
#endif

	while( (name[strlen(name)-1 ]=='/') && (name[0]!=0) )
		name[strlen(name)-1 ]=0;

	SplitPath( &PathBuf, &FileBuf, name );

#if DEBUG_LEVEL>=1
	printf( "PathBuf FileBuf %s %s ", PathBuf, FileBuf );
#endif

	ret = FtpCommand( sConn, "CWD", "/" );
	if( ret != 250 )
		return -1;

	if( PathBuf[0] )
	{
		ret = FtpCommand( sConn, "CWD", PathBuf );
		if( ret != 250 )
			return -2;
	}

	if( !bRem )
	{
		ret = FtpCommand( sConn, "MKD", FileBuf );
		if( ret != 257 )
			return -3;
	}
	else
	{
		ret = FtpCommand( sConn, "RMD", FileBuf );
		if( ret != 250 )
			return -3;
	}

	return 1;
}
