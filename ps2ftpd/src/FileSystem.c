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
#define isnum(c) ((c) >= '0' && (c) <= '9')
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
	/cd/0/	- cd file system
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
	pContext->m_eType = FS_INVALID;
	memset( &(pContext->m_kFile), 0, sizeof(pContext->m_kFile) );
#else
	pContext->m_iFile = -1;
	pContext->m_pDir = NULL;
#endif
}

void FileSystem_Destroy( FSContext* pContext )
{
	FileSystem_Close(pContext);
}

int FileSystem_OpenFile( FSContext* pContext, const char* pFile, FileMode eMode )
{
	int flags;

	FileSystem_Close(pContext);
	FileSystem_BuildPath( buffer, pContext->m_Path, pFile );

	switch( eMode )
	{
		case FM_READ: flags = O_RDONLY; break;
		case FM_CREATE: flags = O_CREAT|O_TRUNC|O_WRONLY; break;

		case FM_APPEND: // TODO: implement
		default:
			return -1;
	}

#ifdef LINUX
	if( (pContext->m_iFile = open(buffer,flags,S_IRWXU|S_IRGRP)) < 0 )
		return -1;

	return 0;
#else
	if( NULL == (pFile = FileSystem_ClassifyPath( pContext, buffer )) )
		return -1;

	switch( pContext->m_eType )
	{
		case FS_IODEVICE:
		{
			if( !pContext->m_kFile.device )
				break;

			pContext->m_kFile.mode = flags;
			if( pContext->m_kFile.device->ops->open( &(pContext->m_kFile), pFile, flags, 0 ) >= 0 )
				return 0;

		}
		break;

		default:
			return -1;
	}

	pContext->m_eType = FS_INVALID;
	return -1;
#endif
}

int FileSystem_OpenDir( FSContext* pContext, const char* pDir )
{
	FileSystem_Close(pContext);
	FileSystem_BuildPath( buffer, pContext->m_Path, pDir );

#ifdef LINUX

	strcpy(pContext->m_List,buffer);

	// unsafe, sure
	pContext->m_pDir = opendir(pContext->m_List);
	return pContext->m_pDir ? 0 : -1;

#else

	if( NULL == (pDir = FileSystem_ClassifyPath( pContext, buffer )) )
		return -1;

	switch( pContext->m_eType )
	{
		case FS_IODEVICE:
		{
			if( !pContext->m_kFile.device )
				break;

			pContext->m_kFile.mode = O_DIROPEN;

			if( pContext->m_kFile.device->ops->dopen( &(pContext->m_kFile), pDir ) >=0 )
				return 0;
		}
		break;

		case FS_DEVLIST:
		{
			pContext->m_kFile.mode = 0; // we use this for an internal counter
			return 0;
		}
		break;
		default: break;
	}

	pContext->m_eType = FS_INVALID;
	return -1;
#endif
}

int FileSystem_ReadFile( FSContext* pContext, char* pBuffer, int iSize )
{
#ifdef LINUX
	if( pContext->m_iFile < 0 )
		return -1;

	return read(pContext->m_iFile,pBuffer,iSize);
#else

	switch( pContext->m_eType )
	{
		case FS_IODEVICE:
		{
			if( !pContext->m_kFile.device || !(pContext->m_kFile.mode & O_RDONLY) )
				break;

			return pContext->m_kFile.device->ops->read( &(pContext->m_kFile), pBuffer, iSize );
		}
		break;

		default:
	}

	return -1;
#endif
}

int FileSystem_WriteFile( FSContext* pContext, const char* pBuffer, int iSize )
{
#ifdef LINUX
	if( pContext->m_iFile < 0 )
		return -1;

	return write(pContext->m_iFile,pBuffer,iSize);
#else

	switch( pContext->m_eType )
	{
		case FS_IODEVICE:
		{
			if( !pContext->m_kFile.device || !(pContext->m_kFile.mode & O_WRONLY) )
				break;

			return pContext->m_kFile.device->ops->write( &(pContext->m_kFile), (char*)pBuffer, iSize );
		}
		break;

		default:
	}

	return -1;
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

	strcpy(pDirectory->m_Name,ent->d_name);
	FileSystem_GetFileInfo( pDirectory, pContext->m_List );

	return 0;
#else
	switch( pContext->m_eType )
	{
		case FS_IODEVICE:
		{
			io_dirent_t ent;

			if( !pContext->m_kFile.device || !(pContext->m_kFile.mode & O_DIROPEN) )
				break;

			if( pContext->m_kFile.device->ops->dread( &(pContext->m_kFile), &ent ) > 0 )
			{
        strcpy(pDirectory->m_Name,ent.name);
        pDirectory->m_iSize = ent.stat.size;

				if( (pContext->m_kFile.device->type & 0xf0000000) != IOP_DT_FSEXT )
					pDirectory->m_eType = FIO_SO_ISDIR(ent.stat.mode) ? FT_DIRECTORY : FT_FILE;
				else
					pDirectory->m_eType = ent.stat.mode&FIO_S_IFDIR ? FT_DIRECTORY : FT_FILE;

				return 0;					
			}
		}
		break;

		case FS_DEVLIST:
		{
			const char* name = NULL;

			// TODO: make a proper scan here

      pDirectory->m_iSize = 0;
			pDirectory->m_eType = FT_DIRECTORY;

			switch( pContext->m_kFile.mode )
			{
				case 0: name = "mc0"; break;
				case 1: name = "mc1"; break;
				case 2: name = "host"; break;
			}

			if( name )
			{
				strcpy(pDirectory->m_Name,name);
				pContext->m_kFile.mode++;
				return 0;
			}
		}
		break;

		default:
			return -1;
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
	FileSystem_BuildPath( buffer, pContext->m_Path, pFile );

#ifdef LINUX
	return -1;
#else

	if( NULL == (pFile = FileSystem_ClassifyPath( pContext, buffer )) )
		return -1;

	switch( pContext->m_eType )
	{
		case FS_IODEVICE:
		{
			if( !pContext->m_kFile.device )
				break;

			return pContext->m_kFile.device->ops->remove( (&pContext->m_kFile), pFile );
		}
		break;

		default: return -1;
	}

	return -1;
#endif
}

int FileSystem_CreateDir( FSContext* pContext, const char* pDir )
{
	FileSystem_BuildPath( buffer, pContext->m_Path, pDir );

#ifdef LINUX
	return -1;
#else
	if( NULL == (pDir = FileSystem_ClassifyPath( pContext, buffer )) )
		return -1;

	switch( pContext->m_eType )
	{
		case FS_IODEVICE:
		{
			if( !pContext->m_kFile.device )
				break;
			return pContext->m_kFile.device->ops->mkdir( &(pContext->m_kFile), pDir, 0 );
		}
		break;

		default: return -1;
	}

	return -1;
#endif
}

int FileSystem_DeleteDir( FSContext* pContext, const char* pDir )
{
	FileSystem_BuildPath( buffer, pContext->m_Path, pDir );

#ifdef LINUX
	return -1;
#else
	if( NULL == (pDir = FileSystem_ClassifyPath( pContext, buffer )) )
		return -1;

	switch( pContext->m_eType )
	{
		case FS_IODEVICE:
		{
			if( !pContext->m_kFile.device )
				break;
			return pContext->m_kFile.device->ops->rmdir( &(pContext->m_kFile), pDir );
		}
		break;

		default: return -1;
	}

	return -1;
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
	switch( pContext->m_eType )
	{
		case FS_IODEVICE:
		{
			if( !pContext->m_kFile.device )
				break;

			if( pContext->m_kFile.mode & O_DIROPEN )
				pContext->m_kFile.device->ops->dclose( &pContext->m_kFile );
			else
				pContext->m_kFile.device->ops->close( &pContext->m_kFile );
		}
		break;

		default: break;
	}

	pContext->m_eType = FS_INVALID;
	memset( &(pContext->m_kFile), 0, sizeof(pContext->m_kFile) );
#endif
}

const char* FileSystem_ClassifyPath( FSContext* pContext, const char* pPath )
{
	char* start;
	char* end;

	// TODO: fix this one
	if( !strcmp(pPath,":") )
	{
		pContext->m_eType = FS_DEVLIST;
		return pPath;
	}

	// extract unit number from path

	if( !(start = end = index( pPath, ':' )) || (start == pPath) )
		return NULL;

	if( isnum(*(start-1)) )
	{
		do { start--; } while( (start > pPath) && isnum(*(start-1)) );

		pContext->m_kFile.unit = strtol( start, NULL, 0 );
	}

	// scan ioman devices

	if( NULL != (pContext->m_kFile.device = FileSystem_ScanDevice(IOPMGR_IOMAN_IDENT,FS_IOMAN_DEVICES,pPath)) )
	{
		pContext->m_eType = FS_IODEVICE;
		return end+1;
	}

	// scan iomanX devices

	if( NULL != (pContext->m_kFile.device = FileSystem_ScanDevice(IOPMGR_IOMANX_IDENT,FS_IOMANX_DEVICES,pPath)) )
	{
		pContext->m_eType = FS_IODEVICE;
		return end+1;
	}

	return NULL;
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

iop_device_t* FileSystem_ScanDevice( const char* pDevice, int iNumDevices, const char* pPath )
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
		return NULL;

	// get device info array
	pkDevInfoTable = (iop_device_t **)(pkModule->text_start + pkModule->text_size + pkModule->data_size + 0x0c);

	// scan array
	for( i = 0; i < iNumDevices; i++ )
	{
		if( NULL != pkDevInfoTable[i] )
		{
			if( (pkDevInfoTable[i]->type & IOP_DT_FS))
			{
				if( !strncmp(pPath,pkDevInfoTable[i]->name,strlen(pkDevInfoTable[i]->name)) )
					return pkDevInfoTable[i];
			}
		}
	}

	return NULL;
}
