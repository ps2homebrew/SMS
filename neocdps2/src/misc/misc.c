#ifndef	MISC_C
#define MISC_C

#include <tamtypes.h>
#include <kernel.h> 
#include <stdio.h>
#include <fileio.h>
#include <stdlib.h> 
#include <stdio.h>
#include <malloc.h>
#include <string.h> 
#include "../defines.h" 
#include <sifrpc.h>
#include <loadfile.h> 

#define	TIMER_IRX 0x8000012A

static SifRpcClientData_t client __attribute__((aligned(64)));
unsigned char rpcBuffer[128] __attribute__((aligned(64))); 

uint32 start;

void swab( const void* src1, const void* src2, int isize)
{
	char*	ptr1;
	char*	ptr2;
	char	tmp;
	int	ic1;
	
	ptr1 = (char*)src1;
	ptr2 = (char*)src2;
	for ( ic1=0 ; ic1<isize ; ic1+=2){
		tmp = ptr1[ic1+0];
		ptr2[ic1+0] = ptr1[ic1+1];
		ptr2[ic1+1] = tmp;
	}
}

char mtoupper(char c) {
  if (c>='a' && c<='z')
    return c+'A'-'a';
  return c;
} 

// untested
uint32 PS2_InitTicks()
{
    int ret;
    int* sec;
    int* usec; 

    ret = SifLoadModule("host0:./timer.irx",0, NULL);
    if (ret < 0)
    {
       	printf("sifLoadModule %s failed: %d\n", "host0:TIMER.IRX", ret);
       	return -1;
    }

    //short delay - give the module enough time to register itself
    ret = 0x01000000;
    while(ret--) asm("nop\nnop\nnop\nnop");

    printf ("Trying to bind Timer...\n");
    while (1) {
        ret = SifBindRpc( &client, TIMER_IRX, 0);
        if ( ret  < 0)  {
           break;
        }
        if (client.server != 0) break;

        // short delay ???
      	ret = 0x10000;
    	while(ret--);
    }
    if (ret < 0 ) {
        printf("SifBindRpc Timer failed: %d !!!!\n", ret);
        return -1;
    }
    SifCallRpc(&client,0,0,(void*)(&rpcBuffer[0]),0,(void*)(&rpcBuffer[0]),128,0,0);
    sec = (int*) (&rpcBuffer[0]);
    usec = (int*) (&rpcBuffer[4]);
    start = (*sec)*1000+ (*usec)/1000; // msec
    return start;
} 

// untested
uint32 PS2_StartTicks()
{
	int* sec;
   	int* usec; 
	// reset start value
	printf("tick-start\n");
	SifCallRpc(&client,0,0,(void*)(&rpcBuffer[0]),0,(void*)(&rpcBuffer[0]),128,0,0);
	sec = (int*) (&rpcBuffer[0]);
   	usec = (int*) (&rpcBuffer[4]);
   	start = (*sec)*1000+ (*usec)/1000; // msec
   	printf("tick-end\n");
   	return start;
} 

// untested
uint32 PS2_GetTicks()
{
	uint32 now;
	int* sec;
   	int* usec; 
	
	printf("tick-start\n");
	SifCallRpc(&client,0,0,(void*)(&rpcBuffer[0]),0,(void*)(&rpcBuffer[0]),128,0,0);
	sec = (int*) (&rpcBuffer[0]);
   	usec = (int*) (&rpcBuffer[4]);
   	now = (*sec)*1000+ (*usec)/1000; // msec
   	printf("tick-end\n");
	return(now-start); 
} 

// untested
void PS2_Delay (uint32 ms)
{
	uint32 then, now, elapsed;
	then = PS2_GetTicks();
	do {
		errno = 0;
		/* Calculate the time interval left (in case of interrupt) */
		now = PS2_GetTicks();
		elapsed = (now-then);
		then = now;
		if ( elapsed >= ms ) {
			break;
		}
		ms -= elapsed;
	} while (1);
} 

#endif
