#include "tenGlob.h"
#include <stdio.h>

typedef struct{
	int FtpIp[5]; //4 ip nums + port
	int CommandConnection;
	int DataConnection;
	int lastRecv;
}tenFtpInfo;

tenFtpState	lFtpState[TEN_MAX_CONNS];
tenFtpInfo	lFtpConns[TEN_MAX_CONNS];
int			nFtpConns=0;



int GetFtpConn( char *p1, char *p2, char *p3 )
{
	int FtpIp[5]; //4 ip nums + port
	char FtpUser[256];
	char FtpPass[256];
	int i;
	char *c;

	FtpIp[4]=21;

	c=ten_strchr( p1, '.' );
	if( !c )
		return -1;
	c[0]=0;
	FtpIp[0]=atoi( p1 );
	p1=c+1;
#if DEBUG_LEVEL>=1
	printf( "found ip1\n" );
#endif

	c=ten_strchr( p1, '.' );
	if( !c )
		return -2;
	c[0]=0;
	FtpIp[1]=atoi( p1 );
	p1=c+1;
#if DEBUG_LEVEL>=1
	printf( "found ip2\n" );
#endif

	c=ten_strchr( p1, '.' );
	if( !c )
		return -3;
	c[0]=0;
	FtpIp[2]=atoi( p1 );
	p1=c+1;
#if DEBUG_LEVEL>=1
	printf( "found ip3\n" );
#endif

	c=ten_strchr( p1, ':' );
	if( !c )
	{
		c=p1+strlen(p1);
	}
	c[0]=0;
	FtpIp[3]=atoi( p1 );
	p1=c+1;
#if DEBUG_LEVEL>=1
	printf( "found ip4\n" );
#endif

	if( strlen(p1) )
	{//there is a port
#if DEBUG_LEVEL>=1
		printf( "found ip5: %s\n", p1 );
#endif
		FtpIp[4]=atoi( p1 );
	}

	strcpy( FtpUser, p2 ); 
	strcpy( FtpPass, p3 );

	for( i=0;i<nFtpConns;i++ )
	{
		if( lFtpConns[i].FtpIp[0] != FtpIp[0] )
			continue;
		if( lFtpConns[i].FtpIp[1] != FtpIp[1] )
			continue;
		if( lFtpConns[i].FtpIp[2] != FtpIp[2] )
			continue;
		if( lFtpConns[i].FtpIp[3] != FtpIp[3] )
			continue;
		if( lFtpConns[i].FtpIp[4] != FtpIp[4] )
			continue;
		if( ten_strcmp( lFtpState[i].FtpUser, FtpUser ) )
			continue;
		if( ten_strcmp( lFtpState[i].FtpPass, FtpPass ) )
			continue;

		//we dont want a busy one
		if( lFtpState[i].bBusy )
		{
#if DEBUG_LEVEL>=1
			printf( "cache %d busy\n",i );
#endif
			continue;
		}
		if( lFtpState[i].bData )
		{
#if DEBUG_LEVEL>=1
			printf( "cache %d data\n",i );
#endif
			continue;
		}

		//cache match
#if DEBUG_LEVEL>=1
		printf( "cache match %d\n",i );
#endif
		return i;
	}

#if DEBUG_LEVEL>=1
	printf( "no cache nFtpConns %d\n",nFtpConns );
#endif
	if( nFtpConns>=TEN_MAX_CONNS )
		return -4;

	lFtpConns[nFtpConns].FtpIp[0]=FtpIp[0];
	lFtpConns[nFtpConns].FtpIp[1]=FtpIp[1];
	lFtpConns[nFtpConns].FtpIp[2]=FtpIp[2];
	lFtpConns[nFtpConns].FtpIp[3]=FtpIp[3];
	lFtpConns[nFtpConns].FtpIp[4]=FtpIp[4];
	lFtpConns[nFtpConns].CommandConnection=-1;
	lFtpConns[nFtpConns].DataConnection=-1;
	lFtpState[nFtpConns].DirReadCacheData=0;

	strcpy( lFtpState[nFtpConns].FtpUser, FtpUser );
	strcpy( lFtpState[nFtpConns].FtpPass, FtpPass );
	lFtpState[nFtpConns].bAuthed=0;

	lFtpConns[nFtpConns].lastRecv = TenTime();

	nFtpConns++;

#if DEBUG_LEVEL>=1
	printf( "ret %d\n",nFtpConns-1 );
#endif
	return nFtpConns-1;
}

//#define IP4_ADDR(ipaddr, a,b,c,d) (ipaddr)->s_addr = htonl(((u32)(a & 0xff) << 24) | ((u32)(b & 0xff) << 16 ) | ((u32)(c & 0xff) << 8) | (u32)(d & 0xff))

int FtpEmpty( int sid )
{
	char tmp[256];
	int ret, wait;

	wait=2;
	while( 1 )
	{
		if( IsDataFromFtpAvail( sid ) )
		{
			wait=0;
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
	return 1;
}

int FtpClose( int i )
{
	FtpDataClose( i );
	disconnect( lFtpConns[i].CommandConnection );
	lFtpConns[i].CommandConnection = -1;
	lFtpState[i].bAuthed=0;
	lFtpState[i].bData=0;
	lFtpState[i].bBusy=0;

	if( lFtpState[i].DirReadCacheData )
		free( lFtpState[i].DirReadCacheData );
	lFtpState[i].DirReadCacheData=0;

	return 1;
}

void CheckTimeout()
{
	int i;
	for( i=0;i<nFtpConns;i++ )
	{
		if( TenTime()-lFtpConns[i].lastRecv > 30 )
		{
			FtpClose( i );
		}
	}

}

int VerifyConnected( int serverId )
{
	int ret;
	struct sockaddr_in listenerAddr;

	CheckTimeout();

	if( lFtpConns[serverId].CommandConnection == -1 )
	{
#if DEBUG_LEVEL>=1
		printf("VerifyConnected\n");
		printf("socket\n");
#endif
		lFtpConns[serverId].CommandConnection = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

		memset(&listenerAddr, 0, sizeof(listenerAddr));
		listenerAddr.sin_family = AF_INET;
		listenerAddr.sin_port = 0;
		listenerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
/*
		printf("bind\n");
		ret = bind( lFtpConns[serverId].CommandConnection, (struct sockaddr*)&listenerAddr, sizeof(struct sockaddr) );
		if( ret < 0 )
			return 0;
*/
		IP4_ADDR( (struct ip_addr *)&(listenerAddr.sin_addr), 
			lFtpConns[serverId].FtpIp[0],
			lFtpConns[serverId].FtpIp[1],
			lFtpConns[serverId].FtpIp[2],
			lFtpConns[serverId].FtpIp[3] );
		listenerAddr.sin_port = htons( lFtpConns[serverId].FtpIp[4] );

#if DEBUG_LEVEL>=2
		printf("target: %d %d %d %d : %d\n",
			lFtpConns[serverId].FtpIp[0],
			lFtpConns[serverId].FtpIp[1],
			lFtpConns[serverId].FtpIp[2],
			lFtpConns[serverId].FtpIp[3],
			lFtpConns[serverId].FtpIp[4] );
#endif

#if DEBUG_LEVEL>=1
		printf("connect\n");
#endif
		ret = connect( lFtpConns[serverId].CommandConnection, (struct sockaddr*)&listenerAddr, sizeof(struct sockaddr) );
		if( ret < 0 )
		{
			printf("connect failed\nret:%d\n",ret);
			return 0;
		}
#if DEBUG_LEVEL>=1
		printf("done\n");
#endif
	}
	return 1;
}

int SendDataToFtp( int serverId, char *data, int siz )
{
	int s;
	int ret;

	if( serverId<0 || serverId>nFtpConns )
		return -1;

	if( !VerifyConnected(serverId) )
		return -2;

	s = lFtpConns[serverId].CommandConnection;

	ret = send( s, data, siz, 0 );
	if( ret <= 0 )
	{//error
		printf( "SendDataToFtp send error %d\n", ret );
		return ret;
	}
	else
	{//fine 
		lFtpConns[serverId].lastRecv = TenTime();
#if DEBUG_LEVEL>=2
		printf( "<<%s\n", data );
#endif
		return ret; //should be count of send
	}
}

int GetDataFromFtp( int serverId, char *buf, int buf_siz )
{
	int s;
	int ret;

	if( serverId<0 || serverId>nFtpConns )
		return -1;

	if( !VerifyConnected(serverId) )
		return -2;

	s = lFtpConns[serverId].CommandConnection;

	ret = recv( s, buf, buf_siz, 0 );
	if( ret <= 0 )
	{//error
		printf( "GetDataFromFtp recv error %d\n", ret );
		return ret;
	}
	else
	{//fine 
		lFtpConns[serverId].lastRecv = TenTime();
		buf[ret]=0;
#if DEBUG_LEVEL>=2
		printf( ">>%s\n", buf );
#endif
		return ret; //should be count of read
	}
}

int IsDataFromFtpAvail( int serverId )
{
	int s;
	fd_set r;
	struct timeval tv;

	if( serverId<0 || serverId>nFtpConns )
		return -1;

	if( !VerifyConnected(serverId) )
		return -2;

	s = lFtpConns[serverId].CommandConnection;

	FD_ZERO(&r);
	FD_SET(s,&r);
	tv.tv_sec=0;
	tv.tv_usec=0;
	if( select( s+1, &r, 0, 0, &tv ) < 0 )
		return 0;
	if( FD_ISSET(s,&r) )
		return 1;
	else
		return 0;
}










////data conn

int FtpDataConn( int serverId )
{
	char tmp[512];
	char *c,*c2;
	int ret;
	int PasvIp[5],tt1,tt2;
	struct sockaddr_in listenerAddr;


	if( serverId<0 || serverId>nFtpConns )
		return -1;

	if( lFtpConns[serverId].DataConnection!=-1 )
	{
		disconnect( lFtpConns[serverId].DataConnection );
		lFtpConns[serverId].DataConnection=-1;
		lFtpState[serverId].bData=0;
	}


	ret = FtpCommandRet( serverId, "PASV", "", tmp, 510 );
	if( ret != 227 )
		return -1;

	c=ten_strchr( tmp, '(' );
	if( !c )
		return -2;
	c++;

	c2=ten_strchr( c, ')' );
	if( !c2 )
		return -3;
	c2[0]=0;


	c2 = ten_strchr( c, ',' );
	c2[0]=0;
	c2++;
	PasvIp[0]=atoi( c );
	c=c2;

	c2 = ten_strchr( c, ',' );
	c2[0]=0;
	c2++;
	PasvIp[1]=atoi( c );
	c=c2;

	c2 = ten_strchr( c, ',' );
	c2[0]=0;
	c2++;
	PasvIp[2]=atoi( c );
	c=c2;

	c2 = ten_strchr( c, ',' );
	c2[0]=0;
	c2++;
	PasvIp[3]=atoi( c );
	c=c2;

	c2 = ten_strchr( c, ',' );
	c2[0]=0;
	c2++;
	tt1=atoi( c );
	c=c2;

	c2 = ten_strchr( c, ',' );
	c2[0]=0;
	c2++;
	tt2=atoi( c );
	c=c2;

	PasvIp[4]=tt1*256+tt2;



#if DEBUG_LEVEL>=1
	printf("Data Conn\n");
	printf("socket\n");
#endif
	lFtpConns[serverId].DataConnection = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	memset(&listenerAddr, 0, sizeof(listenerAddr));
	listenerAddr.sin_family = AF_INET;
	listenerAddr.sin_port = 0;
	listenerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	IP4_ADDR( (struct ip_addr *)&(listenerAddr.sin_addr), 
		PasvIp[0],
		PasvIp[1],
		PasvIp[2],
		PasvIp[3] );
	listenerAddr.sin_port = htons( PasvIp[4] );

#if DEBUG_LEVEL>=1
	printf("target: %d %d %d %d : %d\n",
		PasvIp[0],
		PasvIp[1],
		PasvIp[2],
		PasvIp[3],
		PasvIp[4] );

	printf("connect\n");
#endif
	ret = connect( lFtpConns[serverId].DataConnection, (struct sockaddr*)&listenerAddr, sizeof(struct sockaddr) );
	if( ret < 0 )
	{
		printf("connect failed\nret:%d\n",ret);
		return -4;
	}
#if DEBUG_LEVEL>=1
	printf("done\n");
#endif

	lFtpState[serverId].bData=1;

	return 1;
}

int FtpDataClose( int serverId )
{
	if( serverId<0 || serverId>nFtpConns )
		return -1;

	if( lFtpConns[serverId].DataConnection!=-1 )
	{
		disconnect( lFtpConns[serverId].DataConnection );
		lFtpConns[serverId].DataConnection=-1;
		lFtpState[serverId].bData=0;
	}

	if( lFtpState[serverId].DirReadCacheData )
		free( lFtpState[serverId].DirReadCacheData );
	lFtpState[serverId].DirReadCacheData=0;


	return 1;
}


int SendDataToFtpDataConn( int serverId, char *data, int siz )
{
	int s;
	int ret;

	if( serverId<0 || serverId>nFtpConns )
		return -1;

	s = lFtpConns[serverId].DataConnection;

	ret = send( s, data, siz, 0 );
	if( ret <= 0 )
	{//error
		printf( "SendDataToFtpDataConn send error %d\n", ret );
		return ret;
	}
	else
	{//fine 
		lFtpConns[serverId].lastRecv = TenTime();
#if DEBUG_LEVEL>=2
		printf( "SendDataToFtpDataConn send ok %d\n", ret );
#endif
		return ret; //should be count of send
	}
}

int GetDataFromFtpDataConn( int serverId, char *buf, int buf_siz )
{
	int s;
	int ret;

	if( serverId<0 || serverId>nFtpConns )
		return -1;

	s = lFtpConns[serverId].DataConnection;

	ret = recv( s, buf, buf_siz, 0 );
	if( ret <= 0 )
	{//error
		printf( "GetDataFromFtpDataConn recv error %d\n", ret );
		return ret;
	}
	else
	{//fine 
		lFtpConns[serverId].lastRecv = TenTime();
#if DEBUG_LEVEL>=2
		printf( "GetDataFromFtpDataConn recv ok %d\n", ret );
#endif
		return ret; //should be count of read
	}
}

int IsDataFromFtpDataConnAvail( int serverId )
{
	int s;
	fd_set r;
	struct timeval tv;

	if( serverId<0 || serverId>nFtpConns )
		return -1;

	s = lFtpConns[serverId].DataConnection;

	FD_ZERO(&r);
	FD_SET(s,&r);
	tv.tv_sec=0;
	tv.tv_usec=0;
	if( select( s+1, &r, 0, 0, &tv ) < 0 )
	{
		printf("IsDataFromFtpDataConnAvail select err\n" );
		return 0;
	}
	if( FD_ISSET(s,&r) )
	{
		return 1;
	}
	else
	{
#if DEBUG_LEVEL>=1
		printf("IsDataFromFtpDataConnAvail not set\n" );
#endif
		return 0;
	}
}

int IsDataSendToFtpDataConnAvail( int serverId )
{
	int s;
	fd_set r;
	struct timeval tv;

	if( serverId<0 || serverId>nFtpConns )
		return -1;

	s = lFtpConns[serverId].DataConnection;

	FD_ZERO(&r);
	FD_SET(s,&r);
	tv.tv_sec=0;
	tv.tv_usec=0;
	if( select( s+1, 0, &r, 0, &tv ) < 0 )
	{
		printf("IsDataSendToFtpDataConnAvail select err\n" );
		return 0;
	}
	if( FD_ISSET(s,&r) )
	{
		return 1;
	}
	else
	{
#if DEBUG_LEVEL>=1
		printf("IsDataSendToFtpDataConnAvail not set\n" );
#endif
		return 0;
	}
}






int TenTime()
{
	int sec,usec;
	iop_sys_clock_t tstart;
	GetSystemTime( &tstart );      
	SysClock2USec( &tstart, &sec, &usec );
	return sec;
}

