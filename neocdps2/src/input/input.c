/**************************************
****   INPUT.C  -  Input devices   ****
**************************************/

/*-- Include Files ---------------------------------------------------------*/
#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <tamtypes.h>
#include <sifrpc.h>
#include <sifcmd.h> 
#include <loadfile.h>

#include "../neocd.h"

#include "libpad.h"

#define ROM_PADMAN

#if defined(ROM_PADMAN) && defined(NEW_PADMAN)
#error Only one of ROM_PADMAN & NEW_PADMAN should be defined!
#endif

#if !defined(ROM_PADMAN) && !defined(NEW_PADMAN)
#error ROM_PADMAN or NEW_PADMAN must be defined!
#endif


/*
 * Global var's
 */
// contains the pad's current state
static char padBuf_1[256] __attribute__((aligned(64))) __attribute__ ((section (".bss")));
static char padBuf_2[256] __attribute__((aligned(64))) __attribute__ ((section (".bss")));

static char actAlign[6];
static int actuators;

static struct padButtonStatus buttons;
static u32 paddata;
int pad_nb;

/*--------------------------------------------------------------------------*/
#define P1UP    0x00000001
#define P1DOWN  0x00000002
#define P1LEFT  0x00000004
#define P1RIGHT 0x00000008
#define P1A     0x00000010
#define P1B     0x00000020
#define P1C     0x00000040
#define P1D     0x00000080

#define P2UP    0x00000100
#define P2DOWN  0x00000200
#define P2LEFT  0x00000400
#define P2RIGHT 0x00000800
#define P2A     0x00001000
#define P2B     0x00002000
#define P2C     0x00004000
#define P2D     0x00008000

#define P1START 0x00010000
#define P1SEL   0x00020000
#define P2START 0x00040000
#define P2SEL   0x00080000

#define SPECIAL 0x01000000

#define PADDLE_1	0
#define PADDLE_2	1


/*--------------------------------------------------------------------------*/
static uint32 keys  =~0;
/*--------------------------------------------------------------------------*/


/*
 * waitPadReady()
 */
int waitPadReady(int port, int slot)
{
    int state;
    int lastState;
    char stateString[16];

    state = padGetState(port, slot);
    lastState = -1;
    while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
        if (state != lastState) {
            padStateInt2String(state, stateString);
            printf("Please wait, pad(%d,%d) is in state %s\n", 
                       port, slot, stateString);
        }
        lastState = state;
        state=padGetState(port, slot);
    }
    // Were the pad ever 'out of sync'?
    if (lastState != -1) {
        printf("Pad OK!\n");
    }
    return 0;
}

/*
 * padConnected()
 */
int padConnected(int port, int slot)
{
    int state;
    int lastState;

    state = padGetState(port, slot);
    lastState = -1;
    while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
        if (state != lastState)
        {
            if (state == PAD_STATE_DISCONN)
            	return 0;
        }
        lastState = state;
        state=padGetState(port, slot);
    }
    // Were the pad ever 'out of sync'?
    if (lastState != -1) {
        printf("Pad OK!\n");
    }
    return 1;
}

/*
 * initializePad()
 */
int initializePad(int port, int slot)
{

    int ret;
    int modes;
    int i;

    waitPadReady(port, slot);

    // How many different modes can this device operate in?
    // i.e. get # entrys in the modetable
    modes = padInfoMode(port, slot, PAD_MODETABLE, -1);
    printf("The device has %d modes\n", modes);

    if (modes > 0) {
        printf("( ");
        for (i = 0; i < modes; i++) {
            printf("%d ", padInfoMode(port, slot, PAD_MODETABLE, i));
        }
        printf(")");
    }

    printf("It is currently using mode %d\n", 
               padInfoMode(port, slot, PAD_MODECURID, 0));

    // If modes == 0, this is not a Dual shock controller 
    // (it has no actuator engines)
    if (modes == 0) {
        printf("This is a digital controller?\n");
        return 1;
    }

    // Verify that the controller has a DUAL SHOCK mode
    i = 0;
    do {
        if (padInfoMode(port, slot, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK)
            break;
        i++;
    } while (i < modes);
    if (i >= modes) {
        printf("This is no Dual Shock controller\n");
        return 1;
    }

    // If ExId != 0x0 => This controller has actuator engines
    // This check should always pass if the Dual Shock test above passed
    ret = padInfoMode(port, slot, PAD_MODECUREXID, 0);
    if (ret == 0) {
        printf("This is no Dual Shock controller??\n");
        return 1;
    }

    printf("Enabling dual shock functions\n");

    // When using MMODE_LOCK, user cant change mode with Select button
    padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);

    /*
    waitPadReady(port, slot);
    printf("infoPressMode: %d\n", padInfoPressMode(port, slot));

    waitPadReady(port, slot);        
    printf("enterPressMode: %d\n", padEnterPressMode(port, slot));
    */

    waitPadReady(port, slot);
    actuators = padInfoAct(port, slot, -1, 0);
    printf("# of actuators: %d\n",actuators);

    if (actuators != 0) {
        actAlign[0] = 0;   // Enable small engine
        actAlign[1] = 1;   // Enable big engine
        actAlign[2] = 0xff;
        actAlign[3] = 0xff;
        actAlign[4] = 0xff;
        actAlign[5] = 0xff;

        waitPadReady(port, slot);
        printf("padSetActAlign: %d\n", 
                   padSetActAlign(port, slot, actAlign));
    }
    else {
        printf("Did not find any actuators.\n");
    }
    waitPadReady(port, slot);

    return 1;
}



            
/*--------------------------------------------------------------------------*/
void input_init(void)
{
    int ret;
    
    padInit(0);

    pad_nb=1;
    printf("PortMax: %d\n", padGetPortMax());
    //printf("SlotMax: %d\n", padGetSlotMax(port));


    // paddle 1
    printf("Init Paddle 1\n");
    if((ret = padPortOpen(PADDLE_1, 0, padBuf_1)) != 0)
    {
        if(!initializePad(PADDLE_1, 0))
    	{
        	printf("pad initalization failed!\n");
        	SleepThread();
    	}
    }
    else
    {
        printf("padOpenPort failed: %d\n", ret);
        SleepThread();
    }
    
    printf("Init Paddle 2\n");
    // paddle 2
    
    if((ret = padPortOpen(PADDLE_2, 0, padBuf_2)) != 0)
    {
    	if(padConnected(PADDLE_2, 0))
    	{
		if(!initializePad(PADDLE_2, 0))
		{
	       		printf("pad initalization failed!\n");
		}
		else // pad initized
	  	pad_nb=2;
	 }
    } 
    else
    {
		printf("padOpenPort failed: %d\n", ret);
    }	

       
}

void input_shutdown(void)
{
 	padPortClose(PADDLE_1,0);
 	padPortClose(PADDLE_2,0);
 	padEnd();
}

inline void joySet (int button) {
	keys &= ~button;
}

void processEvents(void)
{
    // reset key values
    keys  =~0;
     	
    // ------- controller 1 ---------------------------------------
    while (padGetState(PADDLE_1, 0) != PAD_STATE_STABLE)
    	; // more error check ?
  
    if (padRead(PADDLE_1, 0, &buttons) != 0)
    {
    	paddata = 0xffff ^ ((buttons.btns[0] << 8) | buttons.btns[1]);

    	if((paddata & PAD_LEFT) || (((buttons.mode >> 4) == 0x07)&&(buttons.ljoy_h < 64)))
    	{
            joySet(P1LEFT);
        }         
        else if((paddata & PAD_RIGHT) || (((buttons.mode >> 4) == 0x07)&&(buttons.ljoy_h > 192)))
        {
           joySet(P1RIGHT);
        }
        
        if((paddata & PAD_UP) || (((buttons.mode >> 4) == 0x07)&&(buttons.ljoy_v < 64)))
        {
           joySet(P1UP);
        }        
        else if ((paddata & PAD_DOWN) || (((buttons.mode >> 4) == 0x07)&&(buttons.ljoy_v > 192)))
        {
            joySet(P1DOWN);
        }
        
        if(paddata & PAD_SQUARE)
        {
            joySet(P1C);
        }
        
        if(paddata & PAD_CROSS)
        {
            joySet(P1A);
        }
        
        if(paddata & PAD_CIRCLE)
        {
            joySet(P1B);
        }
        
        if(paddata & PAD_TRIANGLE)
        {
            joySet(P1D);
        }
        
        if(paddata & PAD_START)
        {
           joySet(P1START);
    	}
    	
    	if(paddata & PAD_SELECT)
        {
           joySet(P1SEL);
    	}
    	
    	if(paddata & PAD_L1)
        {
           AUDIO_PLAYING=!AUDIO_PLAYING;
	}
    	
    } //endif
    
    // ------- controller 2 ---------------------------------------
    if (pad_nb ==2) // poll 2nd joystick if detected
    {
	while (padGetState(PADDLE_2, 0) != PAD_STATE_STABLE)
	    	; // more error check ?
	  
	if (padRead(PADDLE_2, 0, &buttons) != 0)
	{
		paddata = 0xffff ^ ((buttons.btns[0] << 8) | buttons.btns[1]);
	    
		if((paddata & PAD_LEFT) || (((buttons.mode >> 4) == 0x07)&&(buttons.ljoy_h < 64)))
    		{
            	   joySet(P2LEFT);
        	}         
        	else if((paddata & PAD_RIGHT) || (((buttons.mode >> 4) == 0x07)&&(buttons.ljoy_h > 192)))
        	{
           	   joySet(P2RIGHT);
        	}
        
        	if((paddata & PAD_UP) || (((buttons.mode >> 4) == 0x07)&&(buttons.ljoy_v < 64)))
        	{
           	    joySet(P2UP);
        	}        
        	else if ((paddata & PAD_DOWN) || (((buttons.mode >> 4) == 0x07)&&(buttons.ljoy_v > 192)))
        	{
            	    joySet(P2DOWN);
        	}
	        
	        if(paddata & PAD_SQUARE)
	        {
	            joySet(P2C);
	        }
	        
	        if(paddata & PAD_CROSS)
	        {
	            joySet(P2A);
	        }
	        
	        if(paddata & PAD_CIRCLE)
	        {
	            joySet(P2B);
	        }
	        
	        if(paddata & PAD_TRIANGLE)
	        {
	            joySet(P2D);
	        }
	        
	        if(paddata & PAD_START)
	        {
	           joySet(P2START);
	    	}
	    	
	    	if(paddata & PAD_SELECT)
	        {
	           joySet(P2SEL);
	    	}
	    	
	    } //endif
	 } //  //endif joy 2
	 
}
        
/*--------------------------------------------------------------------------*/
inline unsigned char read_player1(void) 
{
    return keys&0xff;
}

/*--------------------------------------------------------------------------*/
inline unsigned char read_player2(void) 
{
    return (keys>>8)&0xff;
}

/*--------------------------------------------------------------------------*/
inline unsigned char read_pl12_startsel(void) 
{
    return (keys>>16)&0x0f;
}

void waitforX(void)
{

   //wait for X key press
   while(1)
   {  
    	while (padGetState(PADDLE_1, 0) != PAD_STATE_STABLE)
    		; // more error check ?

    	if (padRead(PADDLE_1, 0, &buttons) != 0)
    	{
	    	paddata = 0xffff ^ ((buttons.btns[0] << 8) | buttons.btns[1]);
                if(paddata & PAD_CROSS)
            		return;
    	}
   }

}
