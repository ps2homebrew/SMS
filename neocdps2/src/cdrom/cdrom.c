/*
 *  cdrom.h
 *  Copyright (C) 2001-2003 Foster (Original Code)
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

//-- Include files -----------------------------------------------------------
#include <stdio.h>
#include <fileio.h> 
#include <string.h>
#include <libcdvd.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <kernel.h>
#include "cdrom.h"
#include "cdvd_rpc.h"
#include "neocd.h"


/*-- Definitions -----------------------------------------------------------*/

char cdpath[128] __attribute__((aligned(64))) = "cdfs:\\"; 

char bootpath[128] __attribute__((aligned(64)));

#define BUFFER_SIZE 131072
#define PRG_TYPE    0
#define FIX_TYPE    1
#define SPR_TYPE    2
#define Z80_TYPE    3
#define PAT_TYPE    4
#define PCM_TYPE    5
#define OBJ_TYPE    6 
#define min(a, b) ((a) < (b)) ? (a) : (b)

#define	ExitHandler()	__asm__ volatile("sync.l; ei")
#define WAIT_HSYNC	480

/*-- Exported Functions ----------------------------------------------------*/
int    cdrom_init1(void);
int    cdrom_load_prg_file(char *, unsigned int);
int    cdrom_load_z80_file(char *, unsigned int);
int    cdrom_load_fix_file(char *, unsigned int);
int    cdrom_load_spr_file(char *, unsigned int);
int    cdrom_load_obj_file(char *, unsigned int);
int    cdrom_load_pcm_file(char *, unsigned int);
int    cdrom_load_pat_file(char *, unsigned int, unsigned int);
//void   cdrom_load_files(void);
int    cdrom_process_ipl(void);
void   cdrom_shutdown(void);
void   fix_conv(unsigned char *, unsigned char *, int, unsigned char *);
void   spr_conv(unsigned char *, unsigned char *, int, unsigned char *);
void   neogeo_upload(void);
void   cdrom_load_title(void);
void   cdrom_apply_patch(short *source, int offset, int bank);

//-- Private Variables -------------------------------------------------------
static char    cdrom_buffer[BUFFER_SIZE] __attribute__((aligned(64)));
static char    Path[64]  __attribute__((aligned(64)));

//-- Private Function --------------------------------------------------------
static    int    recon_filetype(char *);

int spr_length;

//-- Exported Variables ------------------------------------------------------
int        img_display = 1;

//----------------------------------------------------------------------------
static void CallbackDelayTh(int id, uint16 time, void *sema_id)
{
	iSignalSema((int)sema_id);
	ExitHandler();
}
void delayThread(uint16 hsync)
{
	struct t_ee_sema sparam;
	int sema_id;
	
	sparam.init_count = 0;
	sparam.max_count = 1;
	sparam.option = 0;
	
	sema_id = CreateSema(&sparam);
	SetAlarm(hsync,CallbackDelayTh,(void *)sema_id);
	WaitSema(sema_id);

	DeleteSema(sema_id);
}
//----------------------------------------------------------------------------
// return 0 if game detected on local "host", under "cd" folder
// return 1 in other case (-> cdrom0)
int cdrom_init1(void)
{    
    int neocd_disc, swap_disc;
    int fd, disk_type,trayflg,traycnt;
    
    // try to read the IPL.TXT file from host
    strcpy (bootpath,path_prefix);
    /*if (boot_mode == BOOT_CD) 
	 strcat (bootpath,"CD\\IPL.TXT;1");
    else */
    	strcat (bootpath,"CD\\IPL.TXT");
    
    fd = fioOpen(bootpath, O_RDONLY);
    if (fd>0)
    {  
    	printf("Game Detected : Loading from host\n");
    	fioClose(fd);
    	return BOOT_HOST;
    }
    fioClose(fd);

    // else
    printf("CD-ROM detect....\n");
    
    
    // INIT CDFS SYSTEM
    printf("Init CDFS\n");
    CDVD_Init();
    CDVD_DiskReady(CdBlock);
    
    neocd_disc=0;
    swap_disc=0;
    /* Look for disc exchange */
    while (!neocd_disc)
    {
      // flush cache
      CDVD_FlushCache();
      
      if (swap_disc)
      {
      	traycnt=0;
      	display_insertscreen(); // insert CD
      	cdTrayReq(CdTrayCheck,&trayflg);
      	printf("Waiting for disc swap....\n");
      	waitforX();
      	display_loadingscreen(); // disc Access
      
      	while(CDVD_DiskReady(CdBlock)==CdNotReady) ;
                        
      	do
      	{
      	    delayThread(WAIT_HSYNC * 15);
	    while(!cdTrayReq(CdTrayCheck,&trayflg)){
	        delayThread(WAIT_HSYNC);
	    }
	    traycnt+= trayflg;
  	    while(CDVD_DiskReady(CdBlock)==CdNotReady) ;
	    disk_type = cdGetDiscType();
	    printf ("disc_type : %d",disk_type);
      	}
      	while((disk_type == CDVD_TYPE_NODISK) || 
              (disk_type == CDVD_TYPE_DETECT) || 
              (disk_type == CDVD_TYPE_DETECT_CD));
      }
      
      // try to open IPL.TXT
      while(CDVD_DiskReady(CdBlock)==CdNotReady) ;
      fd = fioOpen("cdfs:\\IPL.TXT", O_RDONLY);
      if (fd<0)
      {
      	 fioClose(fd);
	 printf("No Neogeo CD detected\n");
	 swap_disc++;
      }
      else (neocd_disc=1);
    } // end while
        
    printf("NeogeoCD Discs detected!\n");
    fioClose(fd);
  
    return BOOT_CD;
}

//----------------------------------------------------------------------------
void    cdrom_shutdown(void)
{
    /* free loading picture surface ??? */
}
//----------------------------------------------------------------------------
int    cdrom_load_prg_file(char *FileName, unsigned int Offset)
{
    int 	fd;
    char    	*Ptr;
    int         Readed;

    if (game_boot_mode == BOOT_CD)
    	strcpy(Path, cdpath);
    else
        strcpy(Path, path_prefix);
    
    if (game_boot_mode == BOOT_HOST)
          strcat(Path, "CD\\");

    strcat(Path, FileName);
    
    // if games loaded from CD and from "cd" directory
    /*if ((game_boot_mode == BOOT_HOST)  && (boot_mode = BOOT_CD))
    	strcat(Path, ";1");
    */

    fd = fioOpen(Path, O_RDONLY);
    if (fd<0) {
        printf("Could not open %s", Path);
        return 0;
    }
    
    Ptr = neogeo_prg_memory + Offset;
    
    do
    {
    	Readed = fioRead(fd, cdrom_buffer, BUFFER_SIZE);
        swab(cdrom_buffer, Ptr, Readed);
        Ptr += Readed;
    }
    while( Readed == BUFFER_SIZE );
       
    fioClose(fd);
    return 1;
}

//----------------------------------------------------------------------------
int    cdrom_load_z80_file(char *FileName, unsigned int Offset)
{
    int 	fd;

    if (game_boot_mode == BOOT_CD)
    	strcpy(Path, cdpath);
    else
        strcpy(Path, path_prefix);
    
    if (game_boot_mode == BOOT_HOST)
          strcat(Path, "CD\\");
    
    strcat(Path, FileName);
    
    // if games loaded from CD and from "cd" directory
    /*if ((game_boot_mode == BOOT_HOST)  && (boot_mode = BOOT_CD))
    	strcat(Path, ";1");
*/


    printf("LOADING Z80 FILE\n");
    fd = fioOpen(Path, O_RDONLY);
    if (fd<0) {
        printf("Could not open %s", Path);
        return 0;
    }
    fioRead(fd, &subcpu_memspace[Offset], Z80_MEMSIZE);
    
    fioClose(fd);
    return 1;
}

//----------------------------------------------------------------------------
int    cdrom_load_fix_file(char *FileName, unsigned int Offset)
{
    int 	fd;
    char    	*Ptr, *Src;
    int        	Readed;

    if (game_boot_mode == BOOT_CD)
    	strcpy(Path, cdpath);
    else
        strcpy(Path, path_prefix);
    
    if (game_boot_mode == BOOT_HOST)
          strcat(Path, "CD\\");
    
    strcat(Path, FileName);
    
    // if games loaded from CD and from "cd" directory
    /*if ((game_boot_mode == BOOT_HOST)  && (boot_mode = BOOT_CD))
    	strcat(Path, ";1");
    */
    
    fd = fioOpen(Path, O_RDONLY);
    if (fd<0) {
        printf("Could not open %s", Path);
        return 0;
    }
    
    Ptr = neogeo_fix_memory + Offset;
    
    do {
        memset(cdrom_buffer, 0, BUFFER_SIZE);
        Readed = fioRead(fd,cdrom_buffer, BUFFER_SIZE);
        
        Src = cdrom_buffer;
        fix_conv(Src, Ptr, Readed, video_fix_usage + (Offset>>5));
        Ptr += Readed;
        Offset += Readed;
    } while( Readed == BUFFER_SIZE );
    
    fioClose(fd);
    return 1;
}

//----------------------------------------------------------------------------
int    cdrom_load_spr_file(char *FileName, unsigned int Offset)
{
    int 	fd;
    char    	*Ptr;
    int        	Readed;

    if (game_boot_mode == BOOT_CD)
    	strcpy(Path, cdpath);
    else
        strcpy(Path, path_prefix);
    
    if (game_boot_mode == BOOT_HOST)
          strcat(Path, "CD\\");
    
    strcat(Path, FileName);
    
    // if games loaded from CD and from "cd" directory
    /*if ((game_boot_mode == BOOT_HOST)  && (boot_mode = BOOT_CD))
    	strcat(Path, ";1");
    */
    fd = fioOpen(Path, O_RDONLY);
    if (fd<0) {
        printf("Could not open %s", Path);
        return 0;
    }
    
    Ptr = neogeo_spr_memory + Offset;
    
    do {
        memset(cdrom_buffer, 0, BUFFER_SIZE);
        Readed = fioRead(fd, cdrom_buffer, BUFFER_SIZE);
        spr_conv(cdrom_buffer, Ptr, Readed, video_spr_usage + (Offset>>7));
        Offset += Readed;
        Ptr += Readed;
    } while( Readed == BUFFER_SIZE );
    fioClose(fd);
    
    return 1;
}
//----------------------------------------------------------------------------
int    cdrom_load_obj_file(char *FileName, unsigned int Offset)
{
    int 	fd;
    char    	*Ptr;
    int        	Readed;

    if (game_boot_mode == BOOT_CD)
    	strcpy(Path, cdpath);
    else
        strcpy(Path, path_prefix);
    
    if (game_boot_mode == BOOT_HOST)
          strcat(Path, "CD\\");
    
    strcat(Path, FileName);
    
    // if games loaded from CD and from "cd" directory
    /*if ((game_boot_mode == BOOT_HOST)  && (boot_mode = BOOT_CD))
    	strcat(Path, ";1");
    */
    fd = fioOpen(Path, O_RDONLY);
    if (fd<0) {
        printf("Could not open %s", Path);
        return 0;
    }
    
    Ptr = neogeo_spr_memory + Offset;
    
    do {
        memset(cdrom_buffer, 0, BUFFER_SIZE);
        Readed = fioRead(fd, cdrom_buffer, BUFFER_SIZE);
        spr_conv(cdrom_buffer, Ptr, Readed, video_spr_usage + (Offset>>7)); // TO BE FIXED
        Offset += Readed;
        Ptr += Readed;
    } while( Readed == BUFFER_SIZE );
    fioClose(fd);
    return 1;
}
//----------------------------------------------------------------------------
int    cdrom_load_pcm_file(char *FileName, unsigned int Offset)
{
    int 	fd;
    char        *Ptr;

    if (game_boot_mode == BOOT_CD)
    	strcpy(Path, cdpath);
    else
        strcpy(Path, path_prefix);
    
    if (game_boot_mode == BOOT_HOST)
          strcat(Path, "CD\\");
          
    strcat(Path, FileName);
    
    // if games loaded from CD and from "cd" directory
    /*if ((game_boot_mode == BOOT_HOST)  && (boot_mode = BOOT_CD))
    	strcat(Path, ";1");
	*/
    fd = fioOpen(Path, O_RDONLY);
    if (fd<0) {
        printf("Could not open %s", Path);
        return 0;
    }

    Ptr = neogeo_pcm_memory + Offset;
    fioRead(fd, Ptr, 0x100000);
    fioClose(fd);
    return 1;
}

//----------------------------------------------------------------------------
int    cdrom_load_pat_file(char *FileName, unsigned int Offset, unsigned int Bank)
{

    int 	fd;
    int        	Readed;

    if (game_boot_mode == BOOT_CD)
    	strcpy(Path, cdpath);
    else
        strcpy(Path, path_prefix);
    
    if (game_boot_mode == BOOT_HOST)
          strcat(Path, "CD\\");
    
    strcat(Path, FileName);
    
    // if games loaded from CD and from "cd" directory
    /*if ((game_boot_mode == BOOT_HOST)  && (boot_mode = BOOT_CD))
    	strcat(Path, ";1");
    */
    fd = fioOpen(Path, O_RDONLY);
    if (fd<0) {
        printf("Could not open %s", Path);
        return 0;
    }
    
    Readed = fioRead(fd, cdrom_buffer, BUFFER_SIZE);
    swab(cdrom_buffer, cdrom_buffer, Readed);
    cdrom_apply_patch((short*)cdrom_buffer, Offset, Bank);

    fioClose(fd);

    return 1;
}


int hextodec(char c) {
	switch (tolower(c)) {
	case '0':	return 0;
	case '1':	return 1;
	case '2':	return 2;
	case '3':	return 3;
	case '4':	return 4;
	case '5':	return 5;
	case '6':	return 6;
	case '7':	return 7;
	case '8':	return 8;
	case '9':	return 9;
	case 'a':	return 10;
	case 'b':	return 11;
	case 'c':	return 12;
	case 'd':	return 13;
	case 'e':	return 14;
	case 'f':	return 15;
	default:	return 0;
	}
}



//----------------------------------------------------------------------------
int    cdrom_process_ipl(void)
{
    
    int		fd;
    char	Path[128] __attribute__((aligned(64)));
    char	iplBuffer[1024] __attribute__((aligned(64))); // should be largely enough
    char	FileName[16]  __attribute__((aligned(64)));
    int        	FileType;
    int        	Bnk;
    int        	Off;
    int        	i, j;
    int		ipl_size;


    printf("opening IPL.TXT...\n");

    if (game_boot_mode == BOOT_CD)
    	strcpy(Path, cdpath);
    else
        strcpy(Path, path_prefix);
    
    if (game_boot_mode == BOOT_HOST)
          strcat(Path, "CD\\");
    
    strcat(Path, IPL_TXT);
    
    // if games loaded from CD and from "cd" directory
    /*if ((game_boot_mode == BOOT_HOST)  && (boot_mode = BOOT_CD))
    	strcat(Path, ";1");
	*/
    fd = fioOpen(Path, O_RDONLY);
    if (fd<0) 
    {
        printf("Could not open IPL.TXT!\n");
        return 0;
    }
    ipl_size = fioRead(fd, iplBuffer, 1024);
    fioClose(fd);
    
    i=0;
    while (i<(ipl_size-2))
    {
        Bnk=0;
	Off=0;
        j=0;

        processEvents();
        
        while((iplBuffer[i] != ',')&&(iplBuffer[i]!=0))
            FileName[j++] = CHANGECASE(iplBuffer[i++]);
        FileName[j]=0;

        j -= 3;
        if (j>0)
        {
            FileType = recon_filetype(&FileName[j]);
            i++;
            j=0;
            while(iplBuffer[i] != ',')
            {
                Bnk*=10;
		Bnk+=iplBuffer[i]-'0';
		i++;
	    }

            i++;
            j=0;

            while(iplBuffer[i] != 0x0D)
            {
		Off*=16;
		Off+=hextodec(iplBuffer[i++]);
	    }
            Bnk &= 3;
            
            printf("Loading File: %s %02x %08x\n", FileName, Bnk, Off);

            switch( FileType ) {
            case PRG_TYPE:
                if (!cdrom_load_prg_file(FileName, Off)) {
                    //fioClose(fd);
                    return 0;
                }
                break;
            case FIX_TYPE:
                if (!cdrom_load_fix_file(FileName, (Off>>1))) {
                    //fioClose(fd);
                    return 0;
                }
                break;
            case SPR_TYPE:
                if (!cdrom_load_spr_file(FileName, (Bnk*0x100000) + Off)) {
                    //fioClose(fd);
                    return 0;
                }
                break;
            case OBJ_TYPE: // TEST
                if (!cdrom_load_obj_file(FileName, (Bnk*0x100000) + Off)) {
                    //fioClose(fd);
                    return 0;
                }
                break;
            case Z80_TYPE:
                if (!cdrom_load_z80_file(FileName, (Off>>1))) {
                    //fioClose(fd);
                    return 0;
                }
                break;
            case PAT_TYPE:
                if (!cdrom_load_pat_file(FileName, Off, Bnk)) {
                    //fioClose(fd);
                    return 0;
                }
                break;
            case PCM_TYPE:
                if (!cdrom_load_pcm_file(FileName, (Bnk*0x80000) + (Off>>1))) {
                    //fioClose(fd);
                    return 0;
                }
                break;
            }
        }
        i+=2;
    }
   
    printf("LOADING COMPLETE...\n");

    return 1;
}

//----------------------------------------------------------------------------
int    recon_filetype(char *ext)
{    
    if (strcmp(ext, PRG)==0)
        return PRG_TYPE;
    
    if (strcmp(ext, FIX)==0)
        return FIX_TYPE;
    
    if (strcmp(ext, SPR)==0)
        return SPR_TYPE;
        
    if (strcmp(ext, OBJ)==0) // TEST
        return OBJ_TYPE;
        
    if (strcmp(ext, Z80)==0)
        return Z80_TYPE;
        
    if (strcmp(ext, PAT)==0)
        return PAT_TYPE;
    
    if (strcmp(ext, PCM)==0)
        return PCM_TYPE;
        
    return    -1;
}


//----------------------------------------------------------------------------
unsigned int motorola_peek(unsigned char *address) 
{
    unsigned int a,b,c,d;
	
	a=address[0]<<24;
	b=address[1]<<16;
	c=address[2]<<8;
	d=address[3]<<0;
	
	return (a|b|c|d);
}

//----------------------------------------------------------------------------
void    cdrom_load_files(void)
{

    char    Entry[32], FileName[13];
    char    *Ptr, *Ext;
    int     i, j, Bnk, Off, Type, Reliquat;
    
    if (neocdSettings.soundOn)
       SjPCM_Pause();


    if (m68k_read_memory_8(M68K_GetAReg(0))==0)
        return;


    cdda_stop();
    cdda_current_track = 0;

    m68k_write_memory_32(0x10F68C, 0x00000000);
    m68k_write_memory_8(0x10F6C3, 0x00);
    m68k_write_memory_8(0x10F6D9, 0x01);
    m68k_write_memory_8(0x10F6DB, 0x01);
    m68k_write_memory_32(0x10F742, 0x00000000);
    m68k_write_memory_32(0x10F746, 0x00000000);
    m68k_write_memory_8(0x10FDC2, 0x01);
    m68k_write_memory_8(0x10FDDC, 0x00);
    m68k_write_memory_8(0x10FDDD, 0x00);
    m68k_write_memory_8(0x10FE85, 0x01);
    m68k_write_memory_8(0x10FE88, 0x00);
    m68k_write_memory_8(0x10FEC4, 0x01);

    // Display Loading pict
    display_loadingscreen();


    Ptr = neogeo_prg_memory + M68K_GetAReg(0);


    do {
        Reliquat = ((int)Ptr)&1;

        if (Reliquat)
            Ptr--;

        swab(Ptr, Entry, 32);
        i=Reliquat;

        while((Entry[i]!=0)&&(Entry[i]!=';')) {
            FileName[i-Reliquat] = CHANGECASE(Entry[i]);
            i++;
        }

        FileName[i-Reliquat] = 0;

        if (Entry[i]==';')    // 01/05/99 MSLUG2 FIX
            i += 2;

        i++;

        Bnk = Entry[i++]&3;

        if (i&1)
            i++;


        Off = motorola_peek(&Entry[i]);
        i += 4;
        Ptr += i;

        printf("Loading File: %s %02x %08x\n", FileName, Bnk, Off);

        j=0;

        while(FileName[j] != '.' && FileName[j] != '\0')
            j++;

        if(FileName[j]=='\0')
        {
            sprintf("Internal Error loading file: %s",FileName);
            exit(1);
        }

        j++;
        Ext=&FileName[j];

        Type = recon_filetype(Ext);

        switch( Type ) {
        case PRG_TYPE:
            cdrom_load_prg_file(FileName, Off);
            break;
        case FIX_TYPE:
            cdrom_load_fix_file(FileName, Off>>1);
            break;
        case SPR_TYPE:
            cdrom_load_spr_file(FileName, (Bnk*0x100000) + Off);
            break;
        case OBJ_TYPE:
            cdrom_load_obj_file(FileName, (Bnk*0x100000) + Off);
            break;
        case Z80_TYPE:
            cdrom_load_z80_file(FileName, Off>>1);
            break;
        case PAT_TYPE:
            cdrom_load_pat_file(FileName, Off, Bnk);
            break;
        case PCM_TYPE:
            cdrom_load_pcm_file(FileName, (Bnk*0x80000) + (Off>>1));
            break;
        }
        
        processEvents();

    } while( Entry[i] != 0);
	
    // update neocd time
    //neocd_time=PS2_GetTicks()+REFRESHTIME;
    if (neocdSettings.soundOn)
		SjPCM_Play();


}


//----------------------------------------------------------------------------
void	fix_conv(unsigned char *Src, unsigned char *Ptr, int Taille,
	unsigned char *usage_ptr)
{
	int		i;
	unsigned char	usage;
	
	for(i=Taille;i>0;i-=32) {
		usage = 0;
		*Ptr++ = *(Src+16);
		usage |= *(Src+16);
		*Ptr++ = *(Src+24);
		usage |= *(Src+24);
		*Ptr++ = *(Src);
		usage |= *(Src);
		*Ptr++ = *(Src+8);
		usage |= *(Src+8);
		Src++;
		*Ptr++ = *(Src+16);
		usage |= *(Src+16);
		*Ptr++ = *(Src+24);
		usage |= *(Src+24);
		*Ptr++ = *(Src);
		usage |= *(Src);
		*Ptr++ = *(Src+8);
		usage |= *(Src+8);
		Src++;
		*Ptr++ = *(Src+16);
		usage |= *(Src+16);
		*Ptr++ = *(Src+24);
		usage |= *(Src+24);
		*Ptr++ = *(Src);
		usage |= *(Src);
		*Ptr++ = *(Src+8);
		usage |= *(Src+8);
		Src++;
		*Ptr++ = *(Src+16);
		usage |= *(Src+16);
		*Ptr++ = *(Src+24);
		usage |= *(Src+24);
		*Ptr++ = *(Src);
		usage |= *(Src);
		*Ptr++ = *(Src+8);
		usage |= *(Src+8);
		Src++;
		*Ptr++ = *(Src+16);
		usage |= *(Src+16);
		*Ptr++ = *(Src+24);
		usage |= *(Src+24);
		*Ptr++ = *(Src);
		usage |= *(Src);
		*Ptr++ = *(Src+8);
		usage |= *(Src+8);
		Src++;
		*Ptr++ = *(Src+16);
		usage |= *(Src+16);
		*Ptr++ = *(Src+24);
		usage |= *(Src+24);
		*Ptr++ = *(Src);
		usage |= *(Src);
		*Ptr++ = *(Src+8);
		usage |= *(Src+8);
		Src++;
		*Ptr++ = *(Src+16);
		usage |= *(Src+16);
		*Ptr++ = *(Src+24);
		usage |= *(Src+24);
		*Ptr++ = *(Src);
		usage |= *(Src);
		*Ptr++ = *(Src+8);
		usage |= *(Src+8);
		Src++;
		*Ptr++ = *(Src+16);
		usage |= *(Src+16);
		*Ptr++ = *(Src+24);
		usage |= *(Src+24);
		*Ptr++ = *(Src);
		usage |= *(Src);
		*Ptr++ = *(Src+8);
		usage |= *(Src+8);
		Src+=25;
		*usage_ptr++ = usage;
	}	
}

//----------------------------------------------------------------------------
void spr_conv(unsigned char *Src1, unsigned char *Ptr, int Taille,
	unsigned char *usage_ptr)
{
	unsigned char	*Src2;
	unsigned char	reference[128];
	register int	i;

	memset(reference, 0, 128);
	Src2 = Src1 + 64;

	for(i=Taille;i>0;i-=128) {
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		extract8(Src2, Ptr);
		Src2 += 4;
		Ptr  += 4;
		extract8(Src1, Ptr);
		Src1 += 4;
		Ptr  += 4;
		
		Src1 += 64;
		Src2 += 64;
		
		if (memcmp(reference, Ptr-128, 128)==0)
			*usage_ptr++ = 0;
		else
			*usage_ptr++ = 1;
	}
}


//----------------------------------------------------------------------------

#define COPY_BIT(a, b) { \
    a <<= 1; \
    a |= (b & 0x01); \
    b >>= 1; }

void extract8(char *src, char *dst) 
{ 
   int i;

   unsigned char bh = *src++;
   unsigned char bl = *src++;
   unsigned char ch = *src++;
   unsigned char cl = *src; 
   unsigned char al, ah; 

   for(i = 0; i < 4; i++)
   { 
      al = ah = 0; 

      COPY_BIT(al, ch) 
      COPY_BIT(al, cl) 
      COPY_BIT(al, bh) 
      COPY_BIT(al, bl) 

      COPY_BIT(ah, ch) 
      COPY_BIT(ah, cl) 
      COPY_BIT(ah, bh) 
      COPY_BIT(ah, bl) 

      *dst++ = ((ah << 4) | al);
   } 
} 


//----------------------------------------------------------------------------
/*
void spr_conv(unsigned char *src, unsigned char *dst, int len, unsigned char *usage_ptr)
{
    register int    i;
    int offset;

    for(i=0;i<len;i+=4) {
        if((i&0x7f)<64)
            offset=(i&0xfff80)+((i&0x7f)<<1)+4;
        else
            offset=(i&0xfff80)+((i&0x7f)<<1)-128;

        extract8(src,dst+offset);
        src+=4;
    }
}
*/
//----------------------------------------------------------------------------
void neogeo_upload(void)
{

    int        		Zone;
    int        		Taille;
    int        		Banque;
    int        		Offset = 0;
    unsigned char    	*Source;
    unsigned char    	*Dest;
    // FILE            *fp;
 
    printf ("upload\n");
    Zone = m68k_read_memory_8(0x10FEDA);


    switch( Zone&0x0F )
    {
    case    0:    // PRG

        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        Dest = neogeo_prg_memory + m68k_read_memory_32(0x10FEF4);
        Taille = m68k_read_memory_32(0x10FEFC);

        memcpy(Dest, Source, Taille);

        m68k_write_memory_32( 0x10FEF4, m68k_read_memory_32(0x10FEF4) + Taille );

        break;

    case    2:    // SPR
        Banque = m68k_read_memory_8(0x10FEDB);
        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        Offset = m68k_read_memory_32(0x10FEF4) + (Banque<<20);
        Dest = neogeo_spr_memory + Offset;
        Taille = m68k_read_memory_32(0x10FEFC);
        
        do {
            memset(cdrom_buffer, 0, BUFFER_SIZE);
            swab(Source, cdrom_buffer, min(BUFFER_SIZE, Taille));
            spr_conv(cdrom_buffer, Dest, min(BUFFER_SIZE, Taille), 
                video_spr_usage + (Offset>>7));
            Source += min(BUFFER_SIZE, Taille);
            Dest += min(BUFFER_SIZE, Taille);
            Offset += min(BUFFER_SIZE, Taille);
            Taille -= min(BUFFER_SIZE, Taille);
        } while(Taille!=0);
        
        
        // Mise à jour des valeurs
        Offset = m68k_read_memory_32( 0x10FEF4 );
        Banque = m68k_read_memory_8( 0x10FEDB );
        Taille = m68k_read_memory_8( 0x10FEFC );
        
        Offset += Taille;
        
        while (Offset > 0x100000 )
        {
            Banque++;
            Offset -= 0x100000;
        }
        
        m68k_write_memory_32( 0x10FEF4, Offset );
        m68k_write_memory_16( 0x10FEDB, Banque );
        
        break;

    case    1:    // FIX
        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        Offset = m68k_read_memory_32(0x10FEF4)>>1;
        Dest = neogeo_fix_memory + Offset;
        Taille = m68k_read_memory_32(0x10FEFC);

        do {
            memset(cdrom_buffer, 0, BUFFER_SIZE);
            swab(Source, cdrom_buffer, min(BUFFER_SIZE, Taille));
            fix_conv(cdrom_buffer, Dest, min(BUFFER_SIZE, Taille), 
                video_fix_usage + (Offset>>5));
            Source += min(BUFFER_SIZE, Taille);
            Dest += min(BUFFER_SIZE, Taille);
            Offset += min(BUFFER_SIZE, Taille);
            Taille -= min(BUFFER_SIZE, Taille);
        } while(Taille!=0);
        
        Offset = m68k_read_memory_32( 0x10FEF4 );
        Taille = m68k_read_memory_32( 0x10FEFC );
        
        Offset += (Taille<<1);
        
        m68k_write_memory_32( 0x10FEF4, Offset);
        
        break;

    case    3:    // Z80

        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
    	Dest = subcpu_memspace + (m68k_read_memory_32(0x10FEF4)>>1);
        Taille = m68k_read_memory_32(0x10FEFC);
        
        swab( Source, Dest, Taille);        

        m68k_write_memory_32( 0x10FEF4, m68k_read_memory_32(0x10FEF4) + (Taille<<1) );

        break;        

    case    5:    // Z80 patch

        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        cdrom_apply_patch((short*)Source, m68k_read_memory_32(0x10FEF4), m68k_read_memory_8(0x10FEDB));

        break;
    
    case    4:    // PCM
        Banque = m68k_read_memory_8(0x10FEDB);
        Source = neogeo_prg_memory + m68k_read_memory_32(0x10FEF8);
        Offset = (m68k_read_memory_32(0x10FEF4)>>1) + (Banque<<19);
        Dest = neogeo_pcm_memory + Offset;
        Taille = m68k_read_memory_32(0x10FEFC);
        
        swab( Source, Dest, Taille);        
        
        // Mise à jour des valeurs
        Offset = m68k_read_memory_32( 0x10FEF4 );
        Banque = m68k_read_memory_8( 0x10FEDB );
        Taille = m68k_read_memory_8( 0x10FEFC );
        
        Offset += (Taille<<1);
        
        while (Offset > 0x100000 )
        {
            Banque++;
            Offset -= 0x100000;
        }
        
        m68k_write_memory_32( 0x10FEF4, Offset );
        m68k_write_memory_16( 0x10FEDB, Banque );

        break;    
    
    }

}

//----------------------------------------------------------------------------
void cdrom_load_title(void)
{
    
    char            jue[4] = JUE;
    char            file[12] = TITLE_X_SYS;
    int	            fd;
    char            *Ptr;
    int             Readed;
    int             x, y;

    printf("loading title\n");
    
    if (game_boot_mode == BOOT_CD)
    	strcpy(Path, cdpath);
    else
        strcpy(Path, path_prefix);
    
    if (game_boot_mode == BOOT_HOST)
          strcat(Path, "CD\\");
    
    file[6] = jue[m68k_read_memory_8(0x10FD83)&3];
    strcat(Path, file);
    
    // if games loaded from CD and from "cd" directory
    /*if ((game_boot_mode == BOOT_HOST)  && (boot_mode = BOOT_CD))
    	strcat(Path, ";1");
    */
    fd = fioOpen(Path, O_RDONLY);
    if (fd<0)
    {
    	printf ("cannot find any TITLE_X.SYS\n");
    	return;
    }
    
    fioRead(fd, video_paletteram_pc, 0x5A0);
    swab((char *)video_paletteram_pc, (char *)video_paletteram_pc, 0x5A0);

    for(Readed=0;Readed<720;Readed++)
        video_paletteram_pc[Readed] = video_color_lut[video_paletteram_pc[Readed]];

    Ptr = neogeo_spr_memory;
    
    Readed = fioRead(fd, cdrom_buffer, BUFFER_SIZE);
    spr_conv(cdrom_buffer, Ptr, Readed, video_spr_usage);
    fioClose(fd);

    Readed = 0;
    for(y=0;y<80;y+=16)
    {
        for(x=0;x<144;x+=16)
        {
            video_draw_spr(Readed, Readed, 0, 0, x+16, y+16, 15, 16);
            Readed++;
        }
    }

    blitter();

    memset(neogeo_spr_memory, 0, 4194304);
    memset(neogeo_fix_memory, 0, 131072);
    memset(video_spr_usage, 0, 32768);
    memset(video_fix_usage, 0, 4096);

}

#define PATCH_Z80(a, b) { \
	                    subcpu_memspace[(a)] = (b)&0xFF; \
                            subcpu_memspace[(a+1)] = ((b)>>8)&0xFF; \
                        }

void cdrom_apply_patch(short *source, int offset, int bank)
{
    int master_offset;
    
    master_offset = (((bank*1048576) + offset)/256)&0xFFFF;
    
    while(*source != 0)
    {
        PATCH_Z80( source[0], ((source[1] + master_offset)>>1) );
        PATCH_Z80( source[0] + 2, (((source[2] + master_offset)>>1) - 1) );
        
        if ((source[3])&&(source[4]))
        {
            PATCH_Z80( source[0] + 5, ((source[3] + master_offset)>>1) );
            PATCH_Z80( source[0] + 7, (((source[4] + master_offset)>>1) - 1) );
        }
            
        source += 5;
    }
}
