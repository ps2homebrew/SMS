/* Header file to define some basic shapes */

#ifndef __SHAPES_H__
#define __SHAPES_H__

typedef struct

{
  u16 x;   // X co-ordinate of point
  u16 y;   // Y ""
  u32 z;
  u32 col; // Colour of point
  u32 lit; // The amount the ploygon is lit
  u16 u;   // u texture coord
  u16 v;   // v ""
} point;

typedef struct

{
  point v[2];
  u32 col;
} rect;

typedef struct

{
  point v[2]; // Vertex one and two of the line
  u32 col;   // Colour of line when not shading
} line;

typedef struct

{
  point v[3];
  u32 col;   // Colour of triangle when not shading
} triangle;

typedef struct

{
  point v[4];
  u32 col; 
  u32 lit;
} quad;

typedef struct 

{
  point *v;   /* Pointer to an array of points */
  u32 len; /* The length of the strip (in vertexes) */
  u32 col;
} tristrip;

typedef struct

{
  s32 x, y, z, w;
} point_vu;

typedef struct

{
  u32 v[3];
} polygon;

typedef struct 
{
  u32 x, y, z, w;
} vertex_i;

#endif
