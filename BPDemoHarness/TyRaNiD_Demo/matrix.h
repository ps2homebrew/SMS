#ifndef __MATRIX_H__
#define __MATRIX_H__

void matrix_build_identity(vertex *m);
void matrix_build_rotx(vertex *m, int angle);
void matrix_build_roty(vertex *m, int angle);
void matrix_build_rotz(vertex *m, int angle);
void matrix_add_trans(vertex *m, float tx, float ty, float tz);
void matrix_projection(vertex *m, float r, float l, float t, float b, float n, float f);
void matrix_print(const vertex *m);
void matrix_multiply( vertex* pDest,const vertex* pSrc1,const vertex* pSrc2 );
void matrix_transpose(vertex *m);
void matrix_build_scale(vertex *m, float sx, float sy, float sz);

#endif
