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
// pad_dma_buf is provided by the user, one buf for each pad
// contains the pad's current state
static char padBuf[256] __attribute__((aligned(64)));

static char actAlign[6];
static int actuators;

int ret;
static int port, slot;
int i;
static struct padButtonStatus buttons;
static u32 paddata;

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


/*--------------------------------------------------------------------------*/
static uint32 keys   =~0;
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
 * initializePad()
 */
int
initializePad(int port, int slot)
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

    waitPadReady(port, slot);
    printf("infoPressMode: %d\n", padInfoPressMode(port, slot));

    waitPadReady(port, slot);        
    printf("enterPressMode: %d\n", padEnterPressMode(port, slot));

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

    padInit(0);

    port = 0; // 0 -> Connector 1, 1 -> Connector 2
    slot = 0; // Always zero if not using multitap

    printf("PortMax: %d\n", padGetPortMax());
    printf("SlotMax: %d\n", padGetSlotMax(port));

    // only manage joy 1
    if((ret = padPortOpen(port, slot, padBuf)) == 0)
    {
        printf("padOpenPort failed: %d\n", ret);
        SleepThread();
    }
    
    if(!initializePad(port, slot))
    {
        printf("pad initalization failed!\n");
        SleepThread();
    }


}

void input_shutdown(void)
{
 	;
}

inline void joyDown (int which, int button) {
	keys &= ~button;
}

inline void joyUp (int which, int button) {
        keys |= button;
}

void processEvents(void)
{
	
    while (padGetState(port, slot) != PAD_STATE_STABLE)
    	; // more error check ?
  
    ret = padRead(port, slot, &buttons); // port, slot, buttons        
    if (ret != 0)
    {
    	paddata = 0xffff ^ ((buttons.btns[0] << 8) | buttons.btns[1]);
    
    	if(paddata & PAD_LEFT)
    	{
            joyDown(port, P1LEFT);
        } else joyUp(port, P1LEFT);
        
        if(paddata & PAD_RIGHT)
        {
           joyDown(port, P1RIGHT);
        } else joyUp(port, P1RIGHT);
        
        if(paddata & PAD_UP)
        {
           joyDown(port, P1UP);
        } else joyUp(port, P1UP);
        
        if(paddata & PAD_DOWN)
        {
            joyDown(port, P1DOWN);
        } else joyUp(port, P1DOWN);
        
        if(paddata & PAD_SQUARE)
        {
            joyDown(port, P1C);
        } else joyUp(port, P1C);
        
        if(paddata & PAD_CROSS)
        {
            joyDown(port, P1A);
        } else joyUp(port, P1A);
        
        if(paddata & PAD_CIRCLE)
        {
            joyDown(port, P1B);
        } else joyUp(port, P1B);
        
        if(paddata & PAD_TRIANGLE)
        {
            joyDown(port, P1D);
        } else joyUp(port, P1D);
        
        if(paddata & PAD_START)
        {
           joyDown(port, P1START);
    	} else joyUp(port, P1START);
    	
    	if(paddata & PAD_SELECT)
        {
           joyDown(port, P1SEL);
    	} else joyUp(port, P1SEL);
    	
    }
    
}
        
/*--------------------------------------------------------------------------*/
unsigned char read_player1(void) {
    return keys&0xff;
}

/*--------------------------------------------------------------------------*/
unsigned char read_player2(void) {
    return (keys>>8)&0xff;
}

/*--------------------------------------------------------------------------*/
unsigned char read_pl12_startsel(void) {
    return (keys>>16)&0x0f;
}

void waitforX(void)
{

   //wait for X key press
   while(1)
   {  
    	while (padGetState(port, slot) != PAD_STATE_STABLE)
    		; // more error check ?
  
    	ret = padRead(port, slot, &buttons); // port, slot, buttons        
    	if (ret != 0)
    	{
	    	paddata = 0xffff ^ ((buttons.btns[0] << 8) | buttons.btns[1]);
                if(paddata & PAD_CROSS)
            		return;
    	}
    } 
}

