#ifndef __VU_CODE_H__
#define __VU_CODE_H__

extern u64 vucodebegin;
extern u64 vucodeend;
extern u64 vu_buildmatrix;
extern u64 vu_mulpoint;
extern u64 vu_renderpoints;
extern u64 vu_renderstrips;

#define GET_VUADDR(x) ((((u32) &(x)) - ((u32) &vucodebegin)) >> 3)

typedef struct

{
  vertex o2w[4];
  vertex w2v[4];
  vertex proj[4];
  vertex scr_size;
  vertex scr_offs;
  u64 giftag[2];
  u64 gifend[2];
  float col[4];
  vertex light;
} vu_gp_t;

typedef struct
{
  u32 num;
  u32 dummy[3];
  struct
  {
    vertex point;
    float col[4];
  } points[100];
} vu_renderpoints_t;

#endif
