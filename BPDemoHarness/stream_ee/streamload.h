/*

streamload.h - EE RPC functions for streamload IRX

Copyright (c) 2004 adresd <adresd_ps2dev@yahoo.com>

*/

#ifndef _STREAMLOAD_H
#define _STREAMLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#define STREAMLOAD_IRX        0x5707755
#define STREAMLOAD_TUNE		0x01
#define STREAMLOAD_INIT		0x02
#define STREAMLOAD_PLAY		0x03
#define STREAMLOAD_PAUSE	0x04
#define STREAMLOAD_QUIT		0x05
#define STREAMLOAD_GETPOS  	0x06
#define STREAMLOAD_SETPOS  	0x07


int StreamLoad_Init(int cdmode,char *partitionname);
void StreamLoad_Close();
void StreamLoad_SetupTune(char *pathname);
void StreamLoad_Play(unsigned int volume);
void StreamLoad_Pause();
int StreamLoad_Position();
void StreamLoad_SetPosition(int position);

#ifdef __cplusplus
}
#endif

#endif // _STREAMLOAD_H
