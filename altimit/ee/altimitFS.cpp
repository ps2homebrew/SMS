/*========================================================================
==				AltimitFS.cpp handles mounting devices.		           	==
==				(c) 2004 t0mb0la (tomhawcroft@comcast.net)				==
== Refer to the file LICENSE in the main folder for license information	==
========================================================================*/

#include "altimit.h"

static SifRpcClientData_t cdvdCd __attribute__((aligned(64)));
static char cdRpcBuffer[256] __attribute__((aligned(16)));

int cdInitialised = 0;

void cdExit(void)
{
    cdInitialised = 0;
}

int cdInit(int mode)
{
	int i=0,len=0,ret=0;
	char *pkt;

	cdvdCd.server = NULL;
	while(!cdvdCd.server)
	{
		if ((ret = SifBindRpc(&cdvdCd, CD_INIT_BIND_RPC, 0)) < 0) return -1;
		if (!cdvdCd.server) nopdelay();
	}
	pkt = cdRpcBuffer; 
	PUSHDATA( int, pkt, mode, i);
	pkt += i; len += i;
	if ((ret = SifCallRpc(&cdvdCd, 0, 0, cdRpcBuffer, len, NULL, 0, 0, 0)) < 0) return -1;
	cdInitialised = 1;
	return 0;
}

altimitFS::altimitFS(int ps2hd, int iphd, int pchd, int mc, int cd)
{
	int deventry = 0;

	dbgprintf("Before mallocs\n");
	currentdev = (char *) malloc(MAX_DEVNAME);
	currentdir = (char *) malloc(MAX_PATHNAME);
	dircontents = (altDentry *) malloc(sizeof(altDentry)*MAX_ENTRIES);
	altroot = (altDentry *) malloc(sizeof(altDentry)*MAX_DEVICES);

	dbgprintf("Before memsets\n");
	memset(currentdev, '\0', MAX_DEVNAME);
	memset(currentdir, '\0', MAX_PATHNAME);
	memset(dircontents, '\0', sizeof(altDentry)*MAX_ENTRIES);
	memset(altroot, '\0', sizeof(altDentry)*MAX_DEVICES);

	activeHDD = activeHTTP = activeHOST = activeMC0 = activeMC1 = activeCDFS = false;
	strcpy(currentdir, "/");
	activedevice = NULL;

	if (iphd)
	{
		dbgprintf("Before new httpIO\n");
		http = new httpIO();
		if (http->getstatus() < 0) dbgprintf("http: not found\n");
		else
		{
			activeHTTP = true;
			strcpy(altroot[deventry].filename, "http");
			altroot[deventry].mode = FIO_S_IFDIR;
			altroot[deventry].size = 0;
			altroot[deventry].fsdevice = http;
			deventry++;
		}
	}

	if (pchd)
	{
		dbgprintf("Before new hostIO\n");
		host = new hostIO();
		if (host->getstatus() < 0) dbgprintf("host: not found\n");
		else
		{
			activeHOST = true;
			strcpy(altroot[deventry].filename, "host");
			altroot[deventry].mode = FIO_S_IFDIR;
			altroot[deventry].size = 0;
			altroot[deventry].fsdevice = host;
			deventry++;
		}
	}

	if (mc)
	{
		dbgprintf("Before new mcIO\n");
		mc0 = new mcIO(0);
		if (mc0->getstatus() < 0) dbgprintf("mc0: not found\n");
		else
		{
			activeMC0 = true;
			strcpy(altroot[deventry].filename, "mc0");
			altroot[deventry].mode = FIO_S_IFDIR;
			altroot[deventry].size = 0;
			altroot[deventry].fsdevice = mc0;
			deventry++;
		}
		mc1 = new mcIO(1);
		if (mc1->getstatus() < 0) dbgprintf("mc1: not found\n");
		else
		{
			activeMC1 = true;
			strcpy(altroot[deventry].filename, "mc1");
			altroot[deventry].mode = FIO_S_IFDIR;
			altroot[deventry].size = 0;
			altroot[deventry].fsdevice = mc1;
			deventry++;
		}
	}

	if (ps2hd)
	{
		dbgprintf("Before new hddIO\n");
		pfs = new hddIO();
		if (pfs->getstatus() < 0) dbgprintf("hdd0: not found\n");
		else
		{
			activeHDD = true;
			strcpy(altroot[deventry].filename, "hdd");
			altroot[deventry].mode = FIO_S_IFDIR;
			altroot[deventry].size = 0;
			altroot[deventry].fsdevice = pfs;
			deventry++;
		}
	}

	if (cd)
	{
		dbgprintf("Before new cdfsIO\n");
		cdfs = new cdfsIO();
		if (cdfs->getstatus() < 0) dbgprintf("cdfs: not found\n");
		else
		{
			activeCDFS = true;
			strcpy(altroot[deventry].filename, "cdfs");
			altroot[deventry].mode = FIO_S_IFDIR;
			altroot[deventry].size = 0;
			altroot[deventry].fsdevice = cdfs;
			deventry++;
		}
	}

	dbgprintf("Before final memcpy\n");
	memcpy(dircontents, altroot, sizeof(altDentry)*MAX_DEVICES);
	free(altroot);
}

altimitFS::~altimitFS()
{
 dbgprintf("altimitFS destructor\n");
 delete cdfs;
 delete pfs;
 delete mc1;
 delete mc0;
 delete host;
 delete http;
 if (dircontents) free(dircontents);
 if (currentdir) free(currentdir);
 if (currentdev) free(currentdev);
}
