#include <tamtypes.h>
#include <string.h>
#include "../harness.h"
#include "shapes.h"

extern float sin_tab[];
extern float cos_tab[];

#define CLEAR_M(m) { int _i; float *_t = (float *) m; for(_i=0;_i<16;_i++) { _t[_i] = 0.0f; } } 

extern demo_init_t *init;

void matrix_build_identity(vertex *m)

{
  CLEAR_M(m)
  m[0].x = 1.0f;
  m[1].y = 1.0f;
  m[2].z = 1.0f;
  m[3].w = 1.0f;
}

void matrix_build_rotx(vertex *m, int angle)

{
  matrix_build_identity(m);
  m[1].y = cos_tab[angle];
  m[1].z = sin_tab[angle];
  m[2].y = -sin_tab[angle];
  m[2].z = cos_tab[angle];
}

void matrix_build_roty(vertex *m, int angle)

{
  matrix_build_identity(m);
  m[0].x = cos_tab[angle];
  m[0].z = -sin_tab[angle];
  m[2].x = sin_tab[angle];
  m[2].z = cos_tab[angle];
}

void matrix_build_rotz(vertex *m, int angle)

{
  matrix_build_identity(m);
  m[0].x = cos_tab[angle];
  m[0].y = sin_tab[angle];
  m[1].x = -sin_tab[angle];
  m[1].y = cos_tab[angle];
}

void matrix_build_scale(vertex *m, float sx, float sy, float sz)

{
  CLEAR_M(m);
  m[0].x = sx;
  m[1].y = sy;
  m[2].z = sz;
  m[3].w = 1;
}

void matrix_add_trans(vertex *m, float tx, float ty, float tz)

{
  m[3].x = tx;
  m[3].y = ty;
  m[3].z = tz;
}

void matrix_projection(vertex *m, float r, float l, float t, float b, float n, float f)

{
  float div1, div2, div3;

  div1 = 1.0f / (r - l);
  div2 = 1.0f / (t - b);
  div3 = 1.0f / (f - n);

  CLEAR_M(m)
/*   m[0].x = 2 * n * div1; */
/*   m[0].z = (r + l) * div1; */
/*   m[1].y = 2 * n * div2; */
/*   m[1].z = (t + b) * div2; */
/*   m[2].z = -((f + n) * div3); */
/*   m[2].w = -(2 * n * f * div3); */
/*   m[3].z = -1; */
    m[0].x = 2 * n * div1;
  m[2].x = (r + l) * div1;
  m[1].y = 2 * n * div2;
  m[2].y = (t + b) * div2;
  m[2].z = -((f+n) * div3);
  m[3].z = -(2 * n * f * div3);
  m[2].w = -1;
}

void matrix_print(const vertex *m)

{
  int loop;

  for(loop = 0; loop < 4; loop++)
    {
      init->printf("Row %d : %f\t %f\t %f\t %f\n", loop, m[loop].x, m[loop].y, m[loop].z, m[loop].w);
    }
}

void matrix_transpose(vertex *m)

{
  vertex t[4];

  t[0].x = m[0].x;
  t[0].y = m[1].x;
  t[0].z = m[2].x;
  t[0].w = m[3].x;
  t[1].x = m[0].y;
  t[1].y = m[1].y;
  t[1].z = m[2].y;
  t[1].w = m[3].y;
  t[2].x = m[0].z;
  t[2].y = m[1].z;
  t[2].z = m[2].z;
  t[2].w = m[3].z;
  t[3].x = m[0].w;
  t[3].y = m[1].w;
  t[3].z = m[2].w;
  t[3].w = m[3].w;

  memcpy(m, t, sizeof(vertex) * 4);
}

void matrix_multiply( vertex* pDest,const vertex* pSrc1,const vertex* pSrc2 )
{
	// taken from jar's "slave of the vu" code

	asm __volatile__(
		"lqc2            vf16,0x00(%1)\n"
		"lqc2            vf17,0x10(%1)\n"
		"lqc2            vf18,0x20(%1)\n"
		"lqc2            vf19,0x30(%1)\n"
		"lqc2            vf20,0x00(%2)\n"
		"lqc2            vf21,0x10(%2)\n"
		"lqc2            vf22,0x20(%2)\n"
		"lqc2            vf23,0x30(%2)\n"
		"vmulax.xyzw     ACC,vf20,vf16\n"
		"vmadday.xyzw    ACC,vf21,vf16\n"
		"vmaddaz.xyzw    ACC,vf22,vf16\n"
		"vmaddw.xyzw     vf16,vf23,vf16\n"
		"vmulax.xyzw     ACC,vf20,vf17\n"
		"vmadday.xyzw    ACC,vf21,vf17\n"
		"vmaddaz.xyzw    ACC,vf22,vf17\n"
		"vmaddw.xyzw     vf17,vf23,vf17\n"
		"vmulax.xyzw     ACC,vf20,vf18\n"
		"vmadday.xyzw    ACC,vf21,vf18\n"
		"vmaddaz.xyzw    ACC,vf22,vf18\n"
		"vmaddw.xyzw     vf18,vf23,vf18\n"
		"vmulax.xyzw     ACC,vf20,vf19\n"
		"vmadday.xyzw    ACC,vf21,vf19\n"
		"vmaddaz.xyzw    ACC,vf22,vf19\n"
		"vmaddw.xyzw     vf19,vf23,vf19\n"
		"sqc2            vf16,0x00(%0)\n"
		"sqc2            vf17,0x10(%0)\n"
		"sqc2            vf18,0x20(%0)\n"
		"sqc2            vf19,0x30(%0)\n"
		: : "r"(pDest), "r"(pSrc1), "r"(pSrc2) : "memory");
}

