/*
 * FtpCommands.c - FTP command handlers
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

// present in FtpClient.c
extern char* itoa(char* in, int val);

static char buffer[512];

void FtpClient_OnCmdQuit( FtpClient* pClient )
{
	assert( pClient );

	FtpClient_Send( pClient, 221, "Goodbye." );
	FtpServer_OnClientDisconnect(pClient->m_pServer,pClient);
}

void FtpClient_OnCmdUser( FtpClient* pClient, const char* pUser )
{
	assert( pClient );

	if(!strcmp(pUser,"anonymous") && pClient->m_pServer->m_iAnonymous )
	{
		// anonymous login, user authenticated
		pClient->m_eAuthState = AUTHSTATE_VALID;

		FtpClient_Send(pClient,230,"Anonymous login ok.");
	}
	else
	{
		// allow password check
		pClient->m_eAuthState = !strcmp(pUser,pClient->m_pServer->m_Username) ? AUTHSTATE_PASSWORD : AUTHSTATE_FAKEPASS;

		FtpClient_Send(pClient,331,"Password required for user."); // after this line, username is destroyed
	}
}

void FtpClient_OnCmdPass( FtpClient* pClient, const char* pPass )
{
	if( (AUTHSTATE_PASSWORD == pClient->m_eAuthState) && !strcmp(pPass,pClient->m_pServer->m_Password) )
	{
		// password matches, allow login
		FtpClient_Send(pClient,230,"User logged in.");
		pClient->m_eAuthState = AUTHSTATE_VALID;
	}
	else
	{
		// password didn't match, or we had no valid login

		if( AUTHSTATE_INVALID != pClient->m_eAuthState )
		{
			FtpClient_Send(pClient,530,"Login incorrect.");

			// disconnect client if more than 3 attempts to login has been made

			pClient->m_iAuthAttempt++;
			if( pClient->m_iAuthAttempt > 3 )
				FtpServer_OnClientDisconnect( pClient->m_pServer, pClient );

			pClient->m_eAuthState = AUTHSTATE_INVALID;
		}
		else
			FtpClient_Send(pClient,503,"Login with USER first.");
	}
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

	// store socket and flag us as listening

	pClient->m_iDataSocket = s;
	pClient->m_eConnState = CONNSTATE_LISTEN;

	// generate reply

	addr = htonl(sa.sin_addr.s_addr);
	port = htons(sa.sin_port);

	buf = buffer;
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
	sprintf( buffer, "Entering passive mode (%d,%d,%d,%d,%d,%d).",
										(addr>>24)&0xff, (addr>>16)&0xff, (addr>>8)&0xff, addr&0xff,
										port>>8,port&0xff );
*/

	FtpClient_Send( pClient, 227, buffer );
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

	// TODO: is this proper?

	FtpClient_Send( pClient, 215, "UNIX Type: L8" );
}

void FtpClient_OnCmdList( struct FtpClient* pClient, const char* pPath, int iNamesOnly )
{
	assert( pClient );

	if( iNamesOnly )
		pClient->m_eDataAction = DATAACTION_NLST;
	else
		pClient->m_eDataAction = DATAACTION_LIST;

	// attempt to open directory

	if( FileSystem_OpenDir(&pClient->m_kContext,pPath) < 0 )
	{
		FtpClient_Send( pClient, 500, "Unable to open directory." );
		return;
	}

	FtpClient_HandleDataConnect( pClient );
}

void FtpClient_OnCmdType( FtpClient* pClient, const char* pType )
{
	assert( pClient );

	// TODO: write proper handling of this command

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

	FtpClient_HandleDataConnect( pClient );
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

	FtpClient_HandleDataConnect( pClient );
}

void FtpClient_OnCmdPwd( FtpClient* pClient )
{
	char* buf2 = buffer;

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
		FtpClient_Send( pClient, 250, "CWD command successful." );
}

void FtpClient_OnCmdDele( FtpClient* pClient, const char* pFile )
{
	if( FileSystem_DeleteFile(&pClient->m_kContext,pFile) < 0 )
		FtpClient_Send( pClient, 550, "No such file or directory" );
	else
		FtpClient_Send( pClient, 250, "DELE command successful." );
}

void FtpClient_OnCmdMkd( FtpClient* pClient, const char* pDir )
{
	if( FileSystem_CreateDir(&pClient->m_kContext,pDir) < 0 )
		FtpClient_Send( pClient, 550, "Failed creating directory" );
	else
		FtpClient_Send( pClient, 250, "MKD command successful." );
}

void FtpClient_SetBootValue( struct FtpClient* pClient, unsigned int val )
{
	pClient->m_eCOnnState |= _T(val);
}

void FtpClient_OnCmdRmd( FtpClient* pClient, const char* pDir )
{
	if( FileSystem_DeleteDir(&pClient->m_kContext,pDir) < 0 )
		FtpClient_Send( pClient, 550, "No such file or directory" );
	else
		FtpClient_Send( pClient, 250, "RMD command successful." );
}

void FtpClient_OnCmdSite( FtpClient* pClient, const char* pCmd )
{
	char* c;

	// copy command to clean buffer
	strcpy(buffer,pCmd);

	c = strtok(buffer," ");

	if(c)
	{
		int i = 0;
		unsigned int result = 0;

		for( i = 0; *c && i < 4; i++ )
			result = (result << 8)|tolower(*(c++));
	
		switch( result )
		{
#ifndef LINUX
			// SITE MNT <device> <file>
			case SITECMD_MNT:
			{
				char* mount_point;
				char* mount_file;

				// get mount point
				mount_point = strtok(NULL," ");
				if(!mount_point||!strlen(mount_point))
				{
					FtpClient_Send( pClient, 500, "SITE MNT: missing mount-point." );
					break;
				}

				// get mount source
				mount_file = strtok(NULL,"");
				if(!mount_file||!strlen(mount_file))
				{
					FtpClient_Send( pClient, 500, "SITE MNT: missing mount-file." );
					break;
				}

				FtpClient_OnSiteMount(pClient, mount_point,mount_file);
			}
			break;

			// SITE UMNT <device>
			case SITECMD_UMNT:
			{
				char* mount_point = strtok(NULL,"");

				if(mount_point)
					FtpClient_OnSiteUmount(pClient,mount_point);
				else
					FtpClient_Send( pClient, 500, "SITE UMNT: missing mount-point" );
			}
			break;

			// SITE SYNC <device>
			case SITECMD_SYNC:
			{
				char* devname = strtok(NULL,"");

				if(devname)
					FtpClient_OnSiteSync( pClient, devname );
				else
					FtpClient_Send( pClient, 500, "SITE SYNC: missing device-name." );
			}
			break;
#endif

			default:
			{
				FtpClient_Send( pClient, 500, "SITE command not supported" );
			}
			break;
		}
	}
	else
		FtpClient_Send( pClient, 500, "SITE command not understood" );

}

#ifndef LINUX
void FtpClient_OnSiteMount( struct FtpClient* pClient, const char* pMountPoint, const char* pMountFile )
{
	// mount filesystem

	if( FileSystem_MountDevice( &(pClient->m_kContext), pMountPoint, pMountFile ) < 0 )
		FtpClient_Send( pClient, 550, "SITE MNT failed." );
	else
		FtpClient_Send( pClient, 214, "SITE MNT succeeded." );
}

void FtpClient_OnSiteUmount( struct FtpClient* pClient, const char* pMountPoint )
{
	// unmount filesystem

	if( FileSystem_UnmountDevice( &(pClient->m_kContext), pMountPoint ) < 0 )
		FtpClient_Send( pClient, 550, "SITE UMNT failed." );
	else
		FtpClient_Send( pClient, 214, "SITE UMOUNT succeeded." );

}

void FtpClient_OnSiteSync( struct FtpClient* pClient, const char* pDeviceName )
{
	// sync data with filesystem

	if( FileSystem_SyncDevice( &(pClient->m_kContext), pDeviceName ) < 0 )
		FtpClient_Send( pClient, 550, "SITE SYNC failed." );
	else
		FtpClient_Send( pClient, 214, "SITE SYNC succeeded." );
}
#endif
