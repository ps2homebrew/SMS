/**************************************
 ******    menu / GUI	          *****
 **************************************/ 
 

#include <string.h>
#include <stdio.h>
#include <libpad.h>
#include "../neocd.h"
#include "../save/mc.h"
#include "../video/video.h"
#include "../input/input.h"
#include "../sound/sjpcm.h"
#include "../gs/gfxpipe.h"
#include "../gs/gs.h"
#include "../gs/hw.h"


void IngameMenu()
{
	struct padButtonStatus pad1;
	int pad1_data = 0;
	int old_pad = 0;
	int new_pad;
	int selection = 0;
        static int ypos[8] = {59<<4,77<<4,95<<4,113<<4,131<<4,149<<4,167<<4,185<<4};
        int center_x,center_y;

	if (neocdSettings.soundOn)
	   SjPCM_Pause();
	   
	center_x = 0;
	center_y = (machine_def.vdph-224)/2;

	while(1) 
	{
		// All this probably isnt necessary.. eh..
	    	//gp_uploadTexture(&thegp, PCE_TEX, 640, 0, 0, 0x02, &bmp, 640, 256);
  	    	//gp_setTex(&thegp, PCE_TEX, 640, 640, 256, 0x02, 0, 0, 0);
      
      		
 	   	gp_setTex(&thegp, NGCD_TEX, 512, GS_TEX_SIZE_512, GS_TEX_SIZE_256, GS_PSMCT32, NGCD_TEX, 512, GS_PSMCT32);
		
		
		gp_texrect(&thegp, 		// gfxpipe	
		   0, machine_def.y1_offset, 	// x1,y1
		   0, 0, 			// u1,v1
		   320<<4, machine_def.y2_offset,	// x2,y2
		   320<<4, dph<<4, 			// u2,v2
		   10, 					// z
		   GS_SET_RGBA(0x80,0x80,0x80,0x80) 	// color
		  );

	    	//gp_gouradrect(&thegp, 0, ((vdph-dph)>>1)<<4, GS_SET_RGBA(0, 0, 0, 128), 320<<4, (vdph-((vdph-dph)>>1))<<4, GS_SET_RGBA(0, 0, 0,128 ), 11);
    	    	
    	    	// Shade neocd display
	    	gp_frect(&thegp, 0, 0, 320<<4, machine_def.vdph<<4, 11, GS_SET_RGBA(0, 0, 0, 64));

		//gp_gouradrect(&thegp,(96-16)<<4,54<<4,GS_SET_RGBA(0x00, 0x00, 0x40, 128), (320-96+16)<<4,211<<4, GS_SET_RGBA(0x40,0x40, 0x80, 128), 13);
		gp_gouradrect(&thegp,(96-16)<<4,54<<4,GS_SET_RGBA(0x00,0x00, 0x20, 100), (320-96+16)<<4,211<<4, GS_SET_RGBA(0x00,0x00, 0x20, 100), 13);
		gp_linerect(&thegp, (95-16)<<4, 54<<4, (320-96+16)<<4, 211<<4, 14, GS_SET_RGBA(255, 255, 255, 128));
	
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
			
		if (neocdSettings.soundOn==0)			
	        	TextOutC2(0<<4,320<<4,ypos[3],"Sound : Off",15);
		else
			TextOutC2(0<<4,320<<4,ypos[3],"Sound : On",15);
		
	    	if (neocdSettings.CDDAOn==0)			
			TextOutC2(0<<4,320<<4,ypos[4],"CDDA : Off",15);
		else
			TextOutC2(0<<4,320<<4,ypos[4],"CDDA : On",15);
		
		if (neocdSettings.region==REGION_JAPAN)
			TextOutC2(0<<4,320<<4,ypos[5],"Region : Japan",15);
		else if (neocdSettings.region==REGION_USA)
			TextOutC2(0<<4,320<<4,ypos[5],"Region : Usa",15);
		else //REGION_EUROPE
			TextOutC2(0<<4,320<<4,ypos[5],"Region : Europe",15);
	
		TextOutC2(0<<4,320<<4,ypos[6],"Enter Bios",15);
		
		TextOutC2(0<<4,320<<4,ypos[7],"Reset emulation",15);
	
		
		gp_frect(&thegp, (95-16)<<4, ypos[selection], (320-95+16)<<4, ypos[selection] + (16<<4), 16, GS_SET_RGBA(123, 255, 255, 40));

		gp_hardflush(&thegp);
		WaitForNextVRstart(1);
    		GS_SetCrtFB(whichdrawbuf);
	    	whichdrawbuf ^= 1;
	    	GS_SetDrawFB(whichdrawbuf);

		if(padGetState(0, 0) == PAD_STATE_STABLE) 
		{
			padRead(0, 0, &pad1);
			pad1_data = 0xffff ^ ((pad1.btns[0] << 8) | pad1.btns[1]);

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
			continue;
		}

     		if((new_pad & PAD_UP) && (selection > -1))
     		{
	     	  	if(selection>0)
           			selection--;
          		else selection=7; 
        	}
        
		if((new_pad & PAD_DOWN) && (selection < 8))
		{
		  	if(selection<7)          
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
			  	}	
			  	else // set fullscreen
			  	{ 
			  		machine_def.y1_offset = 0;
			  		machine_def.y2_offset = machine_def.vdph << 4 ;
			  	}
			
			}	
			
			if(selection == 3)  // sound on/off
			{
				neocdSettings.soundOn ^= 1;
				//if (neocdSettings.soundOn)
				//  neogeo_reset();
			}
			
			if(selection == 4) ; //neocdSettings.CDDAOn ^= 1; not activated
			
			if(selection == 5) // region
			{
				neocdSettings.region++;
				if (neocdSettings.region>2) neocdSettings.region=0;
				
				// Write System Region
				//m68k_write_memory_8(0x10FD83,neocdSettings.region);
				
				//neogeo_hreset();
			}


			if(selection == 6) // enter Bios
			{
	          		enterBIOS();
             			break;
			}
			
			if(selection == 7) // Soft reset
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
			pad1_data = 0xffff ^ ((pad1.btns[0] << 8) | pad1.btns[1]);
		}
		if(!(pad1_data & PAD_CROSS)) break;
	}
	
	// Clear the screen (with ZBuffer Disabled)
	gp_disablezbuf(&thegp);
	gp_frect(&thegp,0,0,320<<4,machine_def.vdph<<4,0,GS_SET_RGBA(0,0,0,0x80));
	gp_enablezbuf(&thegp);
	
	if (neocdSettings.soundOn)
		SjPCM_Play();
}



