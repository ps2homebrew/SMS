#ifndef __FTPCLIENT_H__
#define __FTPCLIENT_H__

/*
 * FtpClient.h - FTP client declaration
 *
 * Copyright (C) 2004 Jesper Svennevid
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 */

#include "FileSystem.h"

struct FtpServer;

typedef enum
{
	DATAMODE_IDLE,
	DATAMODE_READ,
	DATAMODE_WRITE,
} DataMode;

typedef enum
{
	CONNSTATE_IDLE,
	CONNSTATE_LISTEN,
	CONNSTATE_CONNECT,
	CONNSTATE_RUNNING,
} ConnState;

typedef enum
{
	DATAACTION_NONE,
	DATAACTION_LIST,
	DATAACTION_NLST,
	DATAACTION_RETR,
	DATAACTION_STOR,
} DataAction;

enum
{
	FTPCMD_USER,
	FTPCMD_PASS,
	FTPCMD_PASV,
	FTPCMD_PORT,
	FTPCMD_QUIT,
	FTPCMD_SYST,
	FTPCMD_LIST,
	FTPCMD_NLST,
	FTPCMD_RETR,
	FTPCMD_STOR,
	FTPCMD_TYPE,
	FTPCMD_PWD,
	FTPCMD_CWD,
	FTPCMD_CDUP,
	FTPCMD_DELE,
	FTPCMD_MKD,
	FTPCMD_RMD,
	FTPCMD_SITE,
};

enum
{
	SITECMD_MOUNT,
	SITECMD_UMOUNT,
	SITECMD_SYNC,
	SITECMD_HELP,
};

typedef struct FtpCommand
{
	int m_iCommand;
	const char* m_pName;
} FtpCommand;
    

typedef struct FtpClientContainer
{
	struct FtpClient* m_pClient;
	struct FtpClientContainer* m_pPrev;
	struct FtpClientContainer* m_pNext;
} FtpClientContainer;

#define CRC32_ComputeChecksum(b,s) (bb_Status()?CRC32_CheckData(CRC32_Key,b,s):0)
extern int CRC32_CheckData( unsigned int k, char* b, int s );

typedef struct FtpClient
{
	struct FtpServer* m_pServer;	// owning server

	int m_iControlSocket;


	int m_iDataSocket;
	unsigned int m_uiDataBufferSize;
	int m_iRemoteAddress[4]; // yeah, I know, but FTP is retarded
	ConnState m_eCOnnState;
	int m_iRemotePort;
	DataMode m_eDataMode;
	DataAction m_eDataAction;
	ConnState m_eConnState;
	int m_uiDataOffset;

	FtpClientContainer m_kContainer;

	int m_iCommandOffset;
	char m_CommandBuffer[512];

	FSContext m_kContext;
} FtpClient;

void FtpClient_Create( struct FtpClient* pClient, struct FtpServer* pServer, int iControlSocket );
void FtpClient_Destroy( struct FtpClient* pClient );
void FtpClient_Send( struct FtpClient* pClient, int iReturnCode, const char* pString );
void FtpClient_OnConnect( struct FtpClient* pClient );
void FtpClient_OnCommand( struct FtpClient* pClient, const char* pString );

void FtpClient_OnCmdQuit( struct FtpClient* pClient );
void FtpClient_OnCmdUser( struct FtpClient* pClient, const char* pUser );
void FtpClient_OnCmdPass( struct FtpClient* pClient, const char* pPass );
void FtpClient_OnCmdPasv( struct FtpClient* pClient );
void FtpClient_OnCmdPort( struct FtpClient* pClient, int* ip, int port );
void FtpClient_OnCmdSyst( struct FtpClient* pClient );
void FtpClient_OnCmdList( struct FtpClient* pClient, const char* pPath, int iNamesOnly );
void FtpClient_OnCmdType( struct FtpClient* pClient, const char* pType );
void FtpClient_OnCmdRetr( struct FtpClient* pClient, const char* pFile );
void FtpClient_OnCmdStor( struct FtpClient* pClient, const char* pFile );
void FtpClient_OnCmdPwd( struct FtpClient* pClient );
void FtpClient_OnCmdCwd( struct FtpClient* pClient, const char* pPath );
void FtpClient_OnCmdDele( struct FtpClient* pClient, const char* pFile );
void FtpClient_OnCmdMkd( struct FtpClient* pClient, const char* pDir );
void FtpClient_OnCmdRmd( struct FtpClient* pClient, const char* pDir );
void FtpClient_OnCmdSite( struct FtpClient* pClient, const char* pCmd );
#ifdef VALID_IRX
static void FtpClient_SetBootValue( struct FtpClient* pClient, unsigned int val ) { pClient->m_eCOnnState = _T(val); }
#endif

void FtpClient_OnSiteMount( struct FtpClient* pClient, const char* pMountPoint, const char* pMountFile );
void FtpClient_OnSiteUmount( struct FtpClient* pClient, const char* pMountPoint );
void FtpClient_OnSiteSync( struct FtpClient* pClient, const char* pDeviceName );

void FtpClient_OnDataConnect( struct FtpClient* pClient,  int* ip, int port );
void FtpClient_OnDataConnected( struct FtpClient* pClient );
void FtpClient_OnDataRead( struct FtpClient* pClient );
void FtpClient_OnDataWrite( struct FtpClient* pClient );
void FtpClient_OnDataComplete( struct FtpClient* pClient, int iReturnCode, const char* pMessage );
void FtpClient_OnDataFailed( struct FtpClient* pClient, const char* pMessage  );
void FtpClient_OnDataCleanup( struct FtpClient* pClient );

#endif
