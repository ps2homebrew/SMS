/*
 *  menu.c
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
 

#include <string.h>
#include <stdio.h>
#include <libpad.h>

#include "neocd.h"
#include "../save/mc.h"
#include "../video/video.h"
#include "../input/input.h"
#include "../sound/sjpcm.h"
#include "../gs/gfxpipe.h"
#include "../gs/gs.h"
#include "../gs/hw.h"


#define MAX_MENU_ITEM 9

void IngameMenu()
{
	struct padButtonStatus pad1;
	int pad1_data = 0;
	int old_pad = 0;
	int new_pad;
	int selection = 0;
        static int ypos[MAX_MENU_ITEM] = {32<<4, 50<<4,68<<4,86<<4,104<<4,122<<4,140<<4,158<<4,176<<4};
        int center_x,center_y;
        
        int settingsChanged = 0;

	if (neocdSettings.soundOn)
	   SjPCM_Pause();
	   
	center_x = 0;
	center_y = (machine_def.vdph-224)/2;

	while(1) 
	{
     		
 	   	gp_setTex(&thegp, NGCD_TEX, 512, GS_TEX_SIZE_512, GS_TEX_SIZE_256, GS_PSMCT32, NGCD_TEX, 512, GS_PSMCT32);
		
		
		gp_texrect(&thegp, 		// gfxpipe	
		   0, machine_def.y1_offset, 	// x1,y1
		   0, 0, 			// u1,v1
		   320<<4, machine_def.y2_offset,	// x2,y2
		   320<<4, dph<<4, 			// u2,v2
		   10, 					// z
		   GS_SET_RGBA(0x80,0x80,0x80,0x80) 	// color
		  );

    	    	
    	    	// Shade neocd display
	    	gp_frect(&thegp, 0, 0, 320<<4, machine_def.vdph<<4, 11, GS_SET_RGBA(0, 0, 0, 64));

		gp_gouradrect(&thegp,(96-16)<<4,((ypos[0]>>4)-5)<<4,GS_SET_RGBA(0x00,0x00, 0x20, 100), (320-96+16)<<4,((ypos[MAX_MENU_ITEM-1]>>4)+23)<<4, GS_SET_RGBA(0x00,0x00, 0x20, 100), 13);
		gp_linerect(&thegp, (95-16)<<4, ((ypos[0]>>4)-5)<<4, (320-96+16)<<4, ((ypos[MAX_MENU_ITEM-1]>>4)+23)<<4, 14, GS_SET_RGBA(255, 255, 255, 128));
	
		TextOutC2(0<<4,320<<4,ypos[0]," - Resume - ",15);
		
		if (neocdSettings.renderFilter==0)
			TextOutC2(0<<4,320<<4,ypos[1],"Filter : Nearest",15);	
		else 
			TextOutC2(0<<4,320<<4,ypos[1],"Filter : Linear",15);	
			
		if (machine_def.vidsys == NTSC_MODE)
		   TextOutC2(0<<4,320<<4,ypos[2],"Fullscreen : Yes",15);
		else if (machine_def.y1_offset == 0)
		   TextOutC2(0<<4,320<<4,ypos[2],"Fullscreen : Yes",15);
		else
		  TextOutC2(0<<4,320<<4,ypos[2],"Fullscreen : No",15);
			
		if (neocdSettings.frameskip==0)			
        	  TextOutC2(0<<4,320<<4,ypos[3],"Frameskip : Off",15);
        	else
        	  TextOutC2(0<<4,320<<4,ypos[3],"Frameskip : On",15);

		
		if (neocdSettings.soundOn==0)			
	        	TextOutC2(0<<4,320<<4,ypos[4],"Sound : Off",15);
		else
			TextOutC2(0<<4,320<<4,ypos[4],"Sound : On",15);
		
	    	if (neocdSettings.CDDAOn==0)			
			TextOutC2(0<<4,320<<4,ypos[5],"CDDA : Off",15);
		else
			TextOutC2(0<<4,320<<4,ypos[5],"CDDA : On",15);
		
		if (neocdSettings.region==REGION_JAPAN)
			TextOutC2(0<<4,320<<4,ypos[6],"Region : Japan",15);
		else if (neocdSettings.region==REGION_USA)
			TextOutC2(0<<4,320<<4,ypos[6],"Region : Usa",15);
		else //REGION_EUROPE
			TextOutC2(0<<4,320<<4,ypos[6],"Region : Europe",15);
	
		TextOutC2(0<<4,320<<4,ypos[7],"Enter Bios",15);
		
		TextOutC2(0<<4,320<<4,ypos[8],"Reset emulation",15);
	
		
		gp_frect(&thegp, (95-16)<<4, ypos[selection], (320-95+16)<<4, ypos[selection] + (16<<4), 16, GS_SET_RGBA(123, 255, 255, 40));

		gp_hardflush(&thegp);
		WaitForNextVRstart(1);
    		GS_SetCrtFB(whichdrawbuf);
	    	whichdrawbuf ^= 1;
	    	GS_SetDrawFB(whichdrawbuf);

		if(padGetState(0, 0) == PAD_STATE_STABLE) 
		{
			padRead(0, 0, &pad1);
			pad1_data = 0xffff ^ pad1.btns;

			if((pad1.mode >> 4) == 0x07) {
				if(pad1.ljoy_v < 64) pad1_data |= PAD_UP;
				else if(pad1.ljoy_v > 192) pad1_data |= PAD_DOWN;
			}
		}
		new_pad = pad1_data & ~old_pad;
  		old_pad = pad1_data;

		//if((pad1_data & PAD_L2)&&
    	   	//   (pad1_data & PAD_R2))   break; // quit menu
		
		if(pad1_data & PAD_SELECT) // Screen positioning
		{
             
			if((pad1_data & PAD_UP) && machine_def.dispy) machine_def.dispy--;
			if(pad1_data & PAD_DOWN) machine_def.dispy++;
			if((pad1_data & PAD_LEFT) && machine_def.dispx) machine_def.dispx--;
			if(pad1_data & PAD_RIGHT) machine_def.dispx++;
			
			if (machine_def.vidsys == PAL_MODE)
			{
				neocdSettings.dispXPAL=machine_def.dispx;
				neocdSettings.dispYPAL=machine_def.dispy;
			}	
			else
			{
				neocdSettings.dispXNTSC=machine_def.dispx;
				neocdSettings.dispYNTSC=machine_def.dispy;
			}
			GS_SetDispMode(machine_def.dispx,machine_def.dispy,WIDTH,HEIGHT);
			settingsChanged=1;
			continue;
		}

     		if((new_pad & PAD_UP) && (selection > -1))
     		{
	     	  	if(selection>0)
           			selection--;
          		else selection=(MAX_MENU_ITEM-1); 
        	}
        
		if((new_pad & PAD_DOWN) && (selection < 8))
		{
		  	if(selection<(MAX_MENU_ITEM-1))          
            			selection++;
          		else selection=0;   
        	}


		if(new_pad & PAD_CROSS) 
		{
			if(selection == 0) break;
			
			if(selection == 1) // video filter
			{
				neocdSettings.renderFilter ^= 1;
				gp_setFilterMethod(neocdSettings.renderFilter);
				settingsChanged=1;
			}	
		
			if(selection == 2) // fullscreen
			{
				if (machine_def.vidsys == NTSC_MODE)
					return;
				
				// option for PAL user only
			  	if (machine_def.y1_offset == 0) // set not fullscreen
			  	{ 
			  		machine_def.y1_offset = ((machine_def.vdph-dph)>>1) << 4 ;
					machine_def.y2_offset = (machine_def.vdph-((machine_def.vdph-dph)>>1)) << 4;
					neocdSettings.fullscreen = 0;
			  	}	
			  	else // set fullscreen
			  	{ 
			  		machine_def.y1_offset = 0;
			  		machine_def.y2_offset = machine_def.vdph << 4 ;
			  		neocdSettings.fullscreen = 1;
			  	}
			  	settingsChanged=1;
			
			}	
			
			if(selection == 3)  // frameskip
			{
				neocdSettings.frameskip ^= 1;
				settingsChanged=1;
			}
				
			if(selection == 4)  // sound on/off
			{
				neocdSettings.soundOn ^= 1;
				settingsChanged=1;
			}
			
			if(selection == 5) ; //neocdSettings.CDDAOn ^= 1; not activated
			
			if(selection == 6) // region
			{
				neocdSettings.region++;
				if (neocdSettings.region>2) neocdSettings.region=0;
				settingsChanged=1;
			}


			if(selection == 7) // enter Bios
			{
	          		enterBIOS();
             			break;
			}
			
			if(selection == 8) // Soft reset
			{
	          		neogeo_reset();
             			break;
			}
		}
	}

	// Wait till X has stopped been pressed.
	while(1) {
		if(padGetState(0, 0) == PAD_STATE_STABLE) {
			padRead(0, 0, &pad1); // port, slot, buttons
			pad1_data = 0xffff ^ pad1.btns;
		}
		if(!(pad1_data & PAD_CROSS)) break;
	}
	
	// save settings to MC
	if (settingsChanged)
	  mc_saveSettings();
	  
	
	// Clear the screen (with ZBuffer Disabled)
	gp_disablezbuf(&thegp);
	gp_frect(&thegp,0,0,320<<4,machine_def.vdph<<4,0,GS_SET_RGBA(0,0,0,0x80));
	gp_enablezbuf(&thegp);
	
	if (neocdSettings.soundOn)
		SjPCM_Play();
}



