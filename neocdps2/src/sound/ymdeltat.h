#ifndef __YMDELTAT_H_
#define __YMDELTAT_H_

#include "../neocd.h"

#define YM_DELTAT_SHIFT    (16)

/* adpcm type A and type B struct */
typedef struct deltat_adpcm_state {
    uint8 *memory;
    int memory_size;
    double freqbase;
    int32 *output_pointer;	/* pointer of output pointers */
    int output_range;

    uint8 reg[16];
    uint8 portstate, portcontrol;
    int portshift;

    uint8 flag;			/* port state        */
    uint8 flagMask;		/* arrived flag mask */
    uint8 now_data;
    uint32 now_addr;
    uint32 now_step;
    uint32 step;
    uint32 start;
    uint32 end;
    uint32 delta;
    int32 volume;
    int32 *pan;			/* &output_pointer[pan] */
    int32 /*adpcmm, */ adpcmx, adpcmd;
    int32 adpcml;		/* hiro-shi!! */

    /* leveling and re-sampling state for DELTA-T */
    int32 volume_w_step;	/* volume with step rate */
    int32 next_leveling;	/* leveling value        */
    int32 sample_step;		/* step of re-sampling   */

    uint8 arrivedFlag;		/* flag of arrived end address */
} YM_DELTAT;

/* static state */
extern uint8 *ym_deltat_memory;	/* memory pointer */

/* before YM_DELTAT_ADPCM_CALC(YM_DELTAT *DELTAT); */
#define YM_DELTAT_DECODE_PRESET(DELTAT) {ym_deltat_memory = DELTAT->memory;}

void YM_DELTAT_ADPCM_Write(YM_DELTAT * DELTAT, int r, int v);
void YM_DELTAT_ADPCM_Reset(YM_DELTAT * DELTAT, int pan);


/* ---------- inline block ---------- */

/* DELTA-T particle adjuster */
#define YM_DELTAT_DELTA_MAX (24576)
#define YM_DELTAT_DELTA_MIN (127)
#define YM_DELTAT_DELTA_DEF (127)

#define YM_DELTAT_DECODE_RANGE 32768
#define YM_DELTAT_DECODE_MIN (-(YM_DELTAT_DECODE_RANGE))
#define YM_DELTAT_DECODE_MAX ((YM_DELTAT_DECODE_RANGE)-1)

extern const int32 ym_deltat_decode_tableB1[];
extern const int32 ym_deltat_decode_tableB2[];

#define YM_DELTAT_Limit(val,max,min)	\
{					\
	if ( val > max ) val = max;	\
	else if ( val < min ) val = min;\
}

/**** ADPCM B (Delta-T control type) ****/
inline void YM_DELTAT_ADPCM_CALC(YM_DELTAT * DELTAT);

/* INLINE void YM_DELTAT_ADPCM_CALC(YM_DELTAT *DELTAT); */
//#define YM_INLINE_BLOCK
//#include "ymdeltat.c" /* include inline function section */
//#undef  YM_INLINE_BLOCK

void YM_DELTAT_postload(YM_DELTAT *DELTAT,uint8 *regs);
void YM_DELTAT_savestate(YM_DELTAT *DELTAT);

#endif
