#include <sifrpc.h>
#include <loadfile.h>
#include <kernel.h>
#define IPCONF_MAX_LEN  (3*16)

char if_conf[IPCONF_MAX_LEN];
int if_conf_len;

char ip[16] __attribute__((aligned(16))) = "192.168.0.10";//jenova "192.168.1.120"; //jules 192.168.128.7
char netmask[16] __attribute__((aligned(16))) = "255.255.255.0";
char gw[16] __attribute__((aligned(16))) = "192.168.0.1";

int main() {
	int i;
	
	memset(if_conf, 0x00, IPCONF_MAX_LEN);
        i = 0;
        strncpy(&if_conf[i], ip, 15);
        i += strlen(ip) + 1;
        
        strncpy(&if_conf[i], netmask, 15);
        i += strlen(netmask) + 1;
        
        strncpy(&if_conf[i], gw, 15);
        i += strlen(gw) + 1;
        
        if_conf_len = i;
	
	SifInitRpc(0);
	SifLoadModule("rom0:SIO2MAN",0,NULL);
	SifLoadModule("rom0:MCMAN",0,NULL);
	SifLoadModule("mc0:\\BOOT\\PS2FTP\\PS2IP.IRX",0,NULL);
	SifLoadModule("mc0:\\BOOT\\PS2FTP\\PS2SMAP.IRX", if_conf_len, &if_conf[0]);
	SifLoadModule("mc0:\\BOOT\\PS2FTP\\PS2FTP.IRX", 0, 0);
	//while(1);
}