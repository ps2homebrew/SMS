/**************************************
****    CDROM.H  -  File reading   ****
****         Header File           ****
**************************************/

#ifndef	CDROM_H
#define CDROM_H

//#include <SDL.h>


/*-- Exported Variables -----------------------------------------------------*/
extern	int	cdrom_current_drive;

/*-- cdrom.c functions ------------------------------------------------------*/
int	cdrom_init1(void);
int	cdrom_load_prg_file(char *, unsigned int);
int	cdrom_load_z80_file(char *, unsigned int);
int	cdrom_load_fix_file(char *, unsigned int);
int	cdrom_load_spr_file(char *, unsigned int);
int	cdrom_load_pcm_file(char *, unsigned int);
int	cdrom_load_pat_file(char *, unsigned int, unsigned int);
int	cdrom_process_ipl(void);
void	cdrom_shutdown(void);
void	cdrom_load_title(void);

void	fix_conv(unsigned char *, unsigned char *, int, unsigned char *);
void	spr_conv(unsigned char *, unsigned char *, int, unsigned char *);

/*-- extract8.asm functions -------------------------------------------------*/
void		extract8(char *, char *);
unsigned int	motorola_peek(unsigned char*);


#endif /* CDROM_H */
