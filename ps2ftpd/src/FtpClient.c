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
#define BUFFER_OFFSET 4096	// use this if you need to use buffer & push it to FtpClient_Send()

// list of commands
FtpCommand commands[] = 
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
	{ FTPCMD_MKD, "xmkd" },
	{ FTPCMD_RMD, "rmd" },
	{ FTPCMD_RMD, "xrmd" },
	{ FTPCMD_SITE, "site" }, 
	{ -1, NULL }

	// commands to implement
	
	// SIZE
};

void FtpClient_Create( FtpClient* pClient, FtpServer* pServer, int iControlSocket )
{
	assert( pClient && pServer );

	memset( pClient, 0, sizeof(FtpClient) );

	pClient->m_pServer = pServer;
	pClient->m_iControlSocket = iControlSocket;

	pClient->m_iDataSocket = -1;
	pClient->m_uiDataBufferSize = 0;
	pClient->m_eDataMode = DATAMODE_IDLE;
	pClient->m_eDataAction = DATAACTION_NONE;
	pClient->m_eConnState = CONNSTATE_IDLE;
	pClient->m_iRemotePort = -1;
	pClient->m_iRemoteAddress[0] = 0;
	pClient->m_iRemoteAddress[1] = 0;
	pClient->m_iRemoteAddress[2] = 0;
	pClient->m_iRemoteAddress[3] = 0;
	pClient->m_uiDataOffset = 0;

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

	FtpClient_OnDataCleanup(pClient);
	FileSystem_Destroy(&pClient->m_kContext);

	if( -1 != pClient->m_iControlSocket )
		disconnect( pClient->m_iControlSocket );

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
	Ftpclient_Send( pClient, 220, "ps2ftpd ready." );
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

		for( i = 0; commands[i].m_iCommand != -1; i++ )
		{
			if(!strcmp(cmd,commands[i].m_pName))
			{
				cmdresult = commands[i].m_iCommand;
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
						ip[i] = strtol(val,NULL,10);
					}
					else if( 4 == i )
					{
						port = strtol(val,NULL,10)*256;
					}
					else if( 5 == i )
					{
						port += strtol(val,NULL,10);
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

			case FTPCMD_SITE:
			{
				char* cmd = strtok(NULL,"");

				if(cmd)
					FtpClient_OnCmdSite(pClient,cmd);
				else
					FtpClient_Send( pClient, 500, "SITE requires parameters." );
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

	pClient->m_uiDataBufferSize = sizeof(buffer);
	pClient->m_uiDataOffset = 0;
}

void FtpClient_OnDataRead( FtpClient* pClient )
{
	assert( pClient );

	switch( pClient->m_eDataAction )
	{
		case DATAACTION_STOR:
		{
			int rv = recv(pClient->m_iDataSocket,buffer,pClient->m_uiDataBufferSize,0);

			if( rv > 0 )
			{
				int sv = FileSystem_WriteFile( &pClient->m_kContext, buffer, rv );

				if( sv <= 0 )
					FtpClient_OnDataFailed(pClient,"Local write failed");

				pClient->m_uiDataOffset += sv;
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
			int rv = FileSystem_ReadFile(&pClient->m_kContext,buffer,pClient->m_uiDataBufferSize);

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

				pClient->m_uiDataOffset += sv;
			}
			else
				FtpClient_OnDataComplete(pClient,0,NULL);
		}
		break;

		case DATAACTION_LIST:
		case DATAACTION_NLST:
		{
			FSFileInfo* pInfo = (FSFileInfo*)(buffer+BUFFER_OFFSET);

			if( FileSystem_ReadDir( &pClient->m_kContext, pInfo ) >= 0 )
			{
				buffer[0] = '\0';
				if( DATAACTION_LIST == pClient->m_eDataAction )
				{
					int i;

					// this one needs a rewrite

					strcat( buffer, (FT_DIRECTORY == pInfo->m_eType) ? "d" : (FT_LINK == pInfo->m_eType) ? "l" : "-" );
					for( i = 0; i < 9; i++ )
					{
						if( pInfo->m_iProtection&(1<<(8-i)) )
						{
							switch( i%3 )
							{
								case 0: strcat( buffer, "r" ); break;
								case 1: strcat( buffer, "w" ); break;
								case 2: strcat( buffer, "x" ); break;
							}
						}
						else
							strcat( buffer, "-" );
					}
					strcat( buffer, " ps2 ps2 " );
					itoa( buffer + strlen(buffer), pInfo->m_iSize );
					strcat( buffer, " " );
					itoa( buffer + strlen(buffer), pInfo->m_TS.m_iYear );
					strcat( buffer, "-" );
					if( pInfo->m_TS.m_iMonth < 10 )
						strcat(buffer, "0" );
					itoa( buffer + strlen(buffer), pInfo->m_TS.m_iMonth );
					strcat( buffer, "-" );
					if( pInfo->m_TS.m_iDay < 10 )
						strcat( buffer, "0" );
					itoa( buffer + strlen(buffer), pInfo->m_TS.m_iDay );
					strcat( buffer, " " );
					if( pInfo->m_TS.m_iHour < 10 )
						strcat( buffer, "0" );
					itoa( buffer + strlen(buffer), pInfo->m_TS.m_iHour );
					strcat( buffer, ":" );
					if( pInfo->m_TS.m_iMinute < 10 )
						strcat( buffer, "0" );
					itoa( buffer + strlen(buffer), pInfo->m_TS.m_iMinute );
					strcat( buffer, " " );
				}
				strcat( buffer, pInfo->m_Name );
				strcat( buffer, "\r\n" );
//				sprintf(buffer,"%srwxr-xr-x user group %d Jan 01 00:01 %s\r\n",(FT_DIRECTORY == pInfo->m_eType) ? "d" : "-", pInfo->m_iSize, pInfo->m_Name);
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

	// thanks to clients implemented incorrectly, we must close the connection after completed transfer
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
