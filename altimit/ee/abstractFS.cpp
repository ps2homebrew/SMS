/*========================================================================
==				AbstractFS.cpp device IO functions.		           		==
==				(c) 2004 t0mb0la (tomhawcroft@comcast.net)				==
== Refer to the file LICENSE in the main folder for license information	==
========================================================================*/

#include "altimit.h"

static int fileMode =  FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP | FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH;
static int statMode =  FIO_CST_MODE | FIO_CST_ATTR | FIO_CST_SIZE | FIO_CST_CT | FIO_CST_AT | FIO_CST_MT | FIO_CST_PRVT;
int hddIOdevice = 0;
int oskModule(char *edittext, char *osktitle);	// on screen keyboard module

hddIO::hddIO()
{
 if(hddIOdevice>=MAX_PARTITIONS) devicestatus = -1;
 else
 {
	sprintf(device, "pfs%d:", hddIOdevice);
	hddIOdevice++;
	pfsmounted = false;
	devicestatus = 0;
	hddpathname = (char *)malloc(MAX_PATHNAME);
	hddnewname = (char *)malloc(MAX_PATHNAME);
 }
}

hddIO::~hddIO()
{
 dbgprintf("hddIO destructor\n");
 fileXioUmount(device);
 hddIOdevice = 0;
 pfsmounted = false;
 if (partitions) free(partitions);
 if (hddpathname) free(hddpathname);
 if (hddnewname) free(hddnewname);
}

int hddIO::open(const char *pathname, int mode)
{
 char *ptr;

 if(!pfsmounted) return -1;
 ptr = (char *)pathname; ptr++;
 while (*ptr != '/') ptr++;
 snprintf(hddpathname, MAX_PATHNAME-1, "%s%s", device, ptr);
 dbgprintf("hddIO open %s\n", hddpathname);
 return fileXioOpen(hddpathname, mode, fileMode);
}

int hddIO::lseek(int handle, int offset, int start)
{
 if(!pfsmounted) return -1;
 dbgprintf("hddIO lseek\n");
 return fileXioLseek(handle, offset, start);
}

int hddIO::read(int handle, unsigned char *buffer, int size)
{
 if(!pfsmounted) return -1;
 dbgprintf("hddIO read\n");
 return fileXioRead(handle, buffer, size);
}

int hddIO::write(int handle, unsigned char *buffer, int size)
{
 if(!pfsmounted) return -1;
 dbgprintf("hddIO write\n");
 return fileXioWrite(handle, buffer, size);
}

int hddIO::close(int handle)
{
 if(!pfsmounted) return -1;
 dbgprintf("hddIO close\n");
 return fileXioClose(handle);
}

int hddIO::remove(const char *pathname)
{
 char *ptr;

 if(!pfsmounted) return -1;
 ptr = (char *)pathname; ptr++;
 while (*ptr != '/') ptr++;
 snprintf(hddpathname, MAX_PATHNAME-1, "%s%s", device, ptr);
 dbgprintf("hddIO remove %s\n", hddpathname);
 return fileXioRemove(hddpathname);
}

int hddIO::rename(const char *pathname, const char *newpathname)
{
 char *ptr, *newptr;

 if(!pfsmounted) return -1;
 ptr = (char *)pathname; ptr++;
 while (*ptr != '/') ptr++;
 newptr = (char *)newpathname; newptr++;
 while (*newptr != '/') newptr++;
 snprintf(hddpathname, MAX_PATHNAME-1, "%s%s", device, ptr);
 snprintf(hddnewname, MAX_PATHNAME-1, "%s%s", device, newptr);
 dbgprintf("hddIO rename %s to %s\n", hddpathname, hddnewname);
 return fileXioRename(hddpathname, hddnewname);
}

int hddIO::mkdir(const char *pathname)
{
 char *ptr;

 if(!pfsmounted) return -1;
 ptr = (char *)pathname; ptr++;
 while (*ptr != '/') ptr++;
 snprintf(hddpathname, MAX_PATHNAME-1, "%s%s", device, ptr);
 dbgprintf("hddIO mkdir %s\n", hddpathname);
 return fileXioMkdir(hddpathname, fileMode);
}

int hddIO::rmdir(const char *pathname)
{
 char *ptr;

 if(!pfsmounted) return -1;
 ptr = (char *)pathname; ptr++;
 while (*ptr != '/') ptr++;
 snprintf(hddpathname, MAX_PATHNAME-1, "%s%s", device, ptr);
 dbgprintf("hddIO rmdir %s\n", hddpathname);
 return fileXioRmdir(hddpathname);
}

int hddIO::getdir(const char *pathname, altDentry contents[])
{
 iox_dirent_t hddcontent;
 int hfd, rv, hddcount, parptr;
 char *ptr;
 char filesystem[40];

 hddcount=0;
 dbgprintf("hddIO getdir %s\n", pathname);
 if(!strcmp(pathname,"/"))
 {
	fileXioUmount(device);
	pfsmounted = false;
 	strcpy(contents[0].filename,"..");
 	contents[0].mode = FIO_S_IFDIR;
 	contents[0].size = 0;
	if (partitions == NULL)
	{
		partitions = (t_hddFilesystem *) memalign(64, sizeof(t_hddFilesystem)*MAX_ENTRIES);
		parts = hddGetFilesystemList(partitions, MAX_ENTRIES);
	}
	for (hddcount=1;hddcount<(parts+1);hddcount++)
	{
		strcpy(contents[hddcount].filename, partitions[hddcount-1].filename);
		contents[hddcount].mode = FIO_S_IFDIR;
		contents[hddcount].size = 0;
	}
 }
 else 
 {
	parptr = 0;
	ptr = (char *)pathname; ptr++;
	while (*ptr != '/' && *ptr != '\0') filesystem[parptr++]=*ptr++;
	if (*ptr == '\0') return -1;
	filesystem[parptr]='\0';
	fileXioUmount(device);
	if ((rv = fileXioMount(device, filesystem, FIO_MT_RDWR)) < 0) return rv;
	strcpy(hddpathname, device);
	strcat(hddpathname, ptr);
	if ((hfd = fileXioDopen(hddpathname)) < 0) { fileXioUmount(device); return hfd; }
	pfsmounted = true;
	while ((rv = fileXioDread(hfd, &hddcontent)))
	{
		if (strcmp(hddcontent.name, "."))
		{
			strcpy(contents[hddcount].filename, hddcontent.name);
			if (hddcontent.stat.mode & FIO_S_IFDIR)
				{ contents[hddcount].mode = FIO_S_IFDIR; contents[hddcount].size = 0; }
			else
				{ contents[hddcount].mode = FIO_S_IFREG;
						contents[hddcount].size = hddcontent.stat.size; }
			hddcount++;
			if (hddcount > MAX_ENTRIES) break;
		}
	}
	fileXioDclose(hfd);
 }
 strcpy (contents[hddcount].filename, "\0");
 return hddcount;
}

int hddIO::getpath(const char *pathname, char *fullpath)
{
 char *ptr;

 if(!pfsmounted) return -1;
 ptr = (char *)pathname; ptr++;
 while (*ptr != '/') ptr++;
 snprintf(hddpathname, MAX_PATHNAME-1, "%s%s", device, ptr);
 dbgprintf("hddIO getpath %s\n", hddpathname);
 strcpy (fullpath, hddpathname);
 return 0;
}

int hddIO::getstat(const char *pathname, iox_stat_t *filestat)
{
 char *ptr;

 if(!pfsmounted) return -1;
 ptr = (char *)pathname; ptr++;
 while (*ptr != '/') ptr++;
 snprintf(hddpathname, MAX_PATHNAME-1, "%s%s", device, ptr);
 dbgprintf("hddIO getstat %s\n", hddpathname);
 return fileXioGetStat(hddpathname, filestat);
}

int hddIO::chstat(const char *pathname, iox_stat_t *filestat)
{
 char *ptr;

 if(!pfsmounted) return -1;
 ptr = (char *)pathname; ptr++;
 while (*ptr != '/') ptr++;
 snprintf(hddpathname, MAX_PATHNAME-1, "%s%s", device, ptr);
 dbgprintf("hddIO chstat %s\n", hddpathname);
 return fileXioChStat(hddpathname, filestat, statMode);
}

int hddIO::freespace()
{
 int totalfree, freezones, zonesize;

 if(!pfsmounted) return 0;
 freezones = fileXioDevctl(device, PFSCTL_GET_ZONE_FREE, NULL, 0, NULL, 0);
 zonesize = fileXioDevctl(device, PFSCTL_GET_ZONE_SIZE, NULL, 0, NULL, 0);
 totalfree = (freezones * zonesize) / 1024;
 return totalfree;
}

int hddIO::getstatus()
{
 dbgprintf("hddIO getstatus\n");
 return devicestatus;
}

httpIO::httpIO()
{
 strcpy(device, "http:");
 devicestatus = 0;
 httppathname = (char *)malloc(MAX_PATHNAME);
}

httpIO::~httpIO()
{
 dbgprintf("httpIO destructor\n");
 if (httppathname) free(httppathname);
}

int httpIO::open(const char *pathname, int mode)
{
 return -1;
}

int httpIO::lseek(int handle, int offset, int start)
{
 return -1;
}

int httpIO::read(int handle, unsigned char *buffer, int size)
{
 return -1;
}

int httpIO::write(int handle, unsigned char *buffer, int size)
{
 return -1;
}

int httpIO::close(int handle)
{
 return -1;
}

int httpIO::remove(const char *pathname)
{
 return -1;
}

int httpIO::rename(const char *pathname, const char *newpathname)
{
 return -1;
}

int httpIO::mkdir(const char *pathname)
{
 return -1;
}

int httpIO::rmdir(const char *pathname)
{
 return -1;
}

int httpIO::getdir(const char *pathname, altDentry contents[])
{
 char parse[2];
 int hfd, size, httpcount, ptr = 0;

 httpcount=0;
 parse[1] = '\0';
 oskModule(httppathname, "Enter URL");
 if ((hfd = fioOpen(httppathname, O_RDONLY)) < 0) { printf("failed!\n"); return hfd; }
 size = fioLseek(hfd, 0, SEEK_END);
 if (size)
 {
	strcpy(contents[ptr].filename,"");
	while(size)
	{
		fioRead(hfd, (unsigned char *)parse, 1);
		if (parse[0] == '\0') { }
		else
		{
			if (parse[0] == '\n') { ptr++; strcpy(contents[ptr].filename,""); }
			else strncat(contents[ptr].filename, parse, MAX_FILENAME);
		}
		if (ptr > MAX_ENTRIES) break;
		size--;
	}
	fioClose(hfd);
	ptr++;
	httpcount=ptr;
	}
 strcpy(contents[httpcount].filename, "\0");
 return httpcount;
}

int httpIO::getpath(const char *pathname, char *fullpath)
{
 return -1;
}

int httpIO::getstat(const char *pathname, iox_stat_t *filestat)
{
 return 0;
}

int httpIO::chstat(const char *pathname, iox_stat_t *filestat)
{
 return -1;
}

int httpIO::freespace()
{
 return 0;
}

int httpIO::getstatus()
{
 return devicestatus;
}

hostIO::hostIO()
{
 int fd;

 if ((fd = fioOpen("host:elflist.txt", O_RDONLY)) >= 0)
 { fioClose(fd); elflist = true; }
 else elflist = false;
 strcpy(device, "host:");
 devicestatus = 0;
 hostpathname = (char *)malloc(MAX_PATHNAME);
 hostnewname = (char *)malloc(MAX_PATHNAME);
}

hostIO::~hostIO()
{
 dbgprintf("hostIO destructor\n");
 if (hostpathname) free(hostpathname);
 if (hostnewname) free(hostnewname);
}

int hostIO::open(const char *pathname, int mode)
{
 pathname++;
 snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("hostIO open %s\n", hostpathname);
 return fileXioOpen(hostpathname, mode, fileMode);
}

int hostIO::lseek(int handle, int offset, int start)
{
 dbgprintf("hostIO lseek\n");
 return fileXioLseek(handle, offset, start);
}

int hostIO::read(int handle, unsigned char *buffer, int size)
{
 dbgprintf("hostIO read\n");
 return fileXioRead(handle, buffer, size);
}

int hostIO::write(int handle, unsigned char *buffer, int size)
{
 dbgprintf("hostIO write\n");
 return fileXioWrite(handle, buffer, size);
}

int hostIO::close(int handle)
{
 dbgprintf("hostIO close\n");
 return fileXioClose(handle);
}

int hostIO::remove(const char *pathname)
{
 pathname++;
 snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("hostIO remove %s\n", hostpathname);
 return fileXioRemove(hostpathname);
}

int hostIO::rename(const char *pathname, const char *newpathname)
{
 pathname++; newpathname++;
 snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 snprintf(hostnewname, MAX_PATHNAME-1, "%s%s", device, newpathname);
 dbgprintf("hostIO rename %s to %s\n", hostpathname, hostnewname);
 return fileXioRename(hostpathname, hostnewname);
}

int hostIO::mkdir(const char *pathname)
{
 pathname++;
 snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("hostIO mkdir %s\n", hostpathname);
 return fileXioMkdir(hostpathname, fileMode);
}

int hostIO::rmdir(const char *pathname)
{
 pathname++;
 snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("hostIO rmdir %s\n", hostpathname);
 return fileXioRmdir(hostpathname);
}

int hostIO::getdir(const char *pathname, altDentry contents[])
{
 fio_dirent_t hostcontent;
 int hfd, rv, size, itemsize, contentptr, hostcount = 0;
 char *elflisttxt, elflistchar;

 pathname++;
 snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("hostIO getdir %s\n", hostpathname);
 if (elflist)
 {
	strcpy(contents[0].filename, "..");
	contents[0].mode = FIO_S_IFDIR;
	contents[0].size = 0;
	hostcount++;
	if ((hfd = fioOpen("host:elflist.txt", O_RDONLY)) < 0) return 1;
	if ((size = fioLseek(hfd, 0, SEEK_END)) > 0)
	{
		elflisttxt = (char *)malloc(size);
		fioLseek(hfd, 0, SEEK_SET);
		fioRead(hfd, elflisttxt, size);
		fioClose(hfd);
		for(contentptr = 0; contentptr <= MAX_FILENAME; contentptr++)
			hostnewname[contentptr] = '\0';
		contentptr = 0;
		for (rv=0;rv<size;rv++)
		{
			elflistchar = elflisttxt[rv];
			if (elflistchar == 0x0a)
			{
				snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, hostnewname);
				if ((hfd = fioOpen(hostpathname, O_RDONLY)) >= 0)
				{
					itemsize = fioLseek(hfd, 0, SEEK_END);
					fioClose(hfd);
					contents[hostcount].mode = FIO_S_IFREG;
					contents[hostcount].size = itemsize;
					strcpy(contents[hostcount].filename, hostnewname);
					hostcount++;
				}
				if (hostcount > MAX_ENTRIES) rv = size;
				else
				{
					for(contentptr = 0; contentptr <= MAX_FILENAME; contentptr++)
						hostnewname[contentptr]='\0';
					contentptr = 0;
				}
			}
			else if (elflistchar != 0x0d)
			{
				hostnewname[contentptr]=elflistchar;
				contentptr++;
			}
		}
		if (contentptr > 0)
		{
			snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, hostnewname);
			if ((hfd = fioOpen(hostpathname, O_RDONLY)) >= 0)
			{
				itemsize = fioLseek(hfd, 0, SEEK_END);
				fioClose(hfd);
				contents[hostcount].mode = FIO_S_IFREG;
				contents[hostcount].size = itemsize;
				strcpy(contents[hostcount].filename, hostnewname);
				hostcount++;
			}
		}
		free(elflisttxt);
	}
	else { fioClose(hfd); return 1; }
	return hostcount;
 }
 if ((hfd = fioDopen(hostpathname)) < 0) return hfd;
 while ((rv = fioDread(hfd, &hostcontent)))
 {
	if (strcmp(hostcontent.name, "."))
	{
		strcpy(contents[hostcount].filename, hostcontent.name);
		if (hostcontent.stat.mode & FIO_SO_IFDIR)
			{ contents[hostcount].mode = FIO_S_IFDIR; contents[hostcount].size = 0; }
		else
			{ contents[hostcount].mode = FIO_S_IFREG;
						contents[hostcount].size = hostcontent.stat.size; }
		hostcount++;
		if (hostcount > MAX_ENTRIES) break;
	}
 }
 fioDclose(hfd);
 strcpy (contents[hostcount].filename, "\0");
 return hostcount;
}

int hostIO::getpath(const char *pathname, char *fullpath)
{
 pathname++;
 snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("hostIO getpath %s\n", hostpathname);
 strcpy(fullpath, hostpathname);
 return 0;
}

int hostIO::getstat(const char *pathname, iox_stat_t *filestat)
{
 pathname++;
 snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("hostIO getstat %s\n", hostpathname);
 return fileXioGetStat(hostpathname, filestat);
}

int hostIO::chstat(const char *pathname, iox_stat_t *filestat)
{
 pathname++;
 snprintf(hostpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("hostIO chstat %s\n", hostpathname);
 return fileXioChStat(hostpathname, filestat, statMode);
}

int hostIO::freespace()
{
 return 0;
}

int hostIO::getstatus()
{
 dbgprintf("hostIO getstatus\n");
 return devicestatus;
}

mcIO::mcIO(int mcport)
{
 int mctype, freespace, format, ret;

 this->mcport = mcport;
 this->changed = true;
 mccontents = NULL;
 mcGetInfo(mcport, 0, &mctype, &freespace, &format);
 mcSync(MC_WAIT, NULL, &ret);
 if(ret < -1) { devicestatus = -1; return; }
 mccontents = (mcTable *)memalign(64, sizeof(mcTable)*MAX_ENTRIES);
 sprintf(device, "mc%d:", mcport);
 devicestatus = 0;
 mcpathname = (char *)malloc(MAX_PATHNAME);
 mcnewname = (char *)malloc(MAX_PATHNAME);
}

mcIO::~mcIO()
{
 dbgprintf("mcIO destructor\n");
 if (mccontents) free(mccontents);
 if (mcpathname) free(mcpathname);
 if (mcnewname) free(mcnewname);
}

int mcIO::open(const char *pathname, int mode)
{
 snprintf(mcpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("mcIO open %s\n", mcpathname);
 return fileXioOpen(mcpathname, mode, fileMode);
}

int mcIO::lseek(int handle, int offset, int start)
{
 dbgprintf("mcIO lseek\n");
 return fileXioLseek(handle, offset, start);
}

int mcIO::read(int handle, unsigned char *buffer, int size)
{
 dbgprintf("mcIO read\n");
 return fileXioRead(handle, buffer, size);
}

int mcIO::write(int handle, unsigned char *buffer, int size)
{
 dbgprintf("mcIO write\n");
 if (!changed) changed = true;
 return fileXioWrite(handle, buffer, size);
}

int mcIO::close(int handle)
{
 dbgprintf("mcIO close\n");
 return fileXioClose(handle);
}

int mcIO::remove(const char *pathname)
{
 snprintf(mcpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("mcIO remove %s\n", mcpathname);
 if(!changed) changed = true;
 return fileXioRemove(mcpathname);
}

int mcIO::rename(const char *pathname, const char *newpathname)
{
// RENAME IS NOT SUPPORTED THROUGH MC RPC, FOR WHATEVER REASON.
// FORTUNATELY, WE CAN ASSUME A FILE OFF MC WILL FIT IN EE RAM
// ALLOWING US TO COPY THE FILE TO RAM, DELETE FROM MC AND
// WRITE BACK TO MC WITH THE NEW FILENAME. WILL NEED A LITTLE
// MORE WORK TO RENAME A FOLDER OF COURSE, SO I'LL JUST NOT
// ALLOW MC FOLDER RENAME FOR THE TIME BEING.
 int newfile, oldfile, mcfilesize;
 unsigned char *buffer;
 fio_stat_t mcfilestat;

 sprintf(mcpathname, "%s%s", device, pathname);
 sprintf(mcnewname, "%s%s", device, newpathname);
 dbgprintf("mcIO rename %s to %s\n", mcpathname, mcnewname);
 if ((fioGetstat(mcpathname, &mcfilestat)) < 0) return -1;
 if (mcfilestat.mode & FIO_SO_IFDIR) return -1;
 if ((oldfile = fileXioOpen(mcpathname, O_RDONLY, fileMode)) >= 0)
 {
	mcfilesize = fileXioLseek(oldfile, 0, SEEK_END);
	if (mcfilesize)
	{
		fileXioLseek(oldfile, 0, SEEK_SET);
		buffer = (unsigned char *)memalign(64,mcfilesize);
 		if (buffer)
		{
			fileXioRead(oldfile, buffer, mcfilesize);
			fileXioClose(oldfile);
			fileXioRemove(mcpathname);
			if ((newfile = fileXioOpen(mcnewname, O_WRONLY | O_TRUNC | O_CREAT, fileMode)) >= 0)
			{
				fileXioWrite(newfile, buffer, mcfilesize);
				fileXioClose(newfile);
				fioChstat(mcnewname, &mcfilestat, statMode);
			}
			free(buffer);
		}
		else return -2;
	}
	else
	{
		fileXioClose(oldfile);
		fileXioRemove(mcpathname);
		newfile = fileXioOpen(mcnewname, O_WRONLY | O_TRUNC | O_CREAT, fileMode);
		fileXioClose(newfile);
	}
	return 0;
 }
 else return -1;
}

int mcIO::mkdir(const char *pathname)
{
 snprintf(mcpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("mcIO mkdir %s\n", mcpathname);
 if (!changed) changed = true;
 return fileXioMkdir(mcpathname, fileMode);
}

int mcIO::rmdir(const char *pathname)
{
 snprintf(mcpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("mcIO rmdir %s\n", mcpathname);
 if (!changed) changed = true;
 return fileXioRmdir(mcpathname);
}

int mcIO::getdir(const char *pathname, altDentry contents[])
{
 int i, mcnumcontents, mccount = 0;

 dbgprintf("mcIO getdir %s\n", pathname);
 if (!strcmp(pathname, "/")) strcpy(mcpathname, "/*");
 else snprintf(mcpathname, MAX_PATHNAME-1, "%s/*", pathname);
 mcGetDir(mcport, 0, mcpathname, 0, MAX_ENTRIES, mccontents);
 mcSync(MC_WAIT, NULL, &mcnumcontents);
 if (mcnumcontents < -1) return mcnumcontents;
 if (!strcmp(pathname, "/"))
 {
	strcpy(contents[0].filename, "..");
	contents[0].mode = FIO_S_IFDIR;
	contents[0].size = 0;
	mccount++;
 }
 for (i = 0; (i < mcnumcontents) && (mccount < MAX_ENTRIES); i++)
 {
	if (strcmp((char *)mccontents[i].name, "."))
	{
		strcpy(contents[mccount].filename, (char *)mccontents[i].name);
		if (mccontents[i].attrFile & MC_ATTR_SUBDIR)
		{ contents[mccount].mode = FIO_S_IFDIR; contents[mccount].size = 0; }
		else
		{ contents[mccount].mode = FIO_S_IFREG; contents[mccount].size = mccontents[i].fileSizeByte; }
		mccount++;
	}
 }
 strcpy(contents[mccount].filename, "\0");
 return mccount;
}

int mcIO::getpath(const char *pathname, char *fullpath)
{
 snprintf(mcpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("mcIO getpath %s\n", mcpathname);
 strcpy(fullpath, mcpathname);
 return 0;
}

int mcIO::getstat(const char *pathname, iox_stat_t *filestat)
{
 snprintf(mcpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("mcIO getstat %s\n", mcpathname);
 return fioGetstat(mcpathname, (fio_stat_t*)filestat);
}

int mcIO::chstat(const char *pathname, iox_stat_t *filestat)
{
 snprintf(mcpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("mcIO chstat %s\n", mcpathname);
 return fioChstat(mcpathname, (fio_stat_t*)filestat, statMode);
}

int mcIO::freespace()
{
 int mctype, mcformat, ret;

 if (!changed) return mcfree;
 dbgprintf("mcIO freespace\n");
 mcGetInfo(mcport, 0, &mctype, &mcfree, &mcformat);
 mcSync(MC_WAIT, NULL, &ret);

 if (ret < -1) return ret;
 else 
 {
	changed = false;
	return mcfree;
 }
} 

int mcIO::getstatus()
{
 dbgprintf("mcIO getstatus\n");
 return devicestatus;
}

cdfsIO::cdfsIO()
{
 cdfscontents = (struct TocEntry *) memalign(64, sizeof(struct TocEntry) * MAX_ENTRIES);
 CDVD_FlushCache();
 strcpy(device, "cdfs:");
 devicestatus = 0;
 cdpathname = (char *)malloc(MAX_PATHNAME);
}

cdfsIO::~cdfsIO()
{
 dbgprintf("cdfsIO destructor\n");
 if (cdfscontents) free(cdfscontents);
 if (cdpathname) free(cdpathname);
}

int cdfsIO::open(const char *pathname, int mode)
{
 snprintf(cdpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("cdfsIO open %s\n", cdpathname);
 return fileXioOpen(cdpathname, mode, fileMode);
}

int cdfsIO::lseek(int handle, int offset, int start)
{
 dbgprintf("cdfsIO lseek\n");
 return fileXioLseek(handle, offset, start);
}

int cdfsIO::read(int handle, unsigned char *buffer, int size)
{
 dbgprintf("cdfsIO read\n");
 return fileXioRead(handle, buffer, size);
}

int cdfsIO::write(int handle, unsigned char *buffer, int size)
{
 return -1;
}

int cdfsIO::close(int handle)
{
 dbgprintf("cdfsIO close\n");
 return fileXioClose(handle);
}

int cdfsIO::remove(const char *pathname)
{
 return -1;
}

int cdfsIO::rename(const char *pathname, const char *newpathname)
{
 return -1;
}

int cdfsIO::mkdir(const char *pathname)
{
 return -1;
}

int cdfsIO::rmdir(const char *pathname)
{
 return -1;
}

int cdfsIO::getdir(const char *pathname, altDentry contents[])
{
 int i, cdfsnumcontents, cdfscount = 0;

 dbgprintf("cdfsIO getdir %s\n", pathname);
 cdfsnumcontents = CDVD_GetDir(pathname, NULL, CDVD_GET_FILES_AND_DIRS, cdfscontents, MAX_ENTRIES, NULL);
 if (!strcmp(pathname, "/"))
 {
	strcpy(contents[0].filename, "..");
	contents[0].mode = FIO_S_IFDIR;
	contents[0].size = 0;
	cdfscount++;
 }
 for (i = 0; (i < cdfsnumcontents) && (cdfscount < MAX_ENTRIES); i++)
 {
	if (strcmp((char *)cdfscontents[i].filename, "."))
	{
		strcpy(contents[cdfscount].filename, (char *)cdfscontents[i].filename);
		if (cdfscontents[i].fileProperties & 0x02)
		{ contents[cdfscount].mode = FIO_S_IFDIR; contents[cdfscount].size = 0; }
		else
		{ contents[cdfscount].mode = FIO_S_IFREG; contents[cdfscount].size = cdfscontents[i].fileSize; }
		cdfscount++;
	}
 }
 strcpy(contents[cdfscount].filename, "\0");
 return cdfscount;
}

int cdfsIO::getpath(const char *pathname, char *fullpath)
{
 snprintf(cdpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("cdfsIO getpath %s\n", cdpathname);
 strcpy(fullpath, cdpathname);
 return 0;
}

int cdfsIO::getstat(const char *pathname, iox_stat_t *filestat)
{
 snprintf(cdpathname, MAX_PATHNAME-1, "%s%s", device, pathname);
 dbgprintf("cdfsIO getstat %s\n", cdpathname);
 return fileXioGetStat(cdpathname, filestat);
}

int cdfsIO::chstat(const char *pathname, iox_stat_t *filestat)
{
 return -1;
}

int cdfsIO::freespace()
{
 return 0;
}

int cdfsIO::getstatus()
{
 dbgprintf("cdfsIO getstatus\n");
 return devicestatus;
}

