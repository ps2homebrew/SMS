/*
 *  cdvd.h - LibCDVD
 *  Copyright (C) 2002, A.Lee & Nicholas Van Veen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _CDVD_H
#define _CDVD_H

#include <tamtypes.h>

// This header contains the common definitions for libcdvd
// that are used by both IOP and EE sides

#define	CDVD_IRX	0xB001337
#define CDVD_FINDFILE	0x01
#define	CDVD_GETDIR 	0x02
#define CDVD_STOP	0x04
#define CDVD_TRAYREQ	0x05
#define CDVD_DISKREADY	0x06
#define CDVD_FLUSHCACHE	0x07
#define CDVD_GETSIZE	0x08


struct TocEntry
{
	u32		fileLBA;
	u32		fileSize;
	u8		fileProperties;
	u8		padding1[3];
	char	filename[128+1];
	u8		padding2[3];
} __attribute__((packed));


enum CDVD_getMode {
	CDVD_GET_FILES_ONLY = 1,
	CDVD_GET_DIRS_ONLY = 2,
	CDVD_GET_FILES_AND_DIRS = 3
};

// Macros for TrayReq
#define CdTrayOpen		0
#define CdTrayClose		1
#define CdTrayCheck		2

// Macros for DiskReady
#define CdComplete		0x02
#define CdNotReady		0x06
#define CdBlock			0x00
#define CdNonBlock		0x01		

#endif // _CDVD_H
