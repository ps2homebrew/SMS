#include "FSMan.h"
#include <kernel.h>
#define printf scr_printf

FSMan::FSMan(BOOL mc = true, BOOL cdvd = false, BOOL hdd = false) {
	int dirPos = 0;
	zeromem(currentDir, sizeof(currentDir));
	zeromem(rootDir, sizeof(t_aioDent)*4);
	zeromem(dirlisting, sizeof(t_aioDent)*256);
	mc0On = mc1On = cdvdOn = hddOn = false;
	strcpy(currentDir, "/");
	currentDevice = NULL;

	if (mc) {
		//load modules
		printf("\tLoading modules for memory card access...\n");
		SifLoadModule("rom0:SIO2MAN", 0, NULL);
		SifLoadModule("rom0:MCMAN", 0, NULL);
		SifLoadModule("rom0:MCSERV", 0, NULL);

		printf("\tInitialising memory card library...");
		if (mcInit(MC_TYPE_MC) == 0) {
			printf("SUCCESS\n");
			//test for mc0, mc1
			printf("\tChecking mc0... ");
			mc0 = new mcIO(0, 256);
			if (!(mc0->getstatus() && AIO_STATE_ERROR)) {
				printf("PRESENT\n");
				mc0On = true;
				strcpy(rootDir[dirPos].name, "mc0");
				rootDir[dirPos].size = 2048;
				rootDir[dirPos].attrib = AIO_ATTRIB_DIR;
				printf("rootdir[%d] = >%s<\n", dirPos, rootDir[dirPos].name);
				dirPos++;
			} else 
				printf("NOT PRESENT\n");
			
			printf("\tChecking mc1... ");
			mc1 = new mcIO(1, 256);
			if (mc1->getstatus() != AIO_STATE_ERROR) {
				printf("PRESENT\n");
				mc1On = true;
				strcpy(rootDir[dirPos].name, "mc1");
				rootDir[dirPos].size = 2048;
				rootDir[dirPos].attrib = AIO_ATTRIB_DIR;
				dirPos++;
			} else
				printf("NOT PRESENT\n");
		} else
				printf("FAILED\n");
	}

	if (cdvd) {
		//load modules
		printf("\tLoading modules for optical disc access...\n");
		//test for media
	}

	if (hdd) {
		//load modules
		printf("\tLoading modules for hard disk access...\n");
		//test for hdd

		//unload modules if hdd not present
	}
	memcpy(dirlisting, rootDir, sizeof(t_aioDent)*4);
	printf("dir #1: %s\n", dirlisting[0].name);
}