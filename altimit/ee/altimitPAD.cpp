/*========================================================================
==				AltimitPAD.cpp pad handling functions.	           		==
==				(c) 2004 t0mb0la (tomhawcroft@comcast.net)				==
== Refer to the file LICENSE in the main folder for license information	==
========================================================================*/

#include "altimit.h"
//#include "screenshot.h"

static char padBuf[256] __attribute__((aligned(64)));
struct padButtonStatus buttons;
PS2MouseData mouse;
char keypress;

extern altimitGS altGS;
extern gsDriver altGsDriver;
extern int heldtime, heldbutton;
extern int insmode;
extern int pointerX, pointerY;
extern int dualshockPAD, usepointer, activeMOUSE, activeKEYBD;
extern unsigned int paddata, old_pad, new_pad;
extern int boot;

int waitPadReady(int port, int slot)
{
 int state;
 int lastState;
 char stateString[16];

 state = padGetState(port, slot);
 lastState = -1;
 while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1))
 {
	if (state != lastState) padStateInt2String(state, stateString);
	lastState = state;
	state=padGetState(port, slot);
 }
 if (lastState != -1)
 {
#ifdef VERBOSE
	dbgprintf("Pad OK!\n");
	return 0;
#endif
 }
 return lastState;
}

int initializePad(int port, int slot)
{
	int modes, i;

	waitPadReady(port, slot);
	modes = padInfoMode(port, slot, PAD_MODETABLE, -1);
	i = 0;
	if (modes > 0)
	{
		i = 0;
		do
		{
			if (padInfoMode(port, slot, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK)
				break;
			i++;
		} while (i < modes);
	}
	if (i >= modes) { dualshockPAD = false; return 1; }
	modes = padInfoMode(port, slot, PAD_MODECUREXID, 0);
	if (modes == 0) { dualshockPAD = false; return 1; }
	dualshockPAD = true;
	padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
	waitPadReady(port, slot);
	return 1;
}

void startpad()
{
	int pad;

	padInit(0);
	if((pad = padPortOpen(0, 0, padBuf)) == 0)
	{
		dbgprintf("Fatal, padOpenPort failed!\n");
		SleepThread();
	}
	if(!initializePad(0, 0))
	{
		dbgprintf("Fatal, pad initialization failed!\n");
		SleepThread();
	}
	pad=padGetState(0, 0);
	while((pad != PAD_STATE_STABLE) && (pad != PAD_STATE_FINDCTP1))
	{
		if(pad==PAD_STATE_DISCONN)
		{
			dbgprintf("Pad is disconnected");
		}
		pad=padGetState(0, 0);
	}
	if(activeMOUSE)
	{
		PS2MouseSetBoundary(0, (altGS.WIDTH - 16), 0, (altGS.HEIGHT - 16));
		PS2MouseSetReadMode(PS2MOUSE_READMODE_ABS);
		PS2MouseSetPosition((altGS.WIDTH / 2), (altGS.HEIGHT / 2));
		PS2MouseSetAccel(2.0);
		PS2MouseSetThres(4);
		PS2MouseRead(&mouse);
	}
}

int readpadbutton()
{
 int pad, Xlimit, Ylimit; //, display;

 Xlimit = altGS.WIDTH - 16;
 Ylimit = altGS.HEIGHT - 16;
 pad = padRead(0, 0, &buttons);
 if (pad != 0)
 {
	paddata = 0xffff ^ ((buttons.btns[0] << 8) | buttons.btns[1]);
	if (paddata && paddata == old_pad) heldtime++;
	else heldtime = 0;
	if (heldtime>20) { heldbutton = paddata; heldtime=20; }
	else heldbutton = 0;
	new_pad = paddata & ~old_pad;
	old_pad = paddata;
	if (activeMOUSE)
	{
		PS2MouseRead(&mouse);
		pointerX = mouse.x;
		pointerY = mouse.y;
		if (mouse.buttons & 1) new_pad = new_pad | PAD_CROSS;
		else if (mouse.buttons & 2) new_pad = new_pad | PAD_SQUARE;
	}
	else if(usepointer)
	{		
		pointerX+=((((buttons.ljoy_h>>5)-3) & 0x0fffffffe)<<1);
		pointerY+=(((buttons.ljoy_v>>5)-3) & 0x0fffffffe);
		if (pointerX < 0) pointerX = 0;
		if (pointerX > Xlimit) pointerX = Xlimit;
		if (pointerY < 0) pointerY = 0;
		if (pointerY > Ylimit) pointerY = Ylimit;
	}
	if (activeKEYBD)
	{
		if(PS2KbdRead(&keypress))
		{
			if(keypress == PS2KBD_ESCAPE_KEY)
			{
				PS2KbdRead(&keypress);
				if(keypress == 0x29) new_pad = new_pad | PAD_RIGHT;
				else if(keypress == 0x2a) new_pad = new_pad | PAD_LEFT;
				else if(keypress == 0x2b) new_pad = new_pad | PAD_DOWN;
				else if(keypress == 0x2c) new_pad = new_pad | PAD_UP;
				else if(keypress == 0x23)
				{
					if (insmode) insmode = false;
					else insmode = true;
				}
				else if(keypress == 0x24) new_pad = new_pad | PAD_L1;
				else if(keypress == 0x27) new_pad = new_pad | PAD_R1;
				else if(keypress == 0x1b) new_pad = new_pad | PAD_TRIANGLE;
				else if(keypress == 0x01) new_pad = new_pad | PAD_SQUARE;
				keypress = '\0';
			}
			else
			{
				if(keypress == 0x0a) { new_pad = new_pad | PAD_CROSS; }
				else if(keypress == 0x07) { new_pad = new_pad | PAD_CIRCLE; keypress = '\0'; }
				else if(keypress == 0x09) { new_pad = new_pad | PAD_SELECT; keypress = '\0'; }
			}
		}
	}
/*	if ((new_pad & PAD_R2) && boot == HOST_BOOT)
	{
		display = altGsDriver.getCurrentDisplayBuffer();
		ps2_screenshot_file("host:altimit.tga", 
			altGsDriver.getFrameBufferBase(display)/256,
			altGS.WIDTH, altGS.HEIGHT, 1);
	}*/
	return new_pad;
 }
 return 0;
}
