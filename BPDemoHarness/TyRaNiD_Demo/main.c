#include <tamtypes.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <PbScreen.h>
#include <PbGs.h>
#include <PbMatrix.h>
#include <PbTexture.h>
#include <PbPrim.h>
#include "shapes.h"
#include "vu1.h"
#include "../harness.h"
#include "obj_types.h"
#include "matrix.h"
#include "vu_code.h"

#define PROJ_DIV 5.0f
#define Z_NEAR 10.0f
#define Z_FAR 40000.0f
#define X_LEFT -Z_NEAR/PROJ_DIV
#define X_RIGHT Z_NEAR/PROJ_DIV
#define Y_TOP (Z_NEAR * 0.75f)/PROJ_DIV
#define Y_BOT -(Z_NEAR * 0.75f)/PROJ_DIV

#define VUADDR(x, y) ((&(x) - &(y)) >> 4)
const demo_init_t *init;

u32 vucode_size;

vu_gp_t vu_gp __attribute__((aligned(128)));

void init_vudata()

{
  matrix_build_identity(vu_gp.w2v);
  //matrix_add_trans(vu_gp.o2w, 0, 0, -100);
  matrix_projection(vu_gp.proj, X_RIGHT, X_LEFT, Y_TOP, Y_BOT, Z_NEAR, Z_FAR);
  vu_gp.scr_size.x = SCR_W / 2;
  vu_gp.scr_size.y = SCR_H / 2;
  vu_gp.scr_size.z = -20000000;
  vu_gp.scr_size.w = 0;
  vu_gp.scr_offs.x = PbScreenGetOffsetX() + SCR_W / 2;
  vu_gp.scr_offs.y = PbScreenGetOffsetY() + SCR_H / 2;
  vu_gp.scr_offs.z = 20000000;
  vu_gp.scr_offs.w = 0;
  vu_gp.giftag[0] = GIF_TAG(0, 0, 1, (4 | (1 << 3)), 0, 2);
  //vu_gp.giftag[0] = GIF_TAG(0, 0, 1, 0, 0, 2);
  //vu_gp.giftag[0] = GIF_TAG(0, 0, 1, 4, 0, 2);
  vu_gp.giftag[1] = 0x51;
  vu_gp.gifend[0] = GIF_TAG(0, 1, 0, 0, 0, 0);
  vu_gp.gifend[1] = 0;
  vu_gp.col[0] = 0xFF;
  vu_gp.col[1] = 0xFF;
  vu_gp.col[2] = 0xFF;
  vu_gp.col[3] = 0x80;
  vu_gp.light.x = 0;
  vu_gp.light.y = 0;
  vu_gp.light.z = 1;
  vu_gp.light.w = 0;
}

typedef struct 

{
  vertex o2w[4];
  u32 rotx, roty, rotz;
  float tx, ty, tz;
  float sx, sy, sz;
  vertex *v;
  obj_vustrips_t *strips;
  float colour[4];
  u8 dummy[4];
} scrobj_t;

typedef struct

{
   float tx, ty, tz;
   u32 rotx, roty, rotz;
} camera_t;

void update_camera(camera_t *c)

{
  vertex m1[4] __attribute__((aligned(128)));
  vertex m2[4] __attribute__((aligned(128)));
  vertex m3[4] __attribute__((aligned(128)));

  matrix_build_rotx(m2, c->rotx);
  matrix_build_roty(m3, c->roty);
  matrix_multiply(m1, m2, m3);
  matrix_build_rotz(m2, c->rotz);
  matrix_add_trans(m2, c->tx, c->ty, c->tz);
  matrix_multiply(vu_gp.w2v, m1, m2);
}

void update_obj(scrobj_t *obj)

{
  vertex m1[4] __attribute__((aligned(128)));
  vertex m2[4] __attribute__((aligned(128)));
  vertex m3[4] __attribute__((aligned(128)));
  /* Rebuild o2w matrix */
  
  matrix_build_scale(m2, obj->sx, obj->sy, obj->sz);
  matrix_build_rotx(m3, obj->rotx);
  matrix_multiply(m1, m2, m3);
  matrix_build_roty(m2, obj->roty);
  matrix_multiply(m3, m1, m2);
  matrix_build_rotz(m1, obj->rotz);
  matrix_add_trans(m1, obj->tx, obj->ty, obj->tz);
  matrix_multiply(obj->o2w, m3, m1);
}

void render_obj(scrobj_t *obj)

{
  int loop;
  u32 count[4];

  vu1_upload_data_copy(obj->o2w, 0, 4, DB_OFF);
  vu1_upload_data_copy((vertex *) &obj->colour, 16, 1, DB_OFF);
  for(loop = 0; loop < obj->strips->s_count; loop++)
    {
      count[0] = obj->strips->str[loop].size / 3;
      vu1_upload_data_copy((vertex *) count, 0, 1, DB_ON);
      vu1_upload_data(&obj->v[obj->strips->str[loop].pos], 1, obj->strips->str[loop].size, DB_ON);
      vu1_exec_code(GET_VUADDR(vu_renderstrips), EXEC_MSCAL);
    }
}

extern vertex box0_data[];
extern obj_vustrips_t box0_strips;
extern u32 logo[];

#define BOX_X 16
#define BOX_Y 16
#define BOX_SIZE 50.0f
#define BOX_Z 100.0f

void invert_alpha(void *data)

{
   int loop;
   u8 *buf = (u8 *) data;
   u8 val;

   for(loop = 0; loop < (512 * 256); loop++)
   {
     val = buf[loop * 4 + 3];
     val = 0xFF - val;
     buf[loop * 4 + 3] = val;
   }
}

scrobj_t g_boxes[BOX_X * BOX_Y] __attribute__((aligned(128)));
camera_t g_camera __attribute__((aligned(128)));
PbTexture *p_logo = NULL;

void init_objs()

{
  int loopx, loopy;
  float offsetx, offsety;

  g_camera.rotx = 0;
  g_camera.roty = 0;
  g_camera.rotz = 0;
  g_camera.tx = 0.0f;
  g_camera.ty = 0.0f;
  g_camera.tz = -10000.0f;

  offsetx = (BOX_SIZE + 10.0f) * 2.0f * ((float) (BOX_X / 2));
  offsety = (BOX_SIZE + 10.0f) * 2.0f * ((float) (BOX_Y / 2));

  for(loopy = 0; loopy < BOX_Y; loopy++)
  {
    for(loopx = 0; loopx < BOX_X; loopx++)
    {
      g_boxes[(loopy * BOX_X) + loopx].rotx = 0;
      g_boxes[(loopy * BOX_X) + loopx].roty = 0;
      g_boxes[(loopy * BOX_X) + loopx].rotz = 0;
      g_boxes[(loopy * BOX_X) + loopx].tx = (loopx * 120.0f) - offsetx;
      g_boxes[(loopy * BOX_X) + loopx].ty = (loopy * 120.0f) - offsety;
      g_boxes[(loopy * BOX_X) + loopx].tz = -BOX_Z;
      g_boxes[(loopy * BOX_X) + loopx].sx = 1.0f;
      g_boxes[(loopy * BOX_X) + loopx].sy = 1.0f;
      g_boxes[(loopy * BOX_X) + loopx].sz = 1.0f;
      g_boxes[(loopy * BOX_X) + loopx].v = box0_data;
      g_boxes[(loopy * BOX_X) + loopx].strips = &box0_strips;
      //g_boxes[(loopy * BOX_X) + loopx].colour[0] = loopx * loopy;
      g_boxes[(loopy * BOX_X) + loopx].colour[0] = 0xFF;
      g_boxes[(loopy * BOX_X) + loopx].colour[1] = 0;
      g_boxes[(loopy * BOX_X) + loopx].colour[2] = 0;
      g_boxes[(loopy * BOX_X) + loopx].colour[3] = 128.0f;
      update_obj(&g_boxes[(loopy * BOX_X) + loopx]);
    }
  }

  invert_alpha(logo);
  FlushCache(0);
  p_logo = PbTextureCreate32(logo, 512, 256);
  PbTextureUpload(p_logo); 
}

void init_demo()

{
  vucode_size = GET_VUADDR(vucodeend);
  PbScreenSetup(SCR_W, SCR_H, SCR_PSM);
  init_vu1();
  vu1_setup_db(32, 496);
  
  init_vudata(); 

  vu1_upload_code(&vucodebegin, 0, vucode_size); /* Always upload code */
  vu1_send_chain();
  vu1_wait_dma();
  init_objs();
  PbPrimSetState( PB_ALPHA_BLENDING, PB_ENABLE);
  PbPrimSetAlpha(0, 1, 2, 2, 0x80);
}

void do_demo()

{
   int angle;
   int c_angle;
   int fcount = 0;
   int loop;

   angle = 0;
   c_angle = 0;
   
   PbScreenClear(0);
   PbScreenSyncFlip();
   
   while(init->frame_count > 0)
     {
       *((u32 *) 0x120000e0) = 0xFF00;
       PbPrimSetState( PB_ALPHA_BLENDING, PB_DISABLE);
       PbScreenClear(0);
       
       angle = (angle + 4) & 1023;
       c_angle = (c_angle + 1) & 1023;
       g_camera.roty = c_angle;
       g_camera.rotx = c_angle;
       g_camera.rotz = c_angle;
       update_camera(&g_camera);
       vu1_upload_data_copy((vertex *) &vu_gp, 0, sizeof(vu_gp_t) / 16, DB_OFF);

       for(loop = 0; loop < (BOX_X * BOX_Y); loop++)
       {
          g_boxes[loop].rotx = angle;
          g_boxes[loop].roty = angle;
          g_boxes[loop].rotz = angle;
          update_obj(&g_boxes[loop]);
          render_obj(&g_boxes[loop]);
       }
       vu1_add_flush();
       vu1_send_chain();
       vu1_wait_dma();
       PbPrimSetState( PB_ALPHA_BLENDING, PB_ENABLE);
       PbPrimSpriteTexture(p_logo, 50 << 4, 60 << 4, 0 << 4, 0 << 4,
			562 << 4, 176 << 4, 512 << 4, 116 << 4, 0xFFFFFFFF, 0x80808080);
       
       *((u32 *) 0x120000e0) = 0xFF;
       fcount++;
       
       PbScreenSyncFlip();
     }
   
   init->printf("fcount %d\n", fcount);
}

u32 start_demo(const demo_init_t *t)

{
  init = t;

  init_demo();
  do_demo();
  
  return t->screen_mode;
}
