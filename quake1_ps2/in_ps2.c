/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// in_ps2.c -- PS2 input driver.

#include <tamtypes.h>
#include <libkbd.h>
#include <libmouse.h>

#include "quakedef.h"
#include "in_ps2.h"

PS2KbdRawKey key;
PS2MouseData mouse;

float   mouse_x, mouse_y;
float   old_mouse_x, old_mouse_y;

cvar_t		_windowed_mouse = {"_windowed_mouse","0", true};
cvar_t		m_filter = {"m_filter","0", true};

void IN_Init (void)
{
	if(PS2KbdInit() == 0)
    {
		printf("Failed to initialise PS2Kbd\n");
		SleepThread();
    }

	if(PS2MouseInit() == 0)
    {
		printf("Failed to initialise PS2Mouse\n");
		SleepThread();
    }
	
	PS2KbdSetReadmode(PS2KBD_READMODE_RAW);
	
	PS2MouseSetBoundary(0, 639, 0, 479);
	PS2MouseSetReadMode(PS2MOUSE_READMODE_ABS);
	PS2MouseSetPosition(320, 240);
	PS2MouseSetAccel(2.0);
	PS2MouseSetThres(4);
	
	mouse_x = 320; 
	mouse_y = 240;
	//mouse_avail = 1;
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
	qboolean isKeyDown = false;

	PS2KbdReadRaw(&key);
	
	if(key.state == PS2KBD_RAWKEY_DOWN)
		isKeyDown = true;

	switch(key.key)
	{ 
		case PS2_TAB: Key_Event(K_TAB, isKeyDown); break;
		case PS2_ESCAPE: Key_Event(K_ESCAPE, isKeyDown); break;
		case PS2_ENTER: Key_Event(K_ENTER, isKeyDown); break;
		case PS2_SPACE: Key_Event(K_SPACE, isKeyDown); break;
		case PS2_UPARROW: Key_Event(K_UPARROW, isKeyDown); break;
		case PS2_DOWNARROW: Key_Event(K_DOWNARROW, isKeyDown); break;
		case PS2_LEFTARROW: Key_Event(K_LEFTARROW, isKeyDown); break;
		case PS2_RIGHTARROW: Key_Event(K_RIGHTARROW, isKeyDown); break;
		case PS2_BACKSPACE: Key_Event(K_BACKSPACE, isKeyDown); break;
		case PS2_ALT: Key_Event(K_ALT, isKeyDown); break;
		case PS2_CTRL: Key_Event(K_CTRL, isKeyDown); break;
		case PS2_SHIFT: Key_Event(K_SHIFT, isKeyDown); break;
		case PS2_F1: Key_Event(K_F1, isKeyDown); break;
		case PS2_F1+1: Key_Event(K_F2, isKeyDown); break;
		case PS2_F1+2: Key_Event(K_F3, isKeyDown); break;
		case PS2_F1+3: Key_Event(K_F4, isKeyDown); break;
		case PS2_F1+4: Key_Event(K_F5, isKeyDown); break;
		case PS2_F1+5: Key_Event(K_F6, isKeyDown); break;
		case PS2_F1+6: Key_Event(K_F7, isKeyDown); break;
		case PS2_F1+7: Key_Event(K_F8, isKeyDown); break;
		case PS2_F1+8: Key_Event(K_F9, isKeyDown); break;
		case PS2_F1+9: Key_Event(K_F10, isKeyDown); break;
		case PS2_F1+10: Key_Event(K_F11, isKeyDown); break;
		case PS2_F1+11: Key_Event(K_F12, isKeyDown); break;
		case PS2_INS: Key_Event(K_INS, isKeyDown); break;
		case PS2_DEL: Key_Event(K_DEL, isKeyDown); break;		
		case PS2_PGDN: Key_Event(K_PGDN, isKeyDown); break;
		case PS2_PGUP: Key_Event(K_PGUP, isKeyDown); break;
		case PS2_HOME: Key_Event(K_HOME, isKeyDown); break;
		case PS2_END: Key_Event(K_END, isKeyDown); break;
		case PS2_PAUSE: Key_Event(K_PAUSE, isKeyDown); break;
		default:Key_Event(us_keymap[key.key], isKeyDown);break;
	}
	
	PS2MouseRead(&mouse);

	if(mouse.buttons > 0)
	{
		switch(mouse.buttons)
		{
			case 1:	Key_Event(K_MOUSE1,1);Key_Event(K_MOUSE1,0);break;
			case 2: Key_Event(K_MOUSE2,1);Key_Event(K_MOUSE2,0);break;
			case 4: Key_Event(K_MOUSE3,1);Key_Event(K_MOUSE3,0);break;
			default:break;
		}
	}
	
	if(mouse.wheel > 0)
	{
		Key_Event(K_MWHEELUP,1);
		Key_Event(K_MWHEELUP,0);
	}
	else if(mouse.wheel < 0)
	{
		Key_Event(K_MWHEELDOWN,1);
		Key_Event(K_MWHEELDOWN,0);
	}
}

void IN_Move (usercmd_t *cmd)
{
	PS2MouseRead(&mouse);
	mouse_x = (float) (mouse.x-(320));
	mouse_y = (float) (mouse.y-(240));
	
	PS2MouseSetPosition(320, 240);
	
	if (m_filter.value) {
		mouse_x = (mouse_x + old_mouse_x) * 0.5;
		mouse_y = (mouse_y + old_mouse_y) * 0.5;
	}

	old_mouse_x = 320;
	old_mouse_y = 240;
   
	mouse_x *= sensitivity.value;
	mouse_y *= sensitivity.value;
   
	if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;
	if (in_mlook.state & 1)
		V_StopPitchDrift ();
   
	if ( (in_mlook.state & 1) && !(in_strafe.state & 1)) {
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	} else {
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * mouse_y;
		else
			cmd->forwardmove -= m_forward.value * mouse_y;
	}
	mouse_x = mouse_y = 0.0;
}

