/*
 *  mc.c - MC management
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
 
#include <libmc.h> 
#include <string.h>
#include <stdio.h>
#include <fileio.h>
#include "neocd.h"
#include "mc.h"

extern 		u8 ngcdIcn[];		
extern 		int size_ngcdIcn;

unsigned char neogeo_memorycard[SAVE_SIZE] __attribute__((aligned(64)));

int ret;

/* Check if save file is present on MC
   and create it if not
*/
void mc_initSave()
{
   int fd;//, iconSize;
   mcIcon iconSys;
   char fileName[128];

   static iconIVECTOR bgcolor[4] = {
		{  68,  23, 116,  0 }, // top left
		{ 255, 255, 255,  0 }, // top right
		{ 255, 255, 255,  0 }, // bottom left
		{  68,  23, 116,  0 }, // bottom right
		};
   static iconFVECTOR lightdir[3] = {
		{ 0.5, 0.5, 0.5, 0.0 },
		{ 0.0,-0.4,-0.1, 0.0 },
		{-0.5,-0.5, 0.5, 0.0 },
		};
   static iconFVECTOR lightcol[3] = {
		{ 0.3, 0.3, 0.3, 0.00 },
		{ 0.4, 0.4, 0.4, 0.00 },
		{ 0.5, 0.5, 0.5, 0.00 },
		};
   static iconFVECTOR ambient = { 0.50, 0.50, 0.50, 0.00 };


   // "Reset" memory card buffer
   memset ((void*)neogeo_memorycard,0,sizeof(neogeo_memorycard));   
   
   // try to open save file
   fd = fioOpen(MC_SAVE_FILE,O_RDONLY); 
   if(fd<=0) // file does not exist 
   { 
     //create neocdps2 dir on mc0: 
     printf("DIR not found\n");
     if(fioMkdir(MC_SAVE_PATH) < 0)
     {
       printf("neocd dir creation error\n");
       // save disabled
       neocdSettings.SaveOn=0;
       return;
     }
     // create save file if doesnt exist ! first time
     fd = fioOpen(MC_SAVE_FILE,O_WRONLY | O_CREAT);  
     if (fd < 0)
     { 
     	printf("MC : error open/writing %d\n",fd);
     	neocdSettings.SaveOn=0;
     }
     else
     { 
       // write neogeo memory card
       fioWrite(fd, neogeo_memorycard, 8192);
       fioClose(fd);
       mcSync(MC_WAIT, NULL, &ret);
       printf("save file created\n");
       neocdSettings.SaveOn=1;
       
       // Setup icon.sys
       memset(&iconSys, 0, sizeof(mcIcon));
       strcpy((char *)iconSys.head, "PS2D");
       strcpy_sjis((short *)&iconSys.title, "Neocd Card");
       iconSys.nlOffset = 13;
       iconSys.trans = 0x60;
       memcpy(iconSys.bgCol, bgcolor, sizeof(bgcolor));
       memcpy(iconSys.lightDir, lightdir, sizeof(lightdir));
       memcpy(iconSys.lightCol, lightcol, sizeof(lightcol));
       memcpy(iconSys.lightAmbient, ambient, sizeof(ambient));
       strcpy((char *)iconSys.view, "ngcd.ico");
       strcpy((char *)iconSys.copy, "ngcd.ico");
       strcpy((char *)iconSys.del, "ngcd.ico");
       
       // Write icon.sys
       sprintf(fileName, "%s/icon.sys", MC_SAVE_PATH);
       fd = fioOpen(fileName,O_WRONLY | O_CREAT);
       if(fd < 0) return;
       
       fioWrite(fd, &iconSys, sizeof(iconSys));
       fioClose(fd);
       mcSync(MC_WAIT, NULL, &ret);
       
       // Write icon file
       sprintf(fileName, "%s/ngcd.ico", MC_SAVE_PATH);
       fd = fioOpen(fileName,O_WRONLY | O_CREAT);
       if(fd < 0) return;
       
       fioWrite(fd, &ngcdIcn, size_ngcdIcn);
       fioClose(fd);
       printf("icon created\n");
       mcSync(MC_WAIT, NULL, &ret);

     }  
   } 
   // else nothing just read the memory card content
   else
   {
    // read save file  
    fioRead(fd, neogeo_memorycard, 8192);
    mcSync(MC_WAIT, NULL, &ret);
    fioClose(fd);
    printf("save file found\n");
    neocdSettings.SaveOn=1;
   }
  
	
}

// write save on MC
void mc_writeSave()
{
   int fd;

   if (neocdSettings.SaveOn)
   {
   	// try to open save file
   	fd = fioOpen(MC_SAVE_FILE,O_WRONLY); 
   	if(fd<=0) return ; 
   	fioWrite(fd, neogeo_memorycard, 8192);
   	//mcSync(MC_WAIT, NULL, &ret);
        fioClose(fd);
   }
   
}

// read neocd settings from MC
void mc_readSettings()
{
   
   int fd;

   if (neocdSettings.SaveOn)
   {
   	// try to open save file
   	fd = fioOpen(MC_CONF_FILE,O_RDONLY); 
   	if(fd<0) // file does not exist, create it
   	{
   	     // create conf file
     	     fd = fioOpen(MC_CONF_FILE,O_WRONLY | O_CREAT);
     	     fioWrite(fd, &neocdSettings, sizeof(struct_neocdSettings));
     	     mcSync(MC_WAIT, NULL, &ret);
   	}
   	else // else just read it ;)
   	{
   	     fioRead(fd, &neocdSettings, sizeof(struct_neocdSettings));
   	     mcSync(MC_WAIT, NULL, &ret);
   	     
   	     // check corresponding settings
   	     if (neocdSettings.fullscreen == 1)
   	     {
   	     	// should not be done here...
   	     	machine_def.y1_offset = 0;
	     	machine_def.y2_offset = machine_def.vdph << 4 ;
	     }
	     gp_setFilterMethod(neocdSettings.renderFilter);

   	}
   	fioClose(fd);
   }
}

// write neocd settings to MC
void mc_saveSettings()
{
   int fd;

   if (neocdSettings.SaveOn)
   {
   	// try to open save file
   	fd = fioOpen(MC_CONF_FILE,O_WRONLY); 
   	if(fd<0) // file does not exist, return
   	{
   		return;
   	}
   	fioWrite(fd, &neocdSettings, sizeof(struct_neocdSettings));
   	mcSync(MC_WAIT, NULL, &ret);
   	fioClose(fd);
   }
}
