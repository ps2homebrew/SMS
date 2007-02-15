//---------------------------------------------------------------------------
//File name:    scache.c
//---------------------------------------------------------------------------
/*
 * scache.c - USB Mass storage driver for PS2
 *
 * (C) 2004, Marek Olejnik (ole00@post.cz)
 * (C) 2004  Hermes (support for sector sizes from 512 to 4096 bytes)
 *
 * Sector cache
 *
 * See the file LICENSE included with this distribution for licensing terms.
 */
//---------------------------------------------------------------------------

#include <tamtypes.h>
#include <stdio.h>
#include <sysmem.h>
#define malloc(a)	AllocSysMemory(0,(a), NULL)
#define free(a)		FreeSysMemory((a))

#include "mass_stor.h"

//---------------------------------------------------------------------------
// Modified Hermes
//always read 4096 bytes from sector (the rest bytes is stored in the cache)
#define READ_SECTOR_4096(a, b)	mass_stor_readSector4096((a), (b))

#include "mass_debug.h"

//number of cache slots (1 slot = memory block of 4096 bytes)
#define CACHE_SIZE 32

typedef struct _cache_record
{
	unsigned int sector;
	int tax;
} cache_record;


int sectorSize;
int indexLimit;
unsigned char* sectorBuf = NULL;		//sector content - the cache buffer
cache_record rec[CACHE_SIZE];	//cache info record

//statistical infos
unsigned int cacheAccess;
unsigned int cacheHits;

//---------------------------------------------------------------------------
void initRecords()
{
	int i;

	for (i = 0; i < CACHE_SIZE; i++)
	{
		rec[i].sector = 0xFFFFFFF0;
		rec[i].tax = 0;
	}
}

//---------------------------------------------------------------------------
/* search cache records for the sector number stored in cache
  returns cache record (slot) number
 */
int getSlot(unsigned int sector) {
	int i;

	for (i = 0; i < CACHE_SIZE; i++) {
		if (sector >= rec[i].sector && sector < (rec[i].sector + indexLimit)) {
			return i;
		}
	}
	return -1;
}


//---------------------------------------------------------------------------
/* search cache records for the sector number stored in cache */
int inline getIndexRead(unsigned int sector) {
	int i;
	int index =-1;

	for (i = 0; i < CACHE_SIZE; i++) {
		if (sector >= rec[i].sector && sector < (rec[i].sector + indexLimit)) {
			if (rec[i].tax < 0) rec[i].tax = 0;
			rec[i].tax +=2;
			index = i;
		}
		rec[i].tax--;     //apply tax penalty
	}
	if (index < 0)
		return index;
	else
		return ((index * indexLimit) + (sector - rec[index].sector));
}

//---------------------------------------------------------------------------
/* select the best record where to store new sector */
int inline getIndexWrite(unsigned int sector) {
	int i;
	int minTax = 0x0FFFFFFF;
	int index = 0;

	for (i = 0; i < CACHE_SIZE; i++) {
		if (rec[i].tax < minTax) {
			index = i;
			minTax = rec[i].tax;
		}
	}

	rec[index].tax +=2;
	rec[index].sector = sector;

	return index * indexLimit;
}


//---------------------------------------------------------------------------
int scache_readSector(unsigned int sector, void** buf) {
	int index; //index is given in single sectors not octal sectors
	int ret;
	unsigned int alignedSector;

	cacheAccess ++;
	index = getIndexRead(sector);

	if (index >= 0) { //sector found in cache
		cacheHits ++;
		*buf = sectorBuf + (index * sectorSize);

		return sectorSize;
	}

	//compute alignedSector - to prevent storage of duplicit sectors in slots
	alignedSector = (sector/indexLimit)*indexLimit;
	index = getIndexWrite(alignedSector);
	ret = READ_SECTOR_4096(alignedSector, sectorBuf + (index * sectorSize));

	if (ret < 0) {
		return ret;
	}
	*buf = sectorBuf + (index * sectorSize) + ((sector%indexLimit) * sectorSize);

	return sectorSize;
}

int scache_init(char * param, int sectSize)
{
	//added by Hermes
	sectorSize = sectSize;
	indexLimit = 4096/sectorSize; //number of sectors per 1 cache slot

	if (sectorBuf == NULL) {
		XPRINTF("scache init! \n");
		XPRINTF("sectorSize: 0x%x\n",sectorSize);
		sectorBuf = (unsigned char*) malloc(4096 * CACHE_SIZE ); //allocate 4096 bytes per 1 cache record
		if (sectorBuf == NULL) {
			XPRINTF("Sector cache: can't alloate memory of size:%d \n", 4096 * CACHE_SIZE);
			return -1;
		}
		XPRINTF("Sector cache: alocated memory at:%p of size:%d \n", sectorBuf, 4096 * CACHE_SIZE);
	} else {
		XPRINTF("scache flush! \n");
	}
	cacheAccess = 0;
	cacheHits = 0;
	initRecords();
	return(1);
}

//---------------------------------------------------------------------------
void scache_getStat(unsigned int* access, unsigned int* hits) {
	*access = cacheAccess;
        *hits = cacheHits;
}

//---------------------------------------------------------------------------
void scache_close() //dlanor: added for disconnection events (flush impossible)
{
	if(sectorBuf != NULL)
	{
		free(sectorBuf);
		sectorBuf = NULL;
	}
}
//---------------------------------------------------------------------------
//End of file:  scache.c
//---------------------------------------------------------------------------
