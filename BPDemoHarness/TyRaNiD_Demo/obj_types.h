#ifndef __OBJ_TYPES_H__
#define __OBJ_TYPES_H__

typedef struct
{
  u32 v_count;
  vertex v[1];  /* Arrau of vertices */
} obj_vertex_t;

typedef struct
{
  u32 n_count;
  vertex n[1];
} obj_normal_t;

typedef struct
{
  u32 t_count;
  vertex t[1];
} obj_tex_t;

typedef struct
{
  u32 v[3];
  u32 n[3];
  u32 t[3];
} obj_face_t;

typedef struct
{
  u32 size;
  u32 pos;
} obj_strip_t;

#define VTYPE_V    0
#define VTYPE_VN   1
#define VTYPE_VNT  2
#define VTYPE_VNTC 3
typedef struct
{
  u32 s_count;
  u32 v_type;
  obj_strip_t str[]; 
} obj_vustrips_t;

typedef struct
{
  u32 f_count;
  u32 v_type;
  obj_face_t f[1];
} obj_vufaces_t;

#endif
