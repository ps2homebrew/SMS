/**************************************
****   CDAUDIO.H  -  CD-DA Player  ****
****         Header File           ****
**************************************/

#ifndef	CDAUDIO_H
#define CDAUDIO_H

//-- Exported Variables ------------------------------------------------------
extern int	cdda_first_drive;
extern int	cdda_nb_of_drives;
extern int	cdda_current_drive;
extern int	cdda_current_track;
extern int	cdda_playing;
extern char	drive_list[32];
extern int	nb_of_drives;
extern int	cdda_autoloop;

//-- Exported Functions ------------------------------------------------------
int	cdda_init(void);
int	cdda_play(int);
void	cdda_pause(void);
void	cdda_stop(void);
void	cdda_resume(void);
void	cdda_shutdown(void);
void	cdda_loop_check(void);
int 	cdda_get_disk_info(void);
void	cdda_build_drive_list(void);
int	cdda_get_volume(void);
void	cdda_set_volume(int volume);
void 	audio_setup(void);

#endif /* CDAUDIO_H */

