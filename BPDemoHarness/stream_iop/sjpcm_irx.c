/*
    ----------------------------------------------------------------------
    sjpcm_irx.c - SjPCM IOP-side code. (c) Nick Van Veen (aka Sjeep), 2002
	----------------------------------------------------------------------

    This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


Streamloading Modifications
Copyright (c) 2004 adresd <adresd_ps2dev@yahoo.com>
FFT Modifications
Copyright (c) 2004 TyRaNiD <tiraniddo@hotmail.com>

*/

#include "types.h"
#include "stdio.h"
#include "sysclib.h"
#include "thbase.h"
#include "intrman.h"
#include "sysmem.h"
#include "sifman.h"
#include "sifcmd.h"
#include "thbase.h"
#include "thsemap.h"
#include "loadcore.h"
#include "libsd.h"

#include "iomanX.h"

IRX_ID("SJPCMSTREAM",1,1)

#if 1 //thbase.h ps2lib
#define AT_THFIFO 0
#define AT_THPRI  1
#define AT_SINGLE 0
#define AT_MULTI  2
#define AT_MSFIFO 0
#define AT_MSPRI  4

#define SA_THFIFO AT_THFIFO
#define SA_THPRI  AT_THPRI
#define SA_IHTHPRI 0x100

#define TH_C		0x02000000
#endif

#define SdSetParam sceSdSetParam
#define SdSetEffectAttr sceSdSetEffectAttr
#define SdSetCoreAttr sceSdSetCoreAttr
#define SdInit sceSdInit
#define SdSetTransCallback sceSdSetTransCallback
#define SdBlockTrans sceSdBlockTrans
#define SdBlockTransStatus sceSdBlockTransStatus

// LIBSD defines
#define SD_CORE_0				0
#define SD_CORE_1				1

#define SD_VP_VOLL      		(0x00<<8)
#define SD_VP_VOLR      		(0x01<<8)
#define SD_VP_PITCH     		(0x02<<8)
#define SD_P_BVOLL			((0x0F<<8)+(0x01<<7))
#define SD_P_BVOLR			((0x10<<8)+(0x01<<7))
#define SD_P_MVOLL			((0x09<<8)+(0x01<<7))
#define SD_P_MVOLR			((0x0A<<8)+(0x01<<7))


#define SD_INIT_COLD			0
#define SD_C_EFFECT_ENABLE    	(1<<1)
#define SD_C_NOISE_CLK			(4<<1)

#define SD_BLOCK_ONESHOT		(0<<4)
#define SD_BLOCK_LOOP			(1<<4)

#define SD_TRANS_MODE_STOP  		2

////////////////
#define SJPCMSTR_IRX			0x5707755
#define SJPCMSTR_TUNE		0x01
#define SJPCMSTR_INIT		0x02
#define SJPCMSTR_PLAY		0x03
#define SJPCMSTR_PAUSE		0x04
#define SJPCMSTR_QUIT		0x05
#define SJPCMSTR_GETPOS	  	0x06
#define SJPCMSTR_SETPOS	  	0x07
#define SJPCMSTR_GETFFT		0x08

#define TH_C		0x02000000

SifRpcDataQueue_t qd;
SifRpcServerData_t sd0;

void SjPCM_Thread(void* param);
void SjPCM_PlayThread(void* param);
static int SjPCM_TransCallback(void* param);

void* SjPCM_rpc_server(unsigned int fno, void *data, int size);
void* SjPCM_Puts(char* s);
void* SjPCM_Init(unsigned int* sbuff);
void* SjPCM_Enqueue(unsigned int* sbuff);
void* SjPCM_Play();
void* SjPCM_Pause();
void* SjPCM_Setvol(unsigned int* sbuff);
void* SjPCM_Clearbuff();
void* SjPCM_Available(unsigned int* sbuff);
void* SjPCM_Buffered(unsigned int* sbuff);
void* SjPCM_Quit();


extern void wmemcpy(void *dest, void *src, int numwords);

void *streamload_init(char *filenamebase);
void streamload_check(void);
void *streamload_getpos(unsigned int* sbuff);
void *streamload_setpos(unsigned int* sbuff);
void *streamload_getfft(unsigned int* sbuff);
void streamload_updatefft();

unsigned int buffer[0x80];

int memoryAllocated = 0;
char *pcmbufl = NULL;
char *pcmbufr = NULL;
char *spubuf = NULL;

int readpos = 0;
int writepos = 0;
int playing = 0;

int leftfd = 0;
int rightfd = 0;
int fftfd = 0;
int streamsetup = 0;
int streamload_playpos = 0;

int volume = 0x3fff;

int transfer_sema = 0;
int play_tid = 0;

int intr_state;

int SyncFlag;
int PalMode;
int FrameSize;
int CurrFrame;
#define NTSC_FRAMESIZE 800
#define PAL_FRAMESIZE 960
#define FFT_BLOCKSIZE (sizeof(u16) * 1024)
u16 fft_data[2][1024] __attribute__((aligned(64)));
u32 curr_fft;

int _start ()
{
  iop_thread_t param;
  int th;

  printf("SjPCMSTR\n");

  FlushDcache();

  CpuEnableIntr(0);
  CpuEnableIntr(36);	// Enables SPU DMA (channel 0) interrupt.
  CpuEnableIntr(40);	// Enables SPU DMA (channel 1) interrupt.
  CpuEnableIntr(9);	// Enables SPU IRQ interrupt.

  param.attr         = TH_C;
  param.thread       = SjPCM_Thread;
  param.priority 	   = 40;
  param.stacksize    = 0x800;
  param.option       = 0;
  th = CreateThread(&param);
  if (th > 0) {
  	StartThread(th,0);
	return 0;
  }
  else return 1;

}

void SjPCM_Thread(void* param)
{
  printf("SjPCMSTR v1.2 (SJPCM2.1b base by Sjeep) by Adresd.\n");

  printf("SjPCMSTR: RPC Initialize\n");
  SifInitRpc(0);

  SifSetRpcQueue(&qd, GetThreadId());
  SifRegisterRpc(&sd0, SJPCMSTR_IRX, (void *)SjPCM_rpc_server,(void *) &buffer[0],0,0,&qd);
  SifRpcLoop(&qd);
}

void* SjPCM_rpc_server(unsigned int fno, void *data, int size)
{

	switch(fno) {
		case SJPCMSTR_INIT:
			return SjPCM_Init((unsigned int*)data);
		case SJPCMSTR_TUNE:
			return streamload_init((char*)data);
		case SJPCMSTR_PLAY:
			return SjPCM_Play((unsigned int*)data);
		case SJPCMSTR_PAUSE:
			return SjPCM_Pause();
		case SJPCMSTR_QUIT:
			return SjPCM_Quit();
		case SJPCMSTR_GETPOS:
			return streamload_getpos((unsigned int*)data);
		case SJPCMSTR_SETPOS:
			return streamload_setpos((unsigned int*)data);
        	case SJPCMSTR_GETFFT:
			return streamload_getfft((unsigned int*)data);
	}
	return NULL;
}

void* SjPCM_Clearbuff()
{
  CpuSuspendIntr(&intr_state);

  memset(spubuf,0,0x800);
  memset(pcmbufl,0,512*2*20);
  memset(pcmbufr,0,512*2*20);

  readpos = writepos = 0;
  streamload_playpos = 0;

  CpuResumeIntr(intr_state);
	
  return NULL;
}

void* SjPCM_Play(unsigned int *sbuff)
{
  volume = sbuff[0] & 0x3fff;
  printf("Play, vol : 0x%X\n",volume);
  // SPU2 CORE0 BVOL to be set to 0 on both sides
  SdSetParam(SD_CORE_1|SD_P_BVOLL,volume);
  SdSetParam(SD_CORE_1|SD_P_BVOLR,volume);
  playing = 1;
  return NULL;
}

void* SjPCM_Pause()
{
  // SPU2 CORE0 BVOL to be set to 0 on both sides
  SdSetParam(SD_CORE_1|SD_P_BVOLL,0);
  SdSetParam(SD_CORE_1|SD_P_BVOLR,0);
  playing = 0;
  return NULL;
}

void* SjPCM_Init(unsigned int* sbuff)
{
  iop_sema_t sema;
  iop_thread_t play_thread;
  char *partitionname = ((char *)sbuff)+12;

  SyncFlag = sbuff[0];		
  PalMode = sbuff[2];

  printf("PalMode %d\n", PalMode);

  sema.attr = SA_THFIFO;
  sema.initial = 0;
  sema.max = 1;
  transfer_sema= CreateSema(&sema);
  if(transfer_sema <= 0) 
  {
    printf("SjPCMSTR: Failed to create semaphore!\n");
    ExitDeleteThread();
  }

  // mount the given partition, as pfs3:/
  {
    int rv;
    // Attempt to mount our new filesystem to "pfs0:"
    rv = mount("pfs3:", partitionname, FIO_MT_RDONLY,0,0);
    if(rv < 0)
    {
      printf("ERROR: failed to mount filesystem: %d\n", rv);
      ExitDeleteThread();
    } else printf("Mounted '%s' as 'pfs3:'\n",partitionname);
  }

  // Allocate memory
  if(!memoryAllocated)
  {
    pcmbufl = AllocSysMemory(0,512*2*20,NULL);
    if(pcmbufl == NULL) 
    {
      printf("SjPCMSTR: Failed to allocate memory for sound buffer!\n");
      ExitDeleteThread();
    }
    pcmbufr = AllocSysMemory(0,512*2*20,NULL);
    if(pcmbufr == NULL) 
    {
      printf("SjPCMSTR: Failed to allocate memory for sound buffer!\n");
      ExitDeleteThread();
    }
    spubuf = AllocSysMemory(0,0x800,NULL);
    if(spubuf == NULL) 
    {
      printf("SjPCMSTR: Failed to allocate memory for sound buffer!\n");
      ExitDeleteThread();
    }

    printf("SjPCMSTR: Memory Allocated. %d bytes left.\n",QueryTotalFreeMemSize());
    memoryAllocated = 1;
  }

  memset(pcmbufl,0,512*2*20);
  memset(pcmbufr,0,512*2*20);
  memset(spubuf,0,0x800);

  printf("SjPCMSTR: Sound buffers cleared\n");

  // Initialise SPU
  if(SdInit(SD_INIT_COLD) < 0) 
  {
    printf("SjPCMSTR: Failed to initialise libsd!\n");
    ExitDeleteThread();
  }
  else printf("SjPCMSTR: libsd initialised!\n");

  SdSetCoreAttr(SD_CORE_1|SD_C_NOISE_CLK,0);
  SdSetParam(SD_CORE_1|SD_P_MVOLL,0x3fff);
  SdSetParam(SD_CORE_1|SD_P_MVOLR,0x3fff);
  SdSetParam(SD_CORE_1|SD_P_BVOLL,volume);
  SdSetParam(SD_CORE_1|SD_P_BVOLR,volume);

  SdSetTransCallback(1, (void *)SjPCM_TransCallback);

  // Start audio streaming
  SdBlockTrans(1,SD_BLOCK_LOOP,spubuf, 0x800);

  printf("SjPCMSTR: Setting up playing thread\n");

  // Start playing thread
  play_thread.attr         = TH_C;
  play_thread.thread       = SjPCM_PlayThread;
  play_thread.priority 	 = 39;
  play_thread.stacksize    = 0x800;
  play_thread.option      = 0;
  play_tid = CreateThread(&play_thread);
  if (play_tid > 0) 
    StartThread(play_tid,0);
  else {
    printf("SjPCMSTR: Failed to start playing thread!\n");
    ExitDeleteThread();
  }

  // Return data
  printf("SjPCMSTR: Entering playing thread.\n");
  return sbuff;
}


void SjPCM_PlayThread(void* param)
{
  int which;
  printf("PlayThread: Entering\n");
  while(1) 
  {
    WaitSema(transfer_sema);
    // Interrupts are suspended, instead of using semaphores.
    CpuSuspendIntr(&intr_state);
    which = 1 - (SdBlockTransStatus(1, 0 )>>24);
    wmemcpy(spubuf+(1024*which),pcmbufl+readpos,512);		// left
    wmemcpy(spubuf+(1024*which)+512,pcmbufr+readpos,512);	// right
    if (playing)
    {
      readpos += 512;
      if(readpos >= (512*2*20)) readpos = 0;
      streamload_playpos += 512;
      CurrFrame += 256;
    }
    CpuResumeIntr(intr_state);
    streamload_check();
    if(CurrFrame > FrameSize)
    {
      /* Read next fft block */
      streamload_updatefft();
      if(CurrFrame > 0)
        CurrFrame -= FrameSize;
    }
  }
}

void *streamload_getpos(unsigned int* sbuff)
{
  /* adjust to sample level (16bit) */
  sbuff[3] = streamload_playpos/2;
  return sbuff;
}
void *streamload_setpos(unsigned int* sbuff)
{
  if (streamsetup)
  {
    SjPCM_Clearbuff();
    /* adjust to sample level (16bit) */
    streamload_playpos = sbuff[3]*2;
    if (rightfd > 0) 
      lseek(rightfd,streamload_playpos,SEEK_SET);
    if (leftfd > 0) 
      lseek(leftfd,streamload_playpos,SEEK_SET);
    if(fftfd > 0)
    { 
       int frame;
       frame = streamload_playpos >> 1;
       CurrFrame = frame % FrameSize;
       frame /= FrameSize;
       frame *= FFT_BLOCKSIZE;
       lseek(fftfd, frame, SEEK_SET);
       streamload_updatefft();
    }       
    /* now clear buffer, and force preloading */
    streamload_check();
  }
  return sbuff;
}

void *streamload_getfft(unsigned int* sbuff)
{
   return (void *) (&fft_data[curr_fft][0]);
}

int streamload_buffered(void)
{
  unsigned int rp = readpos, wp = writepos;
  if (wp<rp) wp+=512*2*20;
  return  (wp-rp)/4;
}

void streamload_close()
{
  streamsetup = 0;
  // close existing files
  if (leftfd > 0) close(leftfd);
  if (rightfd > 0) close(rightfd);
  if (fftfd > 0) close(fftfd);
}

void *streamload_init(char *filenamebase)
{
  char pathname[256];
  streamload_close();

  sprintf(pathname,"pfs3:/%sL.RAW",filenamebase);

  // open both the input files
  leftfd = open(pathname,O_RDONLY);
  if (leftfd > 0) printf("SjPCMSTR: left file '%s' opened (%d)\n",pathname,leftfd);

  sprintf(pathname,"pfs3:/%sR.RAW",filenamebase);
  rightfd = open(pathname,O_RDONLY);
  if (rightfd > 0) printf("SjPCMSTR: right file '%s' opened (%d)\n",pathname,rightfd);

  if(PalMode)
    {
      sprintf(pathname, "pfs3:/%sPAL.FFT", filenamebase);
      FrameSize = PAL_FRAMESIZE;
    }
  else
    {
      sprintf(pathname, "pfs3:/%sNTSC.FFT", filenamebase);
      FrameSize = NTSC_FRAMESIZE;
    }

  CurrFrame = 0;
  fftfd = open(pathname, O_RDONLY);
  if(fftfd > 0) 
  {
    streamload_updatefft();
    printf("SjPCMSTR: FFT file '%s' opened (%d)\n", pathname, fftfd);
  }
  else printf("SjPCMSTR: Failed to open FFT file '%s'\n", pathname);
  curr_fft = 1;

  if ((rightfd > 0) && (leftfd > 0))
  {
    streamsetup = 1;
    SjPCM_Clearbuff();
    streamload_check(); // prefill start of buffer
    SdSetParam(SD_CORE_1|SD_P_MVOLL,0x3fff);
    SdSetParam(SD_CORE_1|SD_P_MVOLR,0x3fff);
    SdSetParam(SD_CORE_1|SD_P_BVOLL,0x3fff);
    SdSetParam(SD_CORE_1|SD_P_BVOLR,0x3fff);
    printf("streamload init\n");
  }
  else
  {
    streamsetup = 0;
    printf("SjPCMSTR: Could not open input files (%d,%d)\n", rightfd,leftfd);
  }
  return NULL;
}
#define STREAM_LOWTHRESH  (512*6)
#define STREAM_TRANS      (512*2)
void streamload_check(void)
{
  if (streamsetup)
  {
    int retl,retr;
    while(streamload_buffered() < STREAM_LOWTHRESH)
    { // only do something if we dont have enough data queued
      // load in enough data at writepos
      retl = read(leftfd,pcmbufl+writepos,STREAM_TRANS);
      retr = read(rightfd,pcmbufr+writepos,STREAM_TRANS);
      if ((retl <= 0) || (retr <= 0))
      {
        lseek(leftfd,0,SEEK_SET);
        lseek(rightfd,0,SEEK_SET);
        streamload_playpos = 0;
        lseek(fftfd, 0, SEEK_SET);
        CurrFrame = 0;
        printf("SjPCMSTR: Error reading stream, looping\n");
      }
      else
      {
        // inc writepos pointer, as enqueue does
        writepos += STREAM_TRANS;
        if(writepos >= (512*2*20)) writepos = 0;
//      if(SyncFlag)
//      if(writepos == (512*2*20)) readpos = 0x2400;
      }
    }
  }
}

void streamload_updatefft()
{
   int ret; 
   if(fftfd > 0)
   {
     ret = read(fftfd, fft_data[curr_fft ^ 1], FFT_BLOCKSIZE);
     if(ret <= 0)
     {
        lseek(fftfd, 0, SEEK_SET);
        CurrFrame = 0;
        printf("SjPCMSTR: Error with FFT data. Looping\n");
     }
     curr_fft ^= 1;
   }
}

static int SjPCM_TransCallback(void* param)
{
  iSignalSema(transfer_sema);
  return 1;
}

void* SjPCM_Available(unsigned int* sbuff)
{
  unsigned int rp = readpos, wp = writepos;
  if (wp<rp) wp+=512*2*20;
  sbuff[3] = (512*2*20-(wp-rp))/4;
  return sbuff;
}

void* SjPCM_Buffered(unsigned int* sbuff)
{
  unsigned int rp = readpos, wp = writepos;
  if (wp<rp) wp+=512*2*20;
  sbuff[3] = (wp-rp)/4;
  return sbuff;
}

void* SjPCM_Quit(unsigned int* sbuff)
{
  // SPU2 CORE0 BVOL to be set to 0 on both sides
  SdSetParam(SD_CORE_1|SD_P_BVOLL,0);
  SdSetParam(SD_CORE_1|SD_P_BVOLR,0);

  SdSetTransCallback(1,NULL);
  SdBlockTrans(1,SD_TRANS_MODE_STOP,0,0);

  TerminateThread(play_tid);
  DeleteThread(play_tid);

  DeleteSema(transfer_sema);

  umount("pfs3:");

  return sbuff;
}


