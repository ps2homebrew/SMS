/*
 * FtpClient.c - FTP client methods
 *
 * Copyright (C) 2004 Jesper Svennevid
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 */

#include "FtpClient.h"
#include "FtpServer.h"

#ifndef LINUX
#include "irx_imports.h"
#define assert(x)
#else
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define disconnect(s) close(s)
#endif

#include <errno.h>

#define MAX_CLIENTS 4

#ifndef LINUX
// cheap mans atoi, no support for negative values
int atoi( const char*t )
{
	int val = 0;

	while((' ' == *t)||('\t' == *t)) t++;

	while(*t && ((*t >= '0')&&(*t <= '9')))
	{
		val *= 10;
		val += *t-'0';
		t++;
	}
	return val;
}
#endif

// cheap mans itoa
char* itoa(char* in, int val)
{
	char tbuf[16];
	char* t = tbuf;

	do
	{
		*t++ = (val%10)+'0';
		val /= 10;
	} while(val > 0);

	while( t > tbuf )
		*in++ = *--t;

	*in = 0;

	return in;
}

// this shared buffer is used for everything the clients do
static char buffer[8192];
#define BUFFER_OFFSET 2048	// use this if you need to use buffer & push it to FtpClient_Send()

// list of commands
struct
{
	int command;
	const char* string;
} commands[] = 
{
	{ FTPCMD_USER, "user" },
	{ FTPCMD_PASS, "pass" },
	{ FTPCMD_PASV, "pasv" },
	{ FTPCMD_PORT, "port" },
	{ FTPCMD_QUIT, "quit" },
	{ FTPCMD_SYST, "syst" },
	{ FTPCMD_LIST, "list" },
	{ FTPCMD_NLST, "nlst" },
	{ FTPCMD_RETR, "retr" },
	{ FTPCMD_TYPE, "type" },
	{ FTPCMD_STOR, "stor" },
	{ FTPCMD_PWD, "pwd" },
	{ FTPCMD_PWD, "xpwd" },
	{ FTPCMD_CWD, "cwd" },
	{ FTPCMD_CDUP, "cdup" },
	{ FTPCMD_DELE, "dele" },
	{ FTPCMD_MKD, "mkd" },
	{ FTPCMD_RMD, "rmd" },
	{ -1, NULL }

	// commands to implement
	
	// SIZE
}; 

void FtpClient_Create( FtpClient* pClient, FtpServer* pServer, int iControlSocket )
{
	assert( pClient && pServer );

	pClient->m_pServer = pServer;
	pClient->m_iControlSocket = iControlSocket;

	pClient->m_iDataSocket = -1;
	pClient->m_iDataBufferSize = 0;
	pClient->m_eDataMode = DATAMODE_IDLE;
	pClient->m_eDataAction = DATAACTION_NONE;
	pClient->m_eConnState = CONNSTATE_IDLE;
	pClient->m_iRemotePort = -1;
	pClient->m_iRemoteAddress[0] = 0;
	pClient->m_iRemoteAddress[1] = 0;
	pClient->m_iRemoteAddress[2] = 0;
	pClient->m_iRemoteAddress[3] = 0;

	pClient->m_kContainer.m_pClient = pClient;
	pClient->m_kContainer.m_pPrev = &pClient->m_kContainer;
	pClient->m_kContainer.m_pNext = &pClient->m_kContainer;

	pClient->m_iCommandOffset = 0;
	pClient->m_CommandBuffer[0] = '\0';

	FileSystem_Create(&pClient->m_kContext);
}

void FtpClient_Destroy( FtpClient* pClient )
{
	assert( pClient );

	FileSystem_Destroy(&pClient->m_kContext);

	if( -1 != pClient->m_iControlSocket )
		disconnect( pClient->m_iControlSocket );

	if( -1 != pClient->m_iDataSocket )
		disconnect( pClient->m_iDataSocket );

	pClient->m_pServer = NULL;
}

void FtpClient_Send( FtpClient* pClient, int iReturnCode, const char* pString )
{
	assert( pClient );

	buffer[0] = '\0';
	itoa(buffer,iReturnCode);
	strcat(buffer," ");
	strcat(buffer,pString);
	strcat(buffer,"\r\n");
//	sprintf(buf,"%d %s\r\n",iReturnCode,pString);

	// TODO: use a sendqueue here & handle in select-phase
	send( pClient->m_iControlSocket, buffer, strlen(buffer), 0 );
#ifdef BEDUG
	printf("%08x >> %s",(unsigned int)pClient,buffer);
#endif
}

void FtpClient_OnConnect( FtpClient* pClient )
{
	assert( pClient );

	FtpClient_Send( pClient, 220, "ps2ftpd ready." );
}

void FtpClient_OnCommand( FtpClient* pClient, const char* pString )
{
	char* cmd;
	char* c;

	assert( pClient );

	strcpy(buffer,pString);

	cmd = c = strtok(buffer," ");
	while(*c)
	{
		*c = tolower(*c);
		c++;
	}

	if(cmd)
	{
		int i = 0;
		int cmdresult = -1;
		for( i = 0; commands[i].command != -1; i++ )
		{
			if(!strcmp(cmd,commands[i].string))
			{
				cmdresult = commands[i].command;
				break;
			}
		}
	
		switch( cmdresult )
		{
			// USER <name>
			case FTPCMD_USER:
			{
				char* user = strtok(NULL,"");

				if(user)
					FtpClient_OnCmdUser(pClient,user);
				else
					FtpClient_Send( pClient, 500, "USER: Requires a parameter." );
			}
			break;

			// PASS <password>
			case FTPCMD_PASS:
			{
				char* pass = strtok(NULL,"");

				if(pass)
					FtpClient_OnCmdPass(pClient,pass);
				else
					FtpClient_Send( pClient, 530, "Login incorrect." );
			}
			break;

			// PASV
			case FTPCMD_PASV:
			{
				FtpClient_OnCmdPasv(pClient);
			}
			break;

			// PORT a0,a1,a2,a3,p0,p1
			case FTPCMD_PORT:
			{
				int i;
				int ip[4];
				int port = 0;

				for( i = 0; i < 6; i++ )
				{
					char* val = strtok(NULL,",");

					if(!val)
						break;

					if( i >= 0 && i < 4 )
					{
						ip[i] = atoi(val);
					}
					else if( 4 == i )
					{
						port = atoi(val)*256;
					}
					else if( 5 == i )
					{
						port += atoi(val);
					}
				}

				if(6 == i)
					FtpClient_OnCmdPort(pClient,ip,port);
				else
					FtpClient_Send( pClient, 501, "Illegal PORT command." );
			}
			break;

			// QUIT
			case FTPCMD_QUIT:
			{
				FtpClient_OnCmdQuit(pClient);
			}
			break;

			// SYST
			case FTPCMD_SYST:
			{
				FtpClient_OnCmdSyst(pClient);
			}
			break;

			// LIST
			case FTPCMD_LIST:
			{
				char* path = strtok(NULL,"");

				if(path)
					FtpClient_OnCmdList(pClient,path,0);
				else
					FtpClient_OnCmdList(pClient,"",0);
			}
			break;

			// NLST
			case FTPCMD_NLST:
			{
				char* path = strtok(NULL,"");

				if(path)
					FtpClient_OnCmdList(pClient,path,1);
				else
					FtpClient_OnCmdList(pClient,"",1);
			}
			break;

			// TYPE <type>
			case FTPCMD_TYPE:
			{
				char* type = strtok(NULL,"");

				if(type)
					FtpClient_OnCmdType(pClient,type);
				else
					FtpClient_Send( pClient, 501, "Illegal TYPE Command." );
			}
			break;

			// RETR <file>
			case FTPCMD_RETR:
			{
				char* file = strtok(NULL,"");

				if(file)
					FtpClient_OnCmdRetr(pClient,file);
				else
					FtpClient_Send( pClient, 501, "Illegal RETR Command." );
			}
			break;

			// STOR <file>
			case FTPCMD_STOR:
			{
				char* file = strtok(NULL,"");

				if(file)
					FtpClient_OnCmdStor(pClient,file);
				else
					FtpClient_Send( pClient, 501, "Illegal STOR Command." );
			}
			break;

			case FTPCMD_PWD:
			{
				FtpClient_OnCmdPwd(pClient);
			}
			break;

			case FTPCMD_CWD:
			{
				char* path = strtok(NULL,"");

				if(path)
					FtpClient_OnCmdCwd(pClient,path);
				else
					FtpClient_Send( pClient, 501, "Illegal CWD Command." );
			}
			break;

			case FTPCMD_CDUP:
			{
				FtpClient_OnCmdCwd(pClient,"..");
			}
			break;

			case FTPCMD_DELE:
			{
				char* file = strtok(NULL,"");

				if(file)
					FtpClient_OnCmdDele(pClient,file);
				else
					FtpClient_Send( pClient, 501, "Illegal DELE Command." );
			}
			break;

			case FTPCMD_MKD:
			{
				char* dir = strtok(NULL,"");

				if(dir)
					FtpClient_OnCmdMkd(pClient,dir);
				else
					FtpClient_Send( pClient, 501, "Illegal MKD Command." );
			}
			break;

			case FTPCMD_RMD:
			{
				char* dir = strtok(NULL,"");

				if(dir)
					FtpClient_OnCmdRmd(pClient,dir);
				else
					FtpClient_Send( pClient, 501, "Illegal MKD Command." );
			}
			break;

			default:
				FtpClient_Send( pClient, 500, "Not understood." );
			break;
		}
	}
	else
		FtpClient_Send( pClient, 500, "Not understood." );
}

void FtpClient_OnCmdQuit( FtpClient* pClient )
{
	assert( pClient );

	FtpClient_Send( pClient, 221, "Goodbye." );
	FtpServer_OnClientDisconnect(pClient->m_pServer,pClient);
}

void FtpClient_OnCmdUser( FtpClient* pClient, const char* pUser )
{
	assert( pClient );

	if(!strcmp(pUser,"anonymous"))
		FtpClient_Send(pClient,331,"Anonymous login ok.");
	else
		FtpClient_Send(pClient,331,"Password required for user.");
}

void FtpClient_OnCmdPass( FtpClient* pClient, const char* pPass )
{
	assert( pClient );

	// TODO: authentication

	FtpClient_Send(pClient,230,"User logged in.");

	// 530 Login incorrect.
}

void FtpClient_OnCmdPasv( FtpClient* pClient )
{
	struct sockaddr_in sa;
	socklen_t sl;
	int s;
	unsigned int addr;
	unsigned short port;
	char* buf;

	assert( pClient );

	FtpClient_OnDataCleanup(pClient);

	// get socket address we will listen to

	sl = sizeof(sa);
	if( getsockname( pClient->m_iControlSocket, (struct sockaddr*)&sa,&sl ) < 0 )
	{
		FtpClient_Send( pClient, 500, "Could not enter passive mode." );
		return;
	}
	sa.sin_port = 0;

	if( (s = socket( AF_INET, SOCK_STREAM, 0)) < 0 )
  {
  	disconnect(s);
		FtpClient_Send( pClient, 500, "Could not enter passive mode." );
		return;
	}

// FIXME
/*  if( fcntl( s, F_SETFL, O_NONBLOCK ) < 0 )
  {
  	disconnect(s);
		FtpClient_Send( pClient, 500, "Could not enter passive mode." );
		return;
	}
*/
	if( bind( s, (struct sockaddr*)&sa, sl ) < 0 )
	{
  	disconnect(s);
		FtpClient_Send( pClient, 500, "Could not enter passive mode." );
		return;
	}

	if( listen( s, 1 ) < 0 )
  {
  	disconnect(s);
		FtpClient_Send( pClient, 500, "Could not enter passive mode." );
		return;
	}
                
	// report back

	sl = sizeof(sa);
	if( getsockname( s, (struct sockaddr*)&sa,&sl ) < 0 )
	{
		FtpClient_Send( pClient, 500, "Could not enter passive mode." );
		return;
	}

	pClient->m_iDataSocket = s;
	pClient->m_eConnState = CONNSTATE_LISTEN;

	addr = htonl(sa.sin_addr.s_addr);
	port = htons(sa.sin_port);

	buf = buffer+BUFFER_OFFSET;
	strcpy(buf,"Entering passive mode (");
	itoa(buf+strlen(buf),(addr>>24)&0xff);
	strcat(buf,",");
	itoa(buf+strlen(buf),(addr>>16)&0xff);
	strcat(buf,",");
	itoa(buf+strlen(buf),(addr>>8)&0xff);
	strcat(buf,",");
	itoa(buf+strlen(buf),(addr)&0xff);
	strcat(buf,",");
	itoa(buf+strlen(buf),port>>8);
	strcat(buf,",");
	itoa(buf+strlen(buf),port&0xff);
	strcat(buf,").");
/*
	sprintf( buffer+BUFFER_OFFSET, "Entering passive mode (%d,%d,%d,%d,%d,%d).",
										(addr>>24)&0xff, (addr>>16)&0xff, (addr>>8)&0xff, addr&0xff,
										port>>8,port&0xff );
*/

	FtpClient_Send( pClient, 227, buffer+BUFFER_OFFSET );
}

void FtpClient_OnCmdPort( FtpClient* pClient, int* ip, int port )
{
	assert( pClient );

	if( port >= 1024 )
	{
		// TODO: validate IP?

		pClient->m_iRemoteAddress[0] = ip[0];
		pClient->m_iRemoteAddress[1] = ip[1];
		pClient->m_iRemoteAddress[2] = ip[2];
		pClient->m_iRemoteAddress[3] = ip[3];

		pClient->m_iRemotePort = port;

		FtpClient_Send( pClient, 200, "PORT command successful." );
	}
	else
		FtpClient_Send( pClient, 500, "Illegal PORT command." );
}

void FtpClient_OnCmdSyst( FtpClient* pClient )
{
	assert( pClient );

	FtpClient_Send( pClient, 215, "UNIX Type: L8" );
}

void FtpClient_OnCmdList( struct FtpClient* pClient, const char* pPath, int iNamesOnly )
{
	assert( pClient );

	if( iNamesOnly )
		pClient->m_eDataAction = DATAACTION_NLST;
	else
		pClient->m_eDataAction = DATAACTION_LIST;

	if( FileSystem_OpenDir(&pClient->m_kContext,pPath) < 0 )
	{
		FtpClient_Send( pClient, 500, "Unable to open directory." );
		return;
	}

	if( CONNSTATE_RUNNING == pClient->m_eConnState )
	{
		FtpClient_OnDataConnected( pClient );
	}
	else
	{
		if( -1 != pClient->m_iRemotePort )
		{
			FtpClient_OnDataConnect( pClient, pClient->m_iRemoteAddress, pClient->m_iRemotePort );
		}
		else if( CONNSTATE_LISTEN != pClient->m_eConnState )
			FtpClient_Send( pClient, 500, "Unable to build data connection." );
	}
}

void FtpClient_OnCmdType( FtpClient* pClient, const char* pType )
{
	assert( pClient );

	// only binary for now, but we accept them anyway....

	if(!strcmp(pType,"I") || !strcmp(pType,"i"))
		FtpClient_Send( pClient, 200, "Type set to I." );
	else if(!strcmp(pType,"A")||!strcmp(pType,"a"))
		FtpClient_Send( pClient, 200, "Type set to A." );
	else
		FtpClient_Send( pClient, 500, "Illegal TYPE Command." );
}

void FtpClient_OnCmdRetr( FtpClient* pClient, const char* pFile )
{
	assert( pClient );

	if( FileSystem_OpenFile( &pClient->m_kContext, pFile, FM_READ ) < 0 )
	{
		FtpClient_Send( pClient, 500, "File not found." );
		return;
	}

	pClient->m_eDataAction = DATAACTION_RETR;

	if( CONNSTATE_RUNNING == pClient->m_eConnState )
	{
		FtpClient_OnDataConnected( pClient );
	}
	else
	{
		if( -1 != pClient->m_iRemotePort )
			FtpClient_OnDataConnect( pClient, pClient->m_iRemoteAddress, pClient->m_iRemotePort );
		else if( CONNSTATE_LISTEN != pClient->m_eConnState )
			FtpClient_Send( pClient, 500, "Unable to build data connection." );
	}
}

void FtpClient_OnCmdStor( FtpClient* pClient, const char* pFile )
{
	assert( pClient );

	if( FileSystem_OpenFile( &pClient->m_kContext, pFile, FM_CREATE ) < 0 )
	{
		FtpClient_Send( pClient, 500, "Could not create file." );
		return;
	}

	pClient->m_eDataAction = DATAACTION_STOR;

	if( CONNSTATE_RUNNING == pClient->m_eConnState )
	{
		FtpClient_OnDataConnected( pClient );
	}
	else
	{
		if( -1 != pClient->m_iRemotePort )
			FtpClient_OnDataConnect( pClient, pClient->m_iRemoteAddress, pClient->m_iRemotePort );
		else if( CONNSTATE_LISTEN != pClient->m_eConnState )
			FtpClient_Send( pClient, 500, "Unable to build data connection." );
	}
}

void FtpClient_OnCmdPwd( FtpClient* pClient )
{
	char* buf2 = buffer + BUFFER_OFFSET;

	buf2[0] = '\0';
	strcat( buf2, "\"" );
	strcat( buf2, pClient->m_kContext.m_Path );
	strcat( buf2, "\" is current directory." );

	FtpClient_Send( pClient, 257, buf2 );
}

void FtpClient_OnCmdCwd( FtpClient* pClient, const char* pPath )
{
	if( FileSystem_ChangeDir(&pClient->m_kContext,pPath) < 0 )
		FtpClient_Send( pClient, 550, "No such file or directory" );
	else
		FtpClient_Send( pClient, 250, "CWD command successfull." );
}

void FtpClient_OnCmdDele( FtpClient* pClient, const char* pFile )
{
	if( FileSystem_DeleteFile(&pClient->m_kContext,pFile) < 0 )
		FtpClient_Send( pClient, 550, "No such file or directory" );
	else
		FtpClient_Send( pClient, 250, "DELE command successfull." );
}

void FtpClient_OnCmdMkd( FtpClient* pClient, const char* pDir )
{
	if( FileSystem_CreateDir(&pClient->m_kContext,pDir) < 0 )
		FtpClient_Send( pClient, 550, "Failed creating directory" );
	else
		FtpClient_Send( pClient, 250, "MKD command successfull." );
}

void FtpClient_OnCmdRmd( FtpClient* pClient, const char* pDir )
{
	if( FileSystem_DeleteDir(&pClient->m_kContext,pDir) < 0 )
		FtpClient_Send( pClient, 550, "No such file or directory" );
	else
		FtpClient_Send( pClient, 250, "RMD command successfull." );
}

void FtpClient_OnDataConnect( FtpClient* pClient,  int* ip, int port )
{
	int s;
	struct sockaddr_in sa;

	assert( pClient );

	// close any present socket to avoid resource leakage

	if( -1 != pClient->m_iDataSocket )
	{
		disconnect(pClient->m_iDataSocket);
		pClient->m_iDataSocket = -1;
	}

	if( (s = socket( PF_INET, SOCK_STREAM, 0 )) < 0 )
	{
#ifdef DEBUG
		printf("ps2ftpd: failed to create data socket\n");
#endif
		FtpClient_OnDataFailed(pClient,NULL);
		return;
	}

	pClient->m_iDataSocket = s;
/*
	FIXME

	if( fcntl( s, F_SETFL, O_NONBLOCK ) < 0 )
	{
		FtpClient_OnDataFailed(pClient,NULL);
		return;
	}
*/

	memset(&sa,0,sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(pClient->m_iRemotePort);
	sa.sin_addr.s_addr = htonl((ip[0]<<24)|(ip[1]<<16)|(ip[2]<<8)|(ip[3]));

	if( connect( s, (struct sockaddr*)&sa, sizeof(sa) ) < 0 )
	{
		// if we get an error, see if it's a non-blocking connection

		unsigned int err;
		socklen_t sl = sizeof(err);

		if(	getsockopt( pClient->m_iDataSocket, SOL_SOCKET, SO_ERROR, &err, &sl ) || (EINPROGRESS != err) )
		{
			FtpClient_OnDataFailed(pClient,NULL);
			return;
		}

		pClient->m_eConnState = CONNSTATE_CONNECT;
	}
	else
	{
		pClient->m_iDataSocket = s;
		pClient->m_eConnState = CONNSTATE_RUNNING;
		FtpClient_OnDataConnected( pClient );
	}
}

void FtpClient_OnDataConnected( FtpClient* pClient )
{
	assert( pClient );

	switch( pClient->m_eDataAction )
	{
		case DATAACTION_LIST:
		case DATAACTION_NLST:
		{
			pClient->m_eDataMode = DATAMODE_WRITE;
			pClient->m_eConnState = CONNSTATE_RUNNING;

			FtpClient_Send( pClient, 150, "Opening ASCII mode data connection for file list" );
		}
		break;

		case DATAACTION_RETR:
		{
			pClient->m_eDataMode = DATAMODE_WRITE;
			pClient->m_eConnState = CONNSTATE_RUNNING;

			FtpClient_Send( pClient, 150, "Opening BINARY Connection for file transfer" );
		}
		break;

		case DATAACTION_STOR:
		{
			pClient->m_eDataMode = DATAMODE_READ;
			pClient->m_eConnState = CONNSTATE_RUNNING;

			FtpClient_Send( pClient, 150, "Opening BINARY Connection for file transfer" );
		}
		break;
	
		// something has gone wrong, but we have an open socket here, keep it for re-use (some clients support it)
		default:
		{
			pClient->m_eDataMode = DATAMODE_IDLE;
			pClient->m_eConnState = CONNSTATE_RUNNING;
		}
		break;
	}

	if( DATAMODE_WRITE == pClient->m_eDataMode )
	{
		socklen_t sl;
		sl = sizeof(pClient->m_iDataBufferSize);
		if(	!getsockopt( pClient->m_iDataSocket, SOL_SOCKET, SO_SNDBUF, &(pClient->m_iDataBufferSize), &sl ) )
		{
			pClient->m_iDataBufferSize = sizeof(buffer);
		}
	}
	else if( DATAMODE_READ == pClient->m_eDataMode )
	{
		socklen_t sl;

		sl = sizeof(pClient->m_iDataBufferSize);
		if(	!getsockopt( pClient->m_iDataSocket, SOL_SOCKET, SO_RCVBUF, &(pClient->m_iDataBufferSize), &sl ) )
		{
			pClient->m_iDataBufferSize = sizeof(buffer);
		}
	}
	else
		pClient->m_iDataBufferSize = 0;

//	if( pClient->m_iDataBufferSize > sizeof(buffer) )
	pClient->m_iDataBufferSize = sizeof(buffer);
}

void FtpClient_OnDataRead( FtpClient* pClient )
{
	assert( pClient );

	switch( pClient->m_eDataAction )
	{
		case DATAACTION_STOR:
		{
			int rv;

			rv = recv(pClient->m_iDataSocket,buffer,pClient->m_iDataBufferSize,0);

			if( rv > 0 )
			{
				int sv = FileSystem_WriteFile( &pClient->m_kContext, buffer, rv );

				if( sv <= 0 )
					FtpClient_OnDataFailed(pClient,"Local write failed");
			}
			else
				FtpClient_OnDataComplete(pClient,0,NULL);
		}
		break;

		default:
			FtpClient_OnDataFailed(pClient,NULL);
		break;
	}
}

void FtpClient_OnDataWrite( FtpClient* pClient )
{
	assert( pClient );

	switch( pClient->m_eDataAction )
	{
		case DATAACTION_RETR:
		{
			int rv = FileSystem_ReadFile(&pClient->m_kContext,buffer,pClient->m_iDataBufferSize);

			if( -1 == rv )
			{
				FtpClient_OnDataFailed(pClient,"Failed reading data");
				return;
			}

			if( rv > 0 )
			{
				int sv = send( pClient->m_iDataSocket, buffer, rv, 0 );

				if( sv <= 0 )
					FtpClient_OnDataFailed(pClient,"Premature client disconnect");
			}
			else
				FtpClient_OnDataComplete(pClient,0,NULL);
		}
		break;

		case DATAACTION_LIST:
		case DATAACTION_NLST:
		{
			FSDirectory* pDir = (FSDirectory*)(buffer+BUFFER_OFFSET);

			if( FileSystem_ReadDir( &pClient->m_kContext, pDir ) >= 0 )
			{
				buffer[0] = '\0';
				if( DATAACTION_LIST == pClient->m_eDataAction )
				{
					strcat( buffer, (FT_DIRECTORY == pDir->m_eType) ? "d" : "-" );
					strcat( buffer, "rwxr-xr-x user group " );
					itoa( buffer + strlen(buffer), pDir->m_iSize );
					strcat( buffer, " Jan 01 00:01 " );
				}
				strcat( buffer, pDir->m_Name );
				strcat( buffer, "\r\n" );
//				sprintf(buffer,"%srwxr-xr-x user group %d Jan 01 00:01 %s\r\n",(FT_DIRECTORY == pDir->m_eType) ? "d" : "-", pDir->m_iSize, pDir->m_Name);
				send(pClient->m_iDataSocket,buffer,strlen(buffer),0);
			}
			else
				FtpClient_OnDataComplete(pClient,0,NULL);
		}
		break;

		default:
			FtpClient_OnDataFailed(pClient,NULL);
		break;
	}
}

void FtpClient_OnDataComplete( FtpClient* pClient, int iReturnCode, const char* pMessage )
{
	assert( pClient );

	if( pMessage && iReturnCode > 0 )
	{
		FtpClient_Send( pClient, iReturnCode, pMessage );
	}
	else
	{
		switch( pClient->m_eDataAction )
		{
			case DATAACTION_NLST: FtpClient_Send( pClient, 226, "NLST command successful." ); break;
			case DATAACTION_LIST: FtpClient_Send( pClient, 226, "LIST command successful." ); break;
			case DATAACTION_RETR: FtpClient_Send( pClient, 226, "RETR command successful." ); break;
			case DATAACTION_STOR: FtpClient_Send( pClient, 226, "STOR command successful." ); break;
			
			default:
			break;
		}
	}

	FtpClient_OnDataCleanup(pClient);
}

void FtpClient_OnDataFailed( FtpClient* pClient, const char* pMessage  )
{
	assert( pClient );

	pClient->m_eDataAction = DATAACTION_NONE;
	FtpClient_OnDataComplete(pClient, 500, pMessage ? pMessage : "Transfer failed" );
}

void FtpClient_OnDataCleanup( FtpClient* pClient )
{
	assert( pClient );

	FileSystem_Close(&pClient->m_kContext);

	if( -1 != pClient->m_iDataSocket )
	{
		disconnect( pClient->m_iDataSocket );
		pClient->m_iDataSocket = -1;
	}

	pClient->m_iRemotePort = -1;
	pClient->m_iRemoteAddress[0] = 0;
	pClient->m_iRemoteAddress[1] = 0;
	pClient->m_iRemoteAddress[2] = 0;
	pClient->m_iRemoteAddress[3] = 0;

	pClient->m_eConnState = CONNSTATE_IDLE;
	pClient->m_eDataMode = DATAMODE_IDLE;
}
