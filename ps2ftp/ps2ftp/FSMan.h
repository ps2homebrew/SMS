#ifndef FSMan_h
#define FSMan_h

#include "aio/aio.h"
#include <sifrpc.h>
#include <libmc.h>
#include <loadfile.h>
#include <string.h>

#define zeromem(a,b) memset(a, '\0', b)
typedef int BOOL;

class FSMan {
public:
	abstractIO *currentDevice, *mc0, *mc1, *cdvd, *hdd;
	BOOL mc0On, mc1On, cdvdOn, hddOn;
	t_aioDent dirlisting[256];
	char currentDir[1024];
	
	FSMan(BOOL mc = true, BOOL cdvd = false, BOOL hdd = false);
	
private:
	t_aioDent rootDir[4];

};

#endif