/**************************************
****   CDAUDIO.C  -  CD-DA Player  ****
**************************************/

//-- Include files -----------------------------------------------------------
#include <string.h>
#include "../neocd.h"
#include "cdaudio.h"


//-- Private Variables -------------------------------------------------------
static int	cdda_min_track;
static int	cdda_max_track;
static int	cdda_disk_length;
static int	cdda_track_start;
static int	cdda_track_end;
static int	cdda_loop_counter;

//-- Public Variables --------------------------------------------------------
int		cdda_first_drive=0;
int		cdda_current_drive=0;
int		cdda_current_track=0;
int		cdda_current_frame=0;
int		cdda_playing=0;
int		cdda_autoloop=0;
int		cdda_volume=0;

//-- Function Prototypes -----------------------------------------------------
int		cdda_init(void);
int		cdda_play(int);
void		cdda_stop(void);
void		cdda_resume(void);
void		cdda_shutdown(void);
void		cdda_loop_check(void);
int 		cdda_get_disk_info(void);

//----------------------------------------------------------------------------
int	cdda_init(void)
{
	
	cdda_min_track = cdda_max_track = 0;
	cdda_current_track = 0;
	cdda_playing = 0;
	cdda_loop_counter = 0;
	
	cdda_disk_length=0;
	cdda_track_start=0;

	// Open the default drive

	return	1;
}

//----------------------------------------------------------------------------
int cdda_get_disk_info(void)
{
    if(!neocdSettings.CDDAOn) return 1;

    // not implemented yet
    return 1;
}


//----------------------------------------------------------------------------
int cdda_play(int track)
{
    if(!neocdSettings.CDDAOn) return 1;
    // not implemented yet
    return 1;
}

//----------------------------------------------------------------------------
void	cdda_pause(void)
{
	// not implemented yet
}


void	cdda_stop(void)
{
	// not implemented yet
}

//----------------------------------------------------------------------------
void	cdda_resume(void)
{
	// not implemented yet
}

//----------------------------------------------------------------------------
void	cdda_shutdown(void)
{
	if(!neocdSettings.CDDAOn) return;
	// not implemented yet
}

//----------------------------------------------------------------------------
void	cdda_loop_check(void)
{
	if (cdda_playing==1) {
		cdda_loop_counter++;
		if (cdda_loop_counter>=cdda_track_end) {
			if (cdda_autoloop)
				cdda_play(cdda_current_track);
			else
				cdda_stop();
		}
	}
}

