#ifndef __HARNESS_H__
#define __HARNESS_H__

#define SCRMODE_PAL  3
#define SCRMODE_NTSC 2

typedef int (*printf_t)(const char *s, ...);

typedef struct _demo_init
{
  int  (*printf)(const char *s, ...);   /* Pointer to the printf function for debugging,
                   will set to a dummy function in release */
  u16 *(*get_fft)();          /* Gets the current FFT block */
  u32  screen_mode;            /* The PCRTC mode configured */
  volatile u32  curr_frame;    /* The current frame number from start of demo */
  volatile s32  frame_count;   /* Number of frames the demo has */
  volatile float time_count;   /* Number of seconds the demo has floating point */
  volatile s32  time_count_i;  /* Number of seconds the demo has in 16.16 fixed point */
  volatile float curr_time;    /* The current time in floating point from start of demo */
  volatile u32   curr_time_i;  /* The current time in 16.16 fixed point */
  volatile float *sync_points; /* A list of sync points in the current demo time */
  u32   *sync_points_i; /* A list of synx points in the current demo time (fixed point) */
  u32   no_syncs;     /* Number of sync points available */
} demo_init_t;

#define SCR_W 640
#define SCR_H 224
#define SCR_INT 0
#define SCR_FIELD 0
#define SCR_MAGW 4
#define SCR_MAGH 0
#define SCR_PSM 0

#endif
