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

// present in FtpClient.c
extern char* itoa(char* in, int val);

// these offsets depend on the layout of both ioman & iomanX ... ps2netfs does
// something similar, so if it breaks, we all go down.
#define DEVINFO_IOMAN_OFFSET 0x0c
#define DEVINFO_IOMANX_OFFSET 0

#define DEVINFOARRAY(d,ofs) ((iop_device_t **)((d)->text_start + (d)->text_size + (d)->data_size + (ofs)))

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
		// directory listing from I/O device
		case FS_IODEVICE:
		{
			if( !pContext->m_kFile.device )
				break;

			// attempt to open device directory

			pContext->m_kFile.mode = O_DIROPEN;
			if( pContext->m_kFile.device->ops->dopen( &(pContext->m_kFile), pDir ) >=0 )
				return 0;
		}
		break;

		// either device-list or unit-list (depending on if pContext->m_kFile.device was set)
		case FS_DEVLIST:
		{
			pContext->m_kFile.unit = 0; // we use this for enumeration
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

			// read data from I/O device

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

			// write data to device

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
			if( !pContext->m_kFile.device || !(pContext->m_kFile.mode & O_DIROPEN) )
				break;

			if( (pContext->m_kFile.device->type & 0xf0000000) != IOP_DT_FSEXT )
			{
				// legacy device

				io_dirent_t ent;

				if( pContext->m_kFile.device->ops->dread( &(pContext->m_kFile), &ent ) > 0 )
				{
					strcpy(pDirectory->m_Name,ent.name);
					pDirectory->m_iSize = ent.stat.size;
					pDirectory->m_eType = FIO_SO_ISDIR(ent.stat.mode) ? FT_DIRECTORY : FT_FILE;
					return 0;					
				}
			}
			else
			{
				// new device

				iox_dirent_t ent;

				if( pContext->m_kFile.device->ops->dread( &(pContext->m_kFile), &ent ) > 0 )
				{
					strcpy(pDirectory->m_Name,ent.name);
					pDirectory->m_iSize = ent.stat.size;
					pDirectory->m_eType = ent.stat.mode&FIO_S_IFDIR ? FT_DIRECTORY : FT_FILE;
					return 0;					
				}
			}
		}
		break;

		case FS_DEVLIST:
		{
			if( pContext->m_kFile.device )
			{
				// evaluating units below a device

				while( pContext->m_kFile.unit < 16 ) // find a better value, and make a define
				{
					iox_stat_t stat;
					int ret;
					int unit = pContext->m_kFile.unit;

					memset(&stat,0,sizeof(stat));

					// get status from root directory of device
					ret = pContext->m_kFile.device->ops->getstat( &(pContext->m_kFile), "/", (io_stat_t*)&stat );

					// dummy devices does not set mode properly, so we can filter them out easily
					if( (ret >= 0) && !stat.mode  )
						ret = -1;

					// increase to next unit
					pContext->m_kFile.unit++;

					// currently we stop evaluating devices if one was not found (so if mc 1 exists and not mc 0, it will not show)
					if( ret < 0 )
						return -1;

					itoa(pDirectory->m_Name,unit);
					pDirectory->m_iSize = 0;
					pDirectory->m_eType = FT_DIRECTORY;
					return 0;
				}
			}
			else
			{
				// evaluating devices

				smod_mod_info_t* pkModule;
				iop_device_t** ppkDevices;
				int num_devices;
				int dev_offset;

				// get module

				num_devices = FS_IOMANX_DEVICES;
				dev_offset = DEVINFO_IOMANX_OFFSET;
				if( NULL == (pkModule = FileSystem_GetModule(IOPMGR_IOMANX_IDENT)) )
				{
					dev_offset = DEVINFO_IOMAN_OFFSET;
					num_devices = FS_IOMAN_DEVICES;
					if( NULL == (pkModule = FileSystem_GetModule(IOPMGR_IOMAN_IDENT)) )
						return -1;
				}

				// scan filesystem devices

				ppkDevices = DEVINFOARRAY(pkModule,dev_offset);
				while( pContext->m_kFile.unit < num_devices )
				{
					int unit = pContext->m_kFile.unit;
					pContext->m_kFile.unit++;

					if( !ppkDevices[unit] )
						continue;

					if( !(ppkDevices[unit]->type & IOP_DT_FS) )
						continue;

					strcpy(pDirectory->m_Name,ppkDevices[unit]->name);
					pDirectory->m_iSize = 0;
					pDirectory->m_eType = FT_DIRECTORY;

					return 0;
				}
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

void FileSystem_BuildPath( char* pResult, const char* pOriginal, const char* pAdd )
{
	pResult[0] = '\0';

	// absolute path?
	if(pAdd[0] == '/')
	{
		pOriginal = pAdd;
		pAdd = NULL;
	}

#ifdef LINUX
	strcat(pResult,"./");
#endif

	if( pOriginal )
		strcat(pResult,pOriginal);

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
				while( --t >= pContext->m_Path)
				{
					*(t+1) = 0;

					if( *t == '/')
						break;
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

#ifndef LINUX
const char* FileSystem_ClassifyPath( FSContext* pContext, char* pPath )
{
	char* entry;
	char* t;

	// make sure the I/O has been closed before

	FileSystem_Close(pContext);

	// must start as a valid path

	if( pPath[0] != '/' )
		return NULL;

	// begin parsing

	entry = strtok(pPath+1,"/");

	// begin parsing

	pContext->m_eType = FS_DEVLIST;

	// is this a pure device list? then just return a pointer
	if(!entry || !strlen(entry))
		return pPath;

	// attempt to find device
	if( NULL == (pContext->m_kFile.device = FileSystem_ScanDevice(IOPMGR_IOMANX_IDENT,FS_IOMANX_DEVICES,entry)) )
	{
		if( NULL == (pContext->m_kFile.device = FileSystem_ScanDevice(IOPMGR_IOMAN_IDENT,FS_IOMAN_DEVICES,entry)) )
			return NULL;
	}

	// extract unit number if present
	entry = strtok(NULL,"/");

	// no entry present? then we do a unit listing of the current device
	if(!entry || !strlen(entry))
		return pPath;

	t = entry;
	while(*t)
	{
		// enforcing unit nubering
		if(!isnum(*t))
			return NULL;

		t++;
	}

	pContext->m_kFile.unit = strtol(entry, NULL, 0);

	// extract local path

	pContext->m_eType = FS_IODEVICE;
	entry = strtok(NULL,"");
	strcpy(buffer,"/");

	if(entry)
		strcat(buffer,entry);	// even if buffer was passed in as an argument, this will yield a valid result

	return buffer;
}

smod_mod_info_t* FileSystem_GetModule( const char* pDevice )
{
	smod_mod_info_t* pkModule = NULL;

	// scan module list

	pkModule = (smod_mod_info_t *)0x800;
	while( NULL != pkModule )
	{
		if (!strcmp(pkModule->name, pDevice))
			break;

		pkModule = pkModule->next;
	}

	return pkModule;
}

iop_device_t* FileSystem_ScanDevice( const char* pDevice, int iNumDevices, const char* pPath )
{
	smod_mod_info_t* pkModule;
  iop_device_t **ppkDevices;
	int i;
	int offset;

	// get module

	if( NULL == (pkModule = FileSystem_GetModule(pDevice)) )
		return NULL;

	// determine offset
	
	if( !strcmp(IOPMGR_IOMANX_IDENT,pkModule->name) )
		offset = DEVINFO_IOMANX_OFFSET;
	else if( !strcmp(IOPMGR_IOMAN_IDENT,pkModule->name) )
		offset = DEVINFO_IOMAN_OFFSET;
	else
		return NULL; // unknown device, we cannot determine the offset here...

	// get device info array
	ppkDevices = DEVINFOARRAY(pkModule,offset);

	// scan array
	for( i = 0; i < iNumDevices; i++ )
	{
		if( (NULL != ppkDevices[i]) ) // note, last compare is to avoid a bug when mounting partitions right now that I have to track down
		{
			if( (ppkDevices[i]->type & IOP_DT_FS))
			{
				if( !strncmp(pPath,ppkDevices[i]->name,strlen(ppkDevices[i]->name)) )
					return ppkDevices[i];
			}
		}
	}

	return NULL;
}

#endif
