/*
 *  input.c - Input Devices
 *  Copyright (C) 2004-2005 Olivier "Evilo" Biot (PS2 Port)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*-- Include Files ---------------------------------------------------------*/
#include <kernel.h>
#include <stdio.h>
#include <tamtypes.h>
#include <sifrpc.h>
#include <sifcmd.h> 
#include <loadfile.h>
#include <libpad.h>
#include "neocd.h"
#include "input.h"



#define ROM_PADMAN

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
static int pad_nb;

static int askForBios = 0;

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

    //printf("Enabling dual shock functions\n");

    // When using MMODE_LOCK, user cant change mode with Select button
    padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_UNLOCK);

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

void enterBIOS()
{
	askForBios = 1;
}

void processEvents(void)
{
    register int ret;
    // reset key values
    keys  =~0;
    
     	
    // ------- controller 1 ---------------------------------------
    //while (padGetState(PADDLE_1, 0) != PAD_STATE_STABLE); // more error check ?
    while (((ret=padGetState(PADDLE_1, 0)) != PAD_STATE_STABLE)&&(ret!=PAD_STATE_FINDCTP1)&&(ret != PAD_STATE_DISCONN)); // more error check ?

  
    if (padRead(PADDLE_1, 0, &buttons) != 0)
    {
    	paddata = 0xffff ^ buttons.btns;

    	if  ((paddata & PAD_LEFT) 
    	 || (((buttons.mode >> 4) == 0x07)
    	 && (buttons.ljoy_h < 64)))         	keys &= ~P1LEFT; // joySet(P1LEFT);
        else if((paddata & PAD_RIGHT) 
         || (((buttons.mode >> 4) == 0x07)
         && (buttons.ljoy_h > 192)))		keys &= ~P1RIGHT; // joySet(P1RIGHT);
        if ((paddata & PAD_UP) 
         || (((buttons.mode >> 4) == 0x07)
         && (buttons.ljoy_v < 64)))		keys &= ~P1UP; // joySet(P1UP);
        else if ((paddata & PAD_DOWN) 
         || (((buttons.mode >> 4) == 0x07)
         && (buttons.ljoy_v > 192)))		keys &= ~P1DOWN; // joySet(P1DOWN);
        if(paddata & PAD_SQUARE) 		keys &= ~P1C; //joySet(P1C);
        if(paddata & PAD_CROSS) 		keys &= ~P1A; //joySet(P1A);
        if(paddata & PAD_CIRCLE) 		keys &= ~P1B; //joySet(P1B);
	if(paddata & PAD_TRIANGLE) 		keys &= ~P1D; //joySet(P1D);
        if(paddata & PAD_START) 		keys &= ~P1START; //joySet(P1START);
    	if(paddata & PAD_SELECT) 		keys &= ~P1SEL; //joySet(P1SEL);
    	if((paddata & PAD_L2)&&
    	   (paddata & PAD_R2)) 			IngameMenu();
    	
    } //endif
    
    // ------- controller 2 ---------------------------------------
    if (pad_nb ==2) // poll 2nd joystick if detected
    {
	//while (padGetState(PADDLE_2, 0) != PAD_STATE_STABLE); // more error check ?
	while (((ret=padGetState(PADDLE_2, 0)) != PAD_STATE_STABLE)&&(ret!=PAD_STATE_FINDCTP1)&&(ret != PAD_STATE_DISCONN)); // more error check ?
	  
	if (padRead(PADDLE_2, 0, &buttons) != 0)
	{
		paddata = 0xffff ^ buttons.btns;
	    
		if ((paddata & PAD_LEFT) 
		 || (((buttons.mode >> 4) == 0x07)
		 &&(buttons.ljoy_h < 64)))		keys &= ~P2LEFT; // joySet(P2LEFT);
        	else if((paddata & PAD_RIGHT) 
        	 || (((buttons.mode >> 4) == 0x07)
        	 &&(buttons.ljoy_h > 192)))		keys &= ~P2RIGHT; // joySet(P2RIGHT);
        	if ((paddata & PAD_UP) 
        	 || (((buttons.mode >> 4) == 0x07)
        	 && (buttons.ljoy_v < 64)))		keys &= ~P2UP; // joySet(P2UP);
        	else if ((paddata & PAD_DOWN) 
        	 || (((buttons.mode >> 4) == 0x07)
        	 && (buttons.ljoy_v > 192)))		keys &= ~P2DOWN; // joySet(P2DOWN);
	        if(paddata & PAD_SQUARE) 		keys &= ~P2C; //joySet(P2C);
	        if(paddata & PAD_CROSS) 		keys &= ~P2A; //joySet(P2A);
		if(paddata & PAD_CIRCLE) 		keys &= ~P2B;//joySet(P2B);
	        if(paddata & PAD_TRIANGLE) 		keys &= ~P2D;//joySet(P2D);
	        if(paddata & PAD_START) 		keys &= ~P2START;//joySet(P2START);
	    	if(paddata & PAD_SELECT)		keys &= ~P2SEL;//joySet(P2SEL);
	    	if((paddata & PAD_L2)&&
    	   	   (paddata & PAD_R2)) 			IngameMenu();
	    	
	 } //endif
      } //  //endif joy 2
	 
      // check if enter bios from menu
      if (askForBios==1) 
      {
	 	keys &= SPECIAL;
	 	askForBios = 0;
      }
	
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
   int ret;
   //wait for X key press
   while(1)
   {  
    	while (((ret=padGetState(PADDLE_1, 0)) != PAD_STATE_STABLE)&&(ret!=PAD_STATE_FINDCTP1)&&(ret != PAD_STATE_DISCONN)); // more error check ?

    	if (padRead(PADDLE_1, 0, &buttons) != 0)
    	{
	    	paddata = 0xffff ^ buttons.btns;
                if(paddata & PAD_CROSS)
            		return;
    	}
   }

}

int isButtonPressed(u32 button)
{
   int ret;
   while (((ret=padGetState(PADDLE_1, 0)) != PAD_STATE_STABLE)&&(ret!=PAD_STATE_FINDCTP1)&&(ret != PAD_STATE_DISCONN)); // more error check ?
   if (padRead(PADDLE_1, 0, &buttons) != 0)
   {
    	paddata = 0xffff ^ buttons.btns;
     	if(paddata & button)
            return 1;
   }
   return 0;

}
