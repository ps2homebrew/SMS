#ifndef ps2ftp_h
#define ps2ftp_h
/* define DEBUG if you want to print status msgs via the 'perror' command
   ie if running under naplink or ps2link*/
//#define DEBUG

/* 'perror' definition for printf */
#ifdef DEBUG
#define perror(FMT, __VA_LIST__...) printf(FMT, __VA_LIST__...)
#else
#define perror(FMT, __VA_LIST__...) {}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <kernel.h>
#include <ps2ip.h>
#include <sysclib.h>
#define NEED_SNPRINTF_ONLY
#include "snprintf.h"
//hton[l,s] fix
#define ntohl(x) htonl(x)
#define ntohs(x) htons(x)
#define BUFFER_SIZE 1024
#define EOF (-1)
#define bool char
#define true 1
#define false 0

// Valid bits in memcard file attributes (mctable.AttrFile)
#define MC_ATTR_READABLE        0x0001
#define MC_ATTR_WRITEABLE       0x0002
#define MC_ATTR_EXECUTABLE      0x0004
#define MC_ATTR_PROTECTED       0x0008
#define MC_ATTR_SUBDIR          0x0020
#define MC_ATTR_CLOSED          0x0080
#define MC_ATTR_PDAEXEC         0x0800
#define MC_ATTR_PSX             0x1000

/* FTP Command tokens */
enum command_types {USER,PASS,ACCT,CWD,CDUP,SMNT,REIN,QUIT,
      PORT,PASV,TYPE,STRU,MODE,RETR,STOR,STOU,APPE,ALLO,REST,RNFR,RNTO,
      ABOR,DELE,RMD,MKD,PWD,LIST,NLST,SITE,SIZE,SYST,STAT,HELP,NOOP,EOFC,ERROR} ;

/* struct used to build a table mapping command character sequences to
   their command token listed above */
struct command_list {
  char *command ;
  enum command_types command_code ;
} ;

/* Lookup table for FTP Commands */
struct command_list command_lookup[] = {
  "USER", USER,  "PASS", PASS,  "ACCT", ACCT,  "CWD",  CWD,   "CDUP", CDUP,
  "SMNT", SMNT,  "REIN", REIN,  "QUIT", QUIT,  "PORT", PORT,  "PASV", PASV,
  "TYPE", TYPE,  "STRU", STRU,  "MODE", MODE,  "RETR", RETR,  "STOR", STOR,
  "STOU", STOU,  "APPE", APPE,  "ALLO", ALLO,  "REST", REST,  "RNFR", RNFR,
  "RNTO", RNTO,  "ABOR", ABOR,  "DELE", DELE,  "RMD",  RMD,   "MKD",  MKD,
  "XMKD", MKD,   "XRMD", RMD,   "PWD",  PWD,   "XPWD", PWD,   "LIST", LIST,
  "NLST", NLST,  "SITE", SITE,  "SIZE", SIZE,  "SYST", SYST,  "STAT", STAT,
  "HELP", HELP,  "NOOP", NOOP,  "EOFC", EOFC
} ;

enum command_modes {LOGIN, PASSWORD, COMMANDS, EXIT} ;

typedef struct
{
    struct
    {
        unsigned char unknown1;
        unsigned char sec;      // Entry creation date/time (second)
        unsigned char min;      // Entry creation date/time (minute)
        unsigned char hour;     // Entry creation date/time (hour)
        unsigned char day;      // Entry creation date/time (day)
        unsigned char month;    // Entry creation date/time (month)
        unsigned short year;    // Entry creation date/time (year)
    } _create;

    struct
    {
        unsigned char unknown2;
        unsigned char sec;      // Entry modification date/time (second)
        unsigned char min;      // Entry modification date/time (minute)
        unsigned char hour;     // Entry modification date/time (hour)
        unsigned char day;      // Entry modification date/time (day)
        unsigned char month;    // Entry modification date/time (month)
        unsigned short year;    // Entry modification date/time (year)
    } _modify;

    unsigned fileSizeByte;      // File size (bytes). For a directory entry: 0
    unsigned short attrFile;    // File attribute
    unsigned short unknown3;
    unsigned unknown4[2];
    unsigned char name[32];         //Entry name
} mcTable __attribute__((aligned (64)));

#define ARRAY_ENTRIES 64
static mcTable mcDir[ARRAY_ENTRIES] __attribute__((aligned(64)));

int mcInit() {
	int ret;

	LoadStartModule("rom0:SIO2MAN", 0, NULL, &ret);
  	perror("LoadStartModule for rom0:SIO2MAN returned %d\n", ret);
	LoadStartModule("rom0:MCMAN", 0, NULL, &ret);
	perror("LoadStartModule for rom0:MCMAN returned %d\n", ret);
	if (ret != 1) {
		perror("ERROR: LoadStartModule for rom0:MCMAN returned %d\n", ret);
		return 0;
	}		
	//we shouldn't need the server since we're actually on the IOP
		/*else {
		LoadStartModule("rom0:MCSERV", 0, NULL, &ret);
		if (ret !=1 ) {
			perror("ERROR: LoadStartModule for rom0:MCSERV returned %d\n", ret);
			return 0;
		}
	}*/
	return 1;
}

bool isDir(char *dir) {
	char theDir[1024];
	mcTable dirTable[ARRAY_ENTRIES] __attribute__((aligned(64)));
	int numEntries;
	
	sprintf(theDir, "mc0:%s", dir);
	numEntries = mc_getdir (0, 0, theDir, 0, 2 , dirTable);
	if (numEntries > 0)
		if (dirTable[0].attrFile & MC_ATTR_SUBDIR)
			return true;
	return false;
}
#endif