/*
 * PbVu0m.h - VU0 macro functions for Pb demo library
 *
 * Copyright (c) 2004   adresd <adresd_ps2dev@yahoo.com>
 * with some minor changes by emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBVU0M_H_
#define _PBVU0M_H_

#include "PbVec.h"
#include "PbMatrix.h"

// Note: These should all probably be static inline in a header
#define ASINLINE static __inline__ 

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mMatrixApply
//
//  Apply Matrix to a Vector
//
//  v0 = Output Vector
//  v1 = Input Vector
//  m0 = Input Matrix

ASINLINE void PbVu0mMatrixApply(PbFvec *v0, const PbFvec *v1,const PbMatrix *m0)
{
    asm __volatile__(
        "lqc2            vf20,0x00(%2)\n"
        "lqc2            vf16,0x00(%1)\n"
        "lqc2            vf17,0x10(%1)\n"
        "lqc2            vf18,0x20(%1)\n"
        "lqc2            vf19,0x30(%1)\n"
        "vmulax.xyzw     ACC,vf16,vf20\n"
        "vmadday.xyzw    ACC,vf17,vf20\n"
        "vmaddaz.xyzw    ACC,vf18,vf20\n"
        "vmaddw.xyzw     vf20,vf19,vf20\n"
        "sqc2            vf20,0x00(%0)\n"
        : /* No Output */
        : "r"(v0), "r"(m0), "r"(v1)
        : "memory"
    );
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mMatrixApplyN
//  Mtx apply and store result as integer values
//  v0    =   destination pointer
//            0x00(v0) = vert   (128)
//            0x10(v0) = next entry
//  v1    =   source pointer
//            0x00(v1) = vert
//            0x10(v1) = next entry
//  m0    =   local -> screen matrix
//  n     =   number to transform
//  Basic version

ASINLINE void PbVu0mMatrixApplyN(PbFvec *v0, const PbFvec *v1,const PbMatrix *m0, int n)
{
  __asm__ (
"; This loads the transformation matrix\n"
  "lqc2			vf01,0x00(%0)\n"
  "lqc2			vf02,0x10(%0)\n"
  "lqc2			vf03,0x20(%0)\n"
  "lqc2			vf04,0x30(%0)\n"
  : /* No Output */
  : "r"(m0)
  );

  if (n == 0)
    return;

  __asm__ (
"1:\n"
";  TRANSFORM\n"
";\n"
"; This loads the input vector and does the matrix mult (vertices)\n"
"; load input vertex (XYZ)\n"

  "lqc2			vf05,0x00(%1)\n"
  "vmulax.xyzw		ACC,vf01,vf05\n"
  "vmadday.xyzw	ACC,vf02,vf05\n"
  "vmaddaz.xyzw	ACC,vf03,vf05\n"
  "vmaddw.xyzw		vf06,vf04,vf05\n"

";  PERSPECTIVE DIVIDE\n"
";\n"
"; This does the foreshortening (div/z)\n"
"; divides the 'w' field of vf0 (1.0) by the 'w' field of vf06 and puts result in Q\n"

  "vdiv    Q,vf0w,vf06w\n"
  "vwaitq\n"

"; multiplies all parts of vf06 by Q (does the perspective thing)\n"

  "vmulq.xyz	vf06,vf06,Q\n"

"; Convert to integer terms, ready for storing\n"

  "vftoi4.xyzw	vf07,vf06\n"

"; output the vertex value (XYZ)\n"

  "sqc2			vf07,0x00(%0)\n"

"; This does the looping bit\n"

  "addiu			%0,%0,0x10\n"
  "addiu			%1,%1,0x10\n" 
  ".set	push\n"
  ".set	noreorder\n"
  "addiu			%2,%2,-1\n"
  "bne				$0,%2,1b\n"  

"; the delay slot\n" 

  "nop\n"
  "nop\n"
  ".set	pop\n"
  : /* No Output */
  : "r"(v0), "r"(v1), "r"(n)
  : "memory"
  );
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mMatrixIdentity
// Set given matrix to identity
// m0 = Dest

ASINLINE void PbVu0mMatrixIdentity(PbMatrix *m0)
{
    asm __volatile__(
        "vmr32.xyzw  vf18,vf00\n"
        "sqc2        vf00,0x30(%0)\n"
        "vmr32.xyzw  vf17,vf18\n"
        "sqc2        vf18,0x20(%0)\n"
        "vmr32.xyzw  vf16,vf17\n"
        "sqc2        vf17,0x10(%0)\n"
        "sqc2        vf16,0x00(%0)\n"
        : /* No Output */
        : "r"(m0)
        : "memory"
    );
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mMatrixClear
// Set given matrix to all 0's
// m0 = Dest

ASINLINE void PbVu0mMatrixClear(PbMatrix *m0)
{
    asm __volatile__(
        "sqc2        vf00,0x30(%0)\n"
        "sqc2        vf00,0x20(%0)\n"
        "sqc2        vf00,0x10(%0)\n"
        "sqc2        vf00,0x00(%0)\n"
        : /* No Output */
        : "r"(m0)
        : "memory"
    );
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mMatrixMultiply
//  Multiply Matrix
//
//  m0 = Output Matrix
//  m1 = Input Matrix
//  m2 = Input Matrix 2

ASINLINE void PbVu0mMatrixMultiply(PbMatrix *m0, const PbMatrix *m1, const PbMatrix *m2)
{
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
        : /* No Output */
        : "r"(m0), "r"(m1), "r"(m2)
        : "memory"
    );
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorFTOI4
// Convert float vector to INT4 format
//
// v0 = Dest
// v1 = Src

ASINLINE void PbVu0mVectorFTOI4(PbFvec *v0, const PbFvec *v1)
{
    asm __volatile__(
        "lqc2            vf01,0x00(%1)\n"
        "vftoi4.xyzw     vf01, vf01\n" 
        "sqc2            vf01,0x00(%0)\n" 
        : /* No Output */
        : "r"(v0), "r"(v1)
        : "memory"
    );
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorFTOI0
// Convert float vector to INT0 format
// v0 = Dest
// v1 = Src

ASINLINE void PbVu0mVectorFTOI0(PbFvec *v0, const PbFvec *v1)
{
    asm __volatile__(
        "lqc2            vf01,0x00(%1)\n"
        "vftoi0.xyzw     vf01, vf01\n" 
        "sqc2            vf01,0x00(%0)\n" 
        : /* No Output */
        : "r"(v0), "r"(v1)
        : "memory"
    );
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorLength
//  Vector Length
//  v0 = Src
//  Returns length of vector as a float

ASINLINE float PbVu0mVectorLength(const PbFvec *v0)
{
	union { float f; int i; } q;
	__asm__ __volatile__(
		"lqc2		vf01,0(%1)\n"
		"vmul.xyz	vf02,vf01,vf01\n"
		"vmulax.w	ACC,vf00,vf02\n"
		"vmadday.w	ACC,vf00,vf02\n"
		"vmaddz.w	vf02,vf00,vf02\n"
		"vsqrt		Q,vf02w\n"
		"vwaitq\n"
		"cfc2		%0,$vi22\n"
		: "=r"(q.i)
		: "r"(v0)
		);
	return q.f;
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorNormalize
//  Normalize Vector
//  v0 = Dest
//  v1 = Src

ASINLINE void PbVu0mVectorNormalize(PbFvec *v0, const PbFvec *v1)
{
	__asm__ (
		"lqc2		vf01,0x0(%1)\n"
		"vmul.xyz	vf02,vf01,vf01\n"
		"vmulax.w	ACC,vf00,vf02\n"
		"vmadday.w	ACC,vf00,vf02\n"
		"vmaddz.w	vf02,vf00,vf02\n"
		"vrsqrt		Q,vf00w,vf02w\n"
		"vsub.w		vf01,vf00,vf00\n"
		"vwaitq\n"
		"vmulq.xyz	vf01,vf01,Q\n"
		"sqc2		vf01,0x0(%0)\n"
		: /* No Output */
		: "r"(v0), "r"(v1)
		: "memory"
		);
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorCross
//  Cross Product of two vectors
//  v0 = Dest
//  v1 = Src1
//  v2 = Src2

ASINLINE void PbVu0mVectorCross(PbFvec *v0, const PbFvec *v1, const PbFvec *v2)
{
    asm __volatile__(
        "lqc2            vf16,0x0(%1)\n"
        "lqc2            vf17,0x0(%2)\n"
        "vopmula.xyz     ACC,vf16,vf17\n"
        "vopmsub.xyz     vf17,vf17,vf16\n"
        "vsub.w          vf17,vf00,vf00  # w = 0\n"
        "sqc2            vf17,0x0(%0)\n"
        : /* No Output */
        : "r"(v0), "r"(v1), "r"(v2)
        : "memory"
    );
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorDot
// Dot Product
// v0 = Src1
// v1 = Src2
// returns dot as float value

ASINLINE float PbVu0mVectorDot(const PbFvec *v0, const PbFvec *v1)
{
    float dot;
    asm __volatile__(
        "lqc2        vf16,0x0(%1)\n"
        "lqc2        vf17,0x0(%2)\n"
        "vaddw.x     vf18,vf00,vf00\n"
        "vmul.xyz    vf16,vf16,vf17\n"
        "vmulax.x    ACC,vf18,vf16x\n"
        "vmadday.x   ACC,vf18,vf16y\n"
        "vmaddz.x    vf16,vf18,vf16z\n"
        ".set        noat\n"
        "qmfc2       $1,vf16\n"
        "mtc1        $1,%0\n"
        ".set        at\n"
        : "=f"(dot)
        : "r"(v0), "r"(v1)
        : "$1"
    );
    return dot;
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorInterp
// Interpolate between two vectors
// v0 = Dest
// v1 = Src1
// v2 = Src2
// t  = Interpolation Value

ASINLINE void PbVu0mVectorInterp(PbFvec *v0, const PbFvec *v1, const PbFvec *v2, float t)
{
    asm __volatile__(
        ".set        noat\n"
        "mfc1        $1,%3\n"
        "qmtc2       $1,vf18\n"
        ".set        at\n"
        "lqc2        vf17,0x0(%2)\n"
        "lqc2        vf16,0x0(%1)\n"
        "vsubx.w     vf19,vf00,vf18\n"
        "vmulax.xyz  ACC,vf17,vf18\n"
        "vmaddw.xyz  vf16,vf16,vf19\n"
        "sqc2        vf16,0x0(%0)\n"
        : /* No Output */
        : "r"(v0), "r"(v1), "r"(v2), "f"(t)
        : "memory"
    );
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorClear
// Clear Vector (all fields to 0)
// v0 = Dest

ASINLINE void PbVu0mVectorClear(PbFvec *v0)
{
	__asm__ ("sq $0,(%0)":/* No Output */ : "r"(v0): "memory");
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorMult
// Multiply Vectors
// v0 = Dest
// v1 = Src1
// v2 = Src2

ASINLINE void PbVu0mVectorMult(PbFvec *v0, const PbFvec *v1, const PbFvec *v2)
{
	__asm__ (
		"lqc2		vf01,(%1)\n"
		"lqc2		vf02,(%2)\n"
		"vmul.xyzw	vf01,vf01,vf02\n"
		"sqc2		vf01,(%0)\n"
		: /* No Output */
		: "r" (&v0[0]) , "r" (&v1[0]), "r" (&v2[0])
		: "memory"
	);
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorSub
// Subtract Vectors
// v0 = Dest
// v1 = Src1
// v2 = Src2

ASINLINE void PbVu0mVectorSub(PbFvec *v0, const PbFvec *v1, const PbFvec *v2)
{
	__asm__ (
		"lqc2		vf01,(%1)\n"
		"lqc2		vf02,(%2)\n"
		"vsub.xyzw	vf01,vf01,vf02\n"
		"sqc2		vf01,(%0)\n"
		: /* No Output */
		: "r" (v0) , "r" (v1), "r" (v2)
		: "memory"
	);
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorAdd
// Add Vectors
// v0 = Dest
// v1 = Src1
// v2 = Src2

ASINLINE void PbVu0mVectorAdd(PbFvec *v0, const PbFvec *v1, const PbFvec *v2)
{
	__asm__(
		"lqc2		vf01,(%1)\n"
		"lqc2		vf02,(%2)\n"
		"vadd.xyzw	vf01,vf01,vf02\n"
		"sqc2		vf01,(%0)\n"
		: /* No Output */
		: "r" (v0) , "r" (v1), "r" (v2)
		: "memory"
	);
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorCopy
// Copy one Vector to Another
// v0 = Dest
// v1 = Src

ASINLINE void PbVu0mVectorCopy(PbFvec *v0, const PbFvec *v1)
{
	register int t0;
	__asm__ __volatile__("": "=r"(t0));
	__asm__ (
		"lq	%2,0(%1)\n"
		"sq	%2,0(%0)\n"
		: /* No Output */
		: "r"(v0), "r"(v1), "r"(t0)
		);
}

///////////////////////////////////////////////////////////////////////////////
// ASINLINE void PbVu0mVectorLoad
// Load values into Vector
// v0 = Dest
// x,y,z,w = float values for fields

ASINLINE void PbVu0mVectorLoad(PbFvec *v0, float x, float y, float z, float w)
{
	register int t0, t1;
	register union {float f; int i;} _x, _y, _z, _w;
	_x.f = x, _y.f = y, _z.f = z, _w.f = w;
	__asm__ __volatile__("": "=r"(t0), "=r"(t1));
	__asm__ (
		"pextlw		%5,%3,%1\n"
		"pextlw		%6,%4,%2\n"
		"pextlw		%5,%6,%5\n"
		"sq			%5,0(%0)\n"
		: /* No Output */
		: "r"(v0), "r"(_x.i), "r"(_y.i), "r"(_z.i), "r"(_w.i),
			"r"(t0), "r"(t1)
		: "memory"
	);
}


#endif // _PBVU0M_

