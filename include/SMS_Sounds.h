#ifndef __SMS_Sounds_H
# define __SMS_Sounds_H

# ifndef __SPU_H
#  include "SMS_SPU.h"
# endif  /* __SPU_H */

# define SMSound_Error  &g_SMSound[ 0 ]
# define SMSound_PAD    &g_SMSound[ 1 ]
# define SMSound_Mount  &g_SMSound[ 2 ]
# define SMSound_UMount &g_SMSound[ 3 ]

# define g_CWD     (  ( char*          )&g_SMSounds[   512 ]  )
# define g_TOC     (  ( char*          )&g_SMSounds[  1536 ]  )
# define g_SCPaint (  ( unsigned long* )&g_SMSounds[  3600 ]  )
# define g_SCErase (  ( unsigned long* )&g_SMSounds[  5792 ]  )
# define g_VCPaint (  ( unsigned long* )&g_SMSounds[  5920 ]  )
# define g_VCErase (  ( unsigned long* )&g_SMSounds[  6448 ]  )
# define g_Balls   (  ( void*          )&g_SMSounds[  6576 ]  )
# define g_BallPkt (  ( unsigned long* )&g_SMSounds[ 12720 ]  )
# define g_DErase  (  ( unsigned long* )&g_SMSounds[ 29184 ]  )
# define g_DPaint  (  ( unsigned long* )&g_SMSounds[ 29232 ]  )
# define g_OSDNR   (  ( unsigned long* )&g_SMSounds[ 29696 ]  )
# define g_VRStack (  ( unsigned char* )&g_SMSounds[ 30000 ]  )
# define g_ARStack (  ( unsigned char* )&g_SMSounds[ 46384 ]  )

extern SMSound       g_SMSound [     4 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_SMSounds[ 70816 ] __attribute__(   (  aligned( 64 ), section( ".data" )  )   );

#endif  /* __SMS_Sounds_H */
