/*
 * FileSystem.c - FTP server filesystem layer
 *
 * Copyright (C) 2004 Jesper Svennevid
 *
 * Device scan is based on the one from ps2netfs.
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 */

#include "FileSystem.h"

#ifndef LINUX
#include "irx_imports.h"
#define assert(x)
#else
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

/*

	plan for future unified path system

	/mc/0/	- memory card, unit 0
	/mc/1/	- memory card, unit 1
	/cdfs/	- cd file system
	/hd/0/	- hdd, partition 0
	/hd/1/	- hdd, partition 1 
	/usb/0/	- usb mass storage, unit 0

*/

// buffer used for concating filenames internally
static char buffer[512];

void FileSystem_Create( FSContext* pContext )
{
	strcpy(pContext->m_Path,"/");

#ifndef LINUX
	pContext->m_iType = FS_INVALID;
#endif

	pContext->m_iFile = -1;

#ifdef LINUX
	pContext->m_pDir = NULL;
#else
	pContext->m_iDir = -1;
#endif
}

void FileSystem_Destroy( FSContext* pContext )
{
	FileSystem_Close(pContext);
}

int FileSystem_OpenFile( FSContext* pContext, const char* pFile, FileMode eMode )
{
	int mode;

	FileSystem_Close(pContext);

	FileSystem_BuildPath( buffer, pContext->m_Path, pFile );

	switch( eMode )
	{
		case FM_READ: mode = O_RDONLY; break;
		case FM_CREATE: mode = O_CREAT|O_TRUNC|O_WRONLY; break;

		case FM_APPEND: // TODO: implement
		default:
			return -1;
	}

#ifdef LINUX
	if( (pContext->m_iFile = open(buffer,mode,S_IRWXU|S_IRGRP)) < 0 )
		return -1;
#else
	pContext->m_iType = FileSystem_ClassifyPath( buffer );
	switch( pContext->m_iType )
	{
		case FS_IOMAN: pContext->m_iFile = io_open( buffer, mode ); break;
		case FS_IOMANX: pContext->m_iFile = open( buffer, mode ); break;
		default: break;
	}

	if( pContext->m_iFile < 0 )
		return -1;

	return 0;
#endif

	return 0;
}

int FileSystem_OpenDir( FSContext* pContext, const char* pDir )
{
	FileSystem_Close(pContext);

	FileSystem_BuildPath( pContext->m_List, pContext->m_Path, pDir );

#ifdef LINUX

	// unsafe, sure
	pContext->m_pDir = opendir(pContext->m_List);
	return pContext->m_pDir ? 0 : -1;

#else

	pContext->m_iType = FileSystem_ClassifyPath( pContext->m_List );
	switch( pContext->m_iType )
	{
		case FS_IOMAN: pContext->m_iDir = io_dopen( pContext->m_List, 0 ); break;
		case FS_IOMANX: pContext->m_iDir = dopen( pContext->m_List ); break;
		case FS_DEVLIST: pContext->m_iDir = 0; break;
		default: break;
	}

	if( pContext->m_iDir < 0 )
		return -1;

	return 0;

#endif
}

int FileSystem_ReadFile( FSContext* pContext, char* pBuffer, int iSize )
{
	if( pContext->m_iFile < 0 )
		return -1;

#ifdef LINUX
	return read(pContext->m_iFile,pBuffer,iSize);
#else
	switch( pContext->m_iType )
	{
		case FS_IOMAN: return io_read( pContext->m_iFile, pBuffer, iSize ); break;
		case FS_IOMANX: return read( pContext->m_iFile, pBuffer, iSize ); break;
		default: return -1;
	}
#endif
}

int FileSystem_WriteFile( FSContext* pContext, const char* pBuffer, int iSize )
{
	if( pContext->m_iFile < 0 )
		return -1;

#ifdef LINUX
	return write(pContext->m_iFile,pBuffer,iSize);
#else
	switch( pContext->m_iType )
	{
		case FS_IOMAN: return io_write( pContext->m_iFile, (char*)pBuffer, iSize ); // bad declaration on that function...
		case FS_IOMANX: return write( pContext->m_iFile, (char*)pBuffer, iSize );
		default: return -1;
	}
#endif
}

int FileSystem_ReadDir( FSContext* pContext, FSDirectory* pDirectory )
{
#ifdef LINUX
	if( NULL == pContext->m_pDir )
		return -1;

	struct dirent* ent = readdir(pContext->m_pDir);

	if( !ent )
		return -1;

	// TODO: FileSystem_GetFileInfo()

	strcpy(pDirectory->m_Name,ent->d_name);

	FileSystem_GetFileInfo( pDirectory, pContext->m_List );

	return 0;
#else

	switch( pContext->m_iType )
	{
		case FS_IOMAN:
		{
      io_dirent_t ent;
      int r = io_dread(pContext->m_iDir,&ent);

      if (r > 0)
      {
        pDirectory->m_iSize = ent.stat.size;
				pDirectory->m_eType = FIO_SO_ISDIR(ent.stat.mode) ? FT_DIRECTORY : FT_FILE;
        strcpy(pDirectory->m_Name,ent.name);
				return 0;
      }
		}
		break;

		case FS_IOMANX:
		{
      iox_dirent_t ent;
      int r = dread(pContext->m_iDir,&ent);

      if (r > 0)
      {
        pDirectory->m_iSize = ent.stat.size;
				pDirectory->m_eType = ent.stat.mode&FIO_S_IFDIR ? FT_DIRECTORY : FT_FILE;
        strcpy(pDirectory->m_Name,ent.name);
				return 0;
      }
		}
		break;

		case FS_DEVLIST:
		{
			// TODO: make a proper scan here

      pDirectory->m_iSize = 0;
			pDirectory->m_eType = FT_DIRECTORY;

			switch( pContext->m_iDir )
			{
				case 0: strcpy(pDirectory->m_Name,"mc0"); pContext->m_iDir++; return 0;
				case 1: strcpy(pDirectory->m_Name,"mc1"); pContext->m_iDir++; return 0;
			}
		}
		break;

		default:
		break;
	}

	return -1;
#endif
}

int FileSystem_GetFileInfo( FSDirectory* pDirectory, const char* pPath )
{
#ifdef LINUX
	struct stat s;

	strcpy( buffer, pPath );
	strcat( buffer, pDirectory->m_Name );

	if( stat( buffer, &s ) < 0 )
		return -1;

	pDirectory->m_eType = S_ISDIR(s.st_mode) ? FT_DIRECTORY : FT_FILE;
	pDirectory->m_iSize = s.st_size;

	return 0;
#else
	return -1;
#endif
}

int FileSystem_DeleteFile( FSContext* pContext, const char* pFile )
{
	int type;

	FileSystem_BuildPath( buffer, pContext->m_Path, pFile );

#ifdef LINUX
	return -1;
#else

	type = FileSystem_ClassifyPath( buffer );
	switch( type )
	{
		case FS_IOMAN: return io_remove( buffer );
		case FS_IOMANX: return remove( buffer );

		default: return -1;
	}

#endif
}

int FileSystem_CreateDir( FSContext* pContext, const char* pDir )
{
	int type;

	FileSystem_BuildPath( buffer, pContext->m_Path, pDir );

#ifdef LINUX
	return -1;
#else

	type = FileSystem_ClassifyPath( buffer );
	switch( type )
	{
		case FS_IOMAN: return io_mkdir( buffer );
		case FS_IOMANX: return mkdir( buffer );

		default: return -1;
	}

#endif
}

int FileSystem_DeleteDir( FSContext* pContext, const char* pDir )
{
	int type;

	FileSystem_BuildPath( buffer, pContext->m_Path, pDir );

#ifdef LINUX
	return -1;
#else

	type = FileSystem_ClassifyPath( buffer );
	switch( type )
	{
		case FS_IOMAN: return io_rmdir( buffer );
		case FS_IOMANX: return rmdir( buffer );

		default: return -1;
	}

#endif
}


void FileSystem_Close( FSContext* pContext )
{
#ifdef LINUX
	if( pContext->m_iFile >= 0 )
	{
		close( pContext->m_iFile );
		pContext->m_iFile = -1;
	}

	if( NULL != pContext->m_pDir )
	{
		closedir( pContext->m_pDir );
		pContext->m_pDir = NULL;
	}
#else
	if( pContext->m_iDir >= 0 )
	{
		switch( pContext->m_iType )
		{
			case FS_IOMAN: io_dclose( pContext->m_iDir ); break;
			case FS_IOMANX: dclose( pContext->m_iDir ); break;
			default: break;
		}
		pContext->m_iDir = -1;
	}
	else if( pContext->m_iFile >= 0 )
	{
		switch( pContext->m_iType )
		{
			case FS_IOMAN: io_close( pContext->m_iFile ); break;
			case FS_IOMANX: close( pContext->m_iFile ); break;
			default: break;
		}
		pContext->m_iFile = -1;
	}

	pContext->m_iType = FS_INVALID;
#endif
}

int FileSystem_ClassifyPath( const char* pPath )
{
	// extract device

	// TODO: fix this one
	if( !strcmp(pPath,":") )
		return FS_DEVLIST;

	if( FileSystem_ScanDevice(IOPMGR_IOMAN_IDENT,FS_IOMAN_DEVICES,pPath) >= 0 )
		return FS_IOMAN;

	if( FileSystem_ScanDevice(IOPMGR_IOMANX_IDENT,FS_IOMANX_DEVICES,pPath) >= 0 )
		return FS_IOMANX;

	return FS_INVALID;
}

// TODO: these two following path functions are quite terrible and should be rewritten, but
// I want to see if my idea is working first

void FileSystem_BuildPath( char* pResult, const char* pOriginal, const char* pAdd )
{
	const char* pO;
	char* pR;

	pResult[0] = '\0';

#ifdef LINUX
	strcat(pResult,".");
#endif

	// absolute path?
	if(pAdd[0] == '/')
	{
		pOriginal = pAdd;
		pAdd = NULL;
	}

	pO = pOriginal;
	pR = pResult;

	pO++;
	while((*pO != '/') && (*pO))
		*(pR++) = *(pO++);
	*pR++ = ':';
	*pR = '\0';

	if( *pO )
	{
		pO++;
		strcat(pResult,pO);
	}

	if( pAdd )
	{
		if( (pResult[strlen(pResult)-1] != '/') && (pResult[strlen(pResult)-1] != ':') )
			strcat( pResult, "/" );

		strcat(pResult,pAdd);
	}
}

int FileSystem_ChangeDir( FSContext* pContext, const char* pPath )
{
	strcpy(buffer,pPath);

	if( pPath[0] == '/' )
	{
		strcpy(pContext->m_Path,pPath);
		if( (pContext->m_Path[strlen(pContext->m_Path)-1] != '/') )
			strcat(pContext->m_Path,"/");
	}
	else
	{
		char* entry = strtok(buffer,"/");

		while(entry && strlen(entry)>0)
		{
			if(!strcmp(entry,".."))
			{
				char* t = strrchr(pContext->m_Path,'/');
				while( --t > pContext->m_Path)
				{
					if( *t == '/')
						break;

					*t = 0;
				}
			}
			else
			{
				strcat( pContext->m_Path, entry );
				strcat( pContext->m_Path, "/" );
			}

			entry = strtok(NULL,"/");
		}
	}

	return 0;
}

int FileSystem_ScanDevice( const char* pDevice, int iNumDevices, const char* pPath )
{
	smod_mod_info_t* pkModule = NULL;
  iop_device_t **pkDevInfoTable;
	int i;

	// scan module list

	pkModule = (smod_mod_info_t *)0x800;
	while( NULL != pkModule )
	{
		if (!strcmp(pkModule->name, pDevice))
			break;
		pkModule = pkModule->next;
	}

	if(!pkModule)
		return -1;

	// get device info array
	pkDevInfoTable = (iop_device_t **)(pkModule->text_start + pkModule->text_size + pkModule->data_size + 0x0c);

	// scan array
	for( i = 0; i < iNumDevices; i++ )
	{
		if( NULL != pkDevInfoTable[i] )
		{
			if( (pkDevInfoTable[i]->type & IOP_DT_FS))
			{
				printf("check: '%s'\n",pkDevInfoTable[i]->name);

				if( !strncmp(pPath,pkDevInfoTable[i]->name,strlen(pkDevInfoTable[i]->name)) )
				{
					printf("match: '%s' '%s' '%d'\n",pPath,pkDevInfoTable[i]->name,iNumDevices);
					return 0;
				}
			}
		}
	}

	return -1;
}
