/**************************************
****   INPUT.H  -  Input devices   ****
****         Header File           ****
**************************************/

#ifndef	INPUT_H
#define INPUT_H

//#include <SDL.h>

/*--------------------------------------------------------------------------*/
#define P1UP    0x00000001
#define P1DOWN  0x00000002
#define P1LEFT  0x00000004
#define P1RIGHT 0x00000008
#define P1A     0x00000010
#define P1B     0x00000020
#define P1C     0x00000040
#define P1D     0x00000080

#define P2UP    0x00000100
#define P2DOWN  0x00000200
#define P2LEFT  0x00000400
#define P2RIGHT 0x00000800
#define P2A     0x00001000
#define P2B     0x00002000
#define P2C     0x00004000
#define P2D     0x00008000

#define P1START 0x00010000
#define P1SEL   0x00020000
#define P2START 0x00040000
#define P2SEL   0x00080000

#define SPECIAL 0x01000000

#define PADDLE_1	0
#define PADDLE_2	1


/*-- input.c functions -----------------------------------------------------*/
void input_init(void);
void processEvents(void);
void enterBIOS(void);

inline unsigned char read_player1(void);
inline unsigned char read_player2(void);
inline unsigned char read_pl12_startsel(void);

/* UI function */
void waitforX(void);
int isButtonPressed(uint32 button);


#endif /* INPUT_H */
