/*
  File: fm.h -- header file for software emuration for FM sound genelator

*/
#ifndef _H_FM_FM_
#define _H_FM_FM_

//#include "../defines.h"
#include "ymdeltat.h"
/* --- select emulation chips --- */

#define BUILD_YM2610  1		//(HAS_YM2610)  /* build YM2610(OPNB)  emulator */
#define BUILD_YM2608  0

/* --- system optimize --- */
/* select stereo output buffer : 1=mixing / 0=separate */
#define FM_STEREO_MIX 0
/* select bit size of output : 8 or 16 */
#define FM_OUTPUT_BIT 16
/* select timer system internal or external */
#define FM_INTERNAL_TIMER 0

/* --- speedup optimize --- */
/* support LFO unit */
#define FM_LFO_SUPPORT 0
/* support OPN SSG type envelope mode */
#define FM_SEG_SUPPORT 0

/* --- external SSG(YM2149/AY-3-8910)emulator interface port */
/* used by YM2203,YM2608,and YM2610 */

/* SSGClk   : Set SSG Clock      */
/* int n    = chip number        */
/* int clk  = MasterClock(Hz)    */
/* int rate = sample rate(Hz) */
#define SSGClk(/*chip,*/clock) AY8910_set_clock(/*chip,*/clock)

/* SSGWrite : Write SSG port     */
/* int n    = chip number        */
/* int a    = address            */
/* int v    = data               */
#define SSGWrite(/*n,*/a,v) AY8910Write(/*n,*/a,v)

/* SSGRead  : Read SSG port */
/* int n    = chip number   */
/* return   = Read data     */
#define SSGRead(/*n*/) AY8910Read(/*n*/)

/* SSGReset : Reset SSG chip */
/* int n    = chip number   */
#define SSGReset(/*chip*/) AY8910_reset(/*chip*/)

/* --- external callback funstions for realtime update --- */
#if BUILD_YM2610
  /* in 2610intf.c */
//#define YM2610UpdateReq(/*chip*/) YM2610UpdateRequest(/*chip*/);
#define YM2610UpdateReq()
#endif

#if FM_STEREO_MIX
#define YM2151_NUMBUF 1
#define YM2608_NUMBUF 1
#define YM2612_NUMBUF 1
#define YM2610_NUMBUF 1
#else
#define YM2151_NUMBUF 2		/* FM L+R */
#define YM2608_NUMBUF 2		/* FM L+R+ADPCM+RYTHM */
#define YM2610_NUMBUF 2		/* FM L+R+ADPCMA+ADPCMB */
#define YM2612_NUMBUF 2		/* FM L+R */
#endif

#if (FM_OUTPUT_BIT==16)
typedef short FMSAMPLE;
typedef unsigned long FMSAMPLE_MIX;
#endif
#if (FM_OUTPUT_BIT==8)
typedef unsigned char FMSAMPLE;
typedef unsigned short FMSAMPLE_MIX;
#endif

typedef void (*FM_TIMERHANDLER) (int c, int cnt, float stepTime);
typedef void (*FM_IRQHANDLER) (int n, int irq);
/* FM_TIMERHANDLER : Stop or Start timer         */
/* int n          = chip number                  */
/* int c          = Channel 0=TimerA,1=TimerB    */
/* int count      = timer count (0=stop)         */
/* doube stepTime = step time of one count (sec.)*/

/* FM_IRQHHANDLER : IRQ level changing sense     */
/* int n       = chip number                     */
/* int irq     = IRQ level 0=OFF,1=ON            */

/* ---------- OPN / OPM one channel  ---------- */
typedef struct fm_slot {
    int32 *DT;			/* detune          :DT_TABLE[DT]       */
    int DT2;			/* multiple,Detune2:(DT2<<4)|ML for OPM */
    int TL;			/* total level     :TL << 8            */
    uint8 KSR;			/* key scale rate  :3-KSR              */
    const int32 *AR;		/* attack rate     :&AR_TABLE[AR<<1]   */
    const int32 *DR;		/* decay rate      :&DR_TABLE[DR<<1]   */
    const int32 *SR;		/* sustin rate     :&DR_TABLE[SR<<1]   */
    int SL;			/* sustin level    :SL_TABLE[SL]       */
    const int32 *RR;		/* release rate    :&DR_TABLE[RR<<2+2] */
    uint8 SEG;			/* SSG EG type     :SSGEG              */
    uint8 ksr;			/* key scale rate  :kcode>>(3-KSR)     */
    uint32 mul;			/* multiple        :ML_TABLE[ML]       */
    /* Phase Generator */
    uint32 Cnt;			/* frequency count :                   */
    uint32 Incr;		/* frequency step  :                   */
    /* Envelope Generator */
    uint8 state;
    void (*eg_next) (struct fm_slot * SLOT);	/* pointer of phase handler */
    int32 evc;			/* envelope counter                    */
    int32 eve;			/* envelope counter end point          */
    int32 evs;			/* envelope counter step               */
    int32 evsa;			/* envelope step for Attack            */
    int32 evsd;			/* envelope step for Decay             */
    int32 evss;			/* envelope step for Sustain           */
    int32 evsr;			/* envelope step for Release           */
    int32 TLL;			/* adjusted TotalLevel                 */
    /* LFO */
    uint8 amon;			/* AMS enable flag              */
    uint32 ams;			/* AMS depth level of this SLOT */
} FM_SLOT;

typedef struct fm_chan {
    FM_SLOT SLOT[4];
    uint8 PAN;			/* PAN :NONE,LEFT,RIGHT or CENTER */
    uint8 ALGO;			/* Algorythm                      */
    uint8 FB;			/* shift count of self feed back  */
    int32 op1_out[2];		/* op1 output for beedback        */
    /* Algorythm (connection) */
    int32 *connect1;		/* pointer of SLOT1 output    */
    int32 *connect2;		/* pointer of SLOT2 output    */
    int32 *connect3;		/* pointer of SLOT3 output    */
    int32 *connect4;		/* pointer of SLOT4 output    */
    /* LFO */
    int32 pms;			/* PMS depth level of channel */
    uint32 ams;			/* AMS depth level of channel */
    /* Phase Generator */
    uint32 fc;			/* fnum,blk    :adjusted to sampling rate */
    uint8 fn_h;			/* freq latch  :                   */
    uint8 kcode;		/* key code    :                   */
} FM_CH;

/* OPN/OPM common state */
typedef struct fm_state {
    uint8 index;		/* chip index (number of chip) */
    int clock;			/* master clock  (Hz)  */
    int rate;			/* sampling rate (Hz)  */
    double freqbase;		/* frequency base      */
    double TimerBase;		/* Timer base time     */
    uint8 address;		/* address register    */
    uint8 irq;			/* interrupt level     */
    uint8 irqmask;		/* irq mask            */
    uint8 status;		/* status flag         */
    uint32 mode;		/* mode  CSM / 3SLOT   */
    uint8 prescaler_sel;        /* prescaler selector	*/
    uint8 fn_h;			/* freq latch			*/
    int TA;			/* timer a             */
    int TAC;			/* timer a counter     */
    uint8 TB;			/* timer b             */
    int TBC;			/* timer b counter     */
    /* speedup customize */
    /* local time tables */
    int32 DT_TABLE[8][32];	/* DeTune tables       */
    int32 AR_TABLE[94];		/* Atttack rate tables */
    int32 DR_TABLE[94];		/* Decay rate tables   */
    /* Extention Timer and IRQ handler */
    FM_TIMERHANDLER Timer_Handler;
    FM_IRQHANDLER IRQ_Handler;
    /* timer model single / interval */
    uint8 timermodel;
} FM_ST;

/***********************************************************/
/* OPN unit                                                */
/***********************************************************/

/* OPN 3slot struct */
typedef struct opn_3slot {
    uint32 fc[3];		/* fnum3,blk3  :calcrated */
    uint8 fn_h[3];		/* freq3 latch            */
    uint8 kcode[3];		/* key code    :          */
} FM_3SLOT;

/* OPN/A/B common state */
typedef struct opn_f {
    uint8 type;			/* chip type         */
    FM_ST ST;			/* general state     */
    FM_3SLOT SL3;		/* 3 slot mode state */
    FM_CH *P_CH;		/* pointer of CH     */
    uint32 FN_TABLE[2048];	/* fnumber -> increment counter */
#if FM_LFO_SUPPORT
    /* LFO */
    uint32 LFOCnt;
    uint32 LFOIncr;
    uint32 LFO_FREQ[8];		/* LFO FREQ table */
#endif
} FM_OPN;


/* adpcm type A struct */
typedef struct adpcm_state {
    uint8 flag;			/* port state        */
    uint8 flagMask;		/* arrived flag mask */
    uint8 now_data;
    uint32 now_addr;
    uint32 now_step;
    uint32 step;
    uint32 start;
    uint32 end;
    int IL;
    int volume;			/* calcrated mixing level */
    int32 *pan;			/* &out_ch[OPN_xxxx] */
    int /*adpcmm, */ adpcmx, adpcmd;
    int adpcml;			/* hiro-shi!! */
} ADPCM_CH;

/* here's the virtual YM2610 */
typedef struct ym2610_f {
    uint8  REGS[512];           /* registers */
    FM_OPN OPN;			/* OPN state    */
    FM_CH CH[6];		/* channel state */
    int address1;		/* address register1 */
    /* ADPCM-A unit */
    uint8 *pcmbuf;		/* pcm rom buffer */
    uint32 pcm_size;		/* size of pcm rom */
    int32 *adpcmTL;		/* adpcmA total level */
    ADPCM_CH adpcm[6];		/* adpcm channels */
    uint32 adpcmreg[0x30];	/* registers */
    uint8 adpcm_arrivedEndAddress;
    /* Delta-T ADPCM unit */
    YM_DELTAT deltaT;
} YM2610;

extern YM2610 FM2610;

#if (BUILD_YM2610||BUILD_YM2610B)
/* -------------------- YM2610(OPNB) Interface -------------------- */
int YM2610Init(int baseclock, int rate,
	       void *pcmroma, int pcmasize, void *pcmromb, int pcmbsize,
	       FM_TIMERHANDLER TimerHandler, FM_IRQHANDLER IRQHandler);
void YM2610Shutdown(void);
void YM2610ResetChip(void);
void YM2610UpdateOne(int16 **buffer, int length);
//void YM2610UpdateOne(int num, FMSAMPLE * bufL, FMSAMPLE * bufR, int length);

int YM2610Write(int a, unsigned char v);
unsigned char YM2610Read(int a);
int YM2610TimerOver(int c);

#endif				/* BUILD_YM2610 */



#endif				/* _H_FM_FM_ */
