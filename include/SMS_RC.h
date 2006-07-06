/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_RC_H
# define __SMS_RC_H

# define RC_1           0x00D9
# define RC_PREV        0x03DB
# define RC_SELECT      0x05DF
# define RC_SLOW_LEFT   0x06DF
# define RC_2           0x10D9
# define RC_NEXT        0x13DB
# define RC_L3          0x15DF
# define RC_SLOW_RIGHT  0x16DF
# define RC_3           0x20D9
# define RC_PLAY        0x23DB
# define RC_R3          0x25DF
# define RC_4           0x30D9
# define RC_SCAN_LEFT   0x33DB
# define RC_START       0x35DF
# define RC_SUBTITLE    0x36DF
# define RC_5           0x40D9
# define RC_SCAN_RIGHT  0x43DB
# define RC_DISPLAY     0x45DD
# define RC_TOP         0x45DF
# define RC_AUDIO       0x46DF
# define RC_6           0x50D9
# define RC_RESET       0x51DB
# define RC_SHUFFLE     0x53DB
# define RC_RIGHT       0x55DF
# define RC_ANGLE       0x56DF
# define RC_7           0x60D9
# define RC_OPEN_CLOSE  0x61DB
# define RC_BOTTOM      0x65DF
# define RC_8           0x70D9
# define RC_LEFT        0x75DF
# define RC_9           0x80D9
# define RC_TIME        0x82DB
# define RC_STOP        0x83DB
# define RC_L2          0x85DF
# define RC_0           0x90D9
# define RC_PAUSE       0x93DB
# define RC_R2          0x95DF
# define RC_TOPX        0x97DF
# define RC_TOP_MENU    0xA1D9
# define RC_A_B         0xA2DB
# define RC_L1          0xA5DF
# define RC_BOTTOMX     0xA7DF
# define RC_ENTER       0xB0D9
# define RC_MENU        0xB1D9
# define RC_R1          0xB5DF
# define RC_LEFTX       0xB7DF
# define RC_REPEAT      0xC2DB
# define RC_TRIANGLE    0xC5DF
# define RC_RIGHTX      0xC7DF
# define RC_CIRCLE      0xD5DF
# define RC_RETURN      0xE0D9
# define RC_CROSS       0xE5DF
# define RC_CLEAR       0xF0D9
# define RC_PROGRAM     0xF1D9
# define RC_SQUARE      0xF5DF

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

int RC_Load  ( void );
int RCX_Load ( void );

int RC_Start  ( void );
int RCX_Start ( void );

int RC_Shutdown  ( void );
int RCX_Shutdown ( void );

int RC_Open  ( int  );
int RCX_Open ( void );

int RC_Close  ( int  );
int RCX_Close ( void );

unsigned int (   *RC_SetTranslator (  unsigned int ( * ) ( unsigned int )  )   ) ( unsigned int );

extern unsigned int ( *RC_Read ) ( void );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_RC_H */
