#include <tamtypes.h>
#include <stdio.h>
#include <string.h>
#include "gs.h"
#include "prim.h"
#include "vu1.h"
#include "../harness.h"
#include "obj_types.h"
#include "matrix.h"
#include "vu_code.h"

extern vertex geo0_data[];
extern obj_vustrips_t geo0_strips;

#define Z_NEAR 10
#define Z_FAR 20000
#define X_LEFT -20
#define X_RIGHT 20
#define Y_TOP 15
#define Y_BOT -15

const demo_init_t *init;

u32 vucode_size;

void apply_matrix_vu(const vertex *m, vertex *v, s32 num) 

     /* Apply a matrix to a list of points */

{
  float t[4];
  int loop;

  for(loop = 0; loop < 4; loop++)
    {
      t[loop] = m[loop].x * v->x + m[loop].y * v->y + m[loop].z * v->z + m[loop].w * v->w;
    }

  memcpy(v, t, sizeof(vertex));
}

vertex m[4] __attribute__((aligned(128)));
vertex p __attribute__((aligned(128))) = {10, 10, -10, 1};

vu_gp_t vu_gp __attribute__((aligned(128)));
vu_renderpoints_t vu_data __attribute__((aligned(128)));

#define GIF_TAG(NLOOP,EOP,PRE,PRIM,FLG,NREG) \
		((u64)(NLOOP)<< 0) | \
		((u64)(EOP)	<< 15)| \
		((u64)(PRE)	<< 46)| \
		((u64)(PRIM)	<< 47)| \
		((u64)(FLG)	<< 58)| \
		((u64)(NREG)	<< 60)

void init_vudata()

{
  vertex *v;

  matrix_build_identity(vu_gp.o2w);
  matrix_add_trans(vu_gp.o2w, 0, 0, -100);
  matrix_projection(vu_gp.proj, X_RIGHT, X_LEFT, Y_TOP, Y_BOT, Z_NEAR, Z_FAR);
  vu_gp.scr_size.x = 320;
  vu_gp.scr_size.y = 112;
  vu_gp.scr_size.z = -2000000;
  vu_gp.scr_size.w = 0;
  vu_gp.scr_offs.x = 1024 + 320;
  vu_gp.scr_offs.y = 1024 + 112;
  vu_gp.scr_offs.z = 2000000;
  vu_gp.scr_offs.w = 0;
  vu_gp.giftag[0] = GIF_TAG(0, 0, 1, (4 | (1 << 3)), 0, 2);
  vu_gp.giftag[1] = 0x51;
  vu_gp.gifend[0] = GIF_TAG(0, 1, 0, 0, 0, 0);
  vu_gp.gifend[1] = 0;
  vu_gp.col[0] = 0xFF;
  vu_gp.col[1] = 0xFF;
  vu_gp.col[2] = 0xFF;
  vu_gp.light.x = 0;
  vu_gp.light.y = 0;
  vu_gp.light.z = 1;
  vu_gp.light.w = 0;

  vu_data.num = 36;
  v = (vertex *) vu_data.points;

  v[0].x = 0;
  v[0].y = 0;
  v[0].z = 0;
  v[0].w = 1;

  v[3].x = 50;
  v[3].y = 0;
  v[3].z = 0;
  v[3].w = 1;
}

typedef struct 

{
  vertex o2w[4];
  u32 rotx, roty, rotz;
  vertex trans;
  vertex *v;
  obj_vustrips_t *strips;
} scrobj_t;

void update_obj(scrobj_t *obj)

{
  vertex m1[4];
  vertex m2[4];
  vertex m3[4];
  /* Rebuild o2w matrix */
  
  matrix_build_rotx(m2, obj->rotx);
  matrix_build_roty(m3, obj->roty);
  matrix_multiply(m1, m2, m3);
  matrix_build_rotz(m2, obj->rotz);
  matrix_add_trans(m2, obj->trans.x, obj->trans.y, obj->trans.z);
  matrix_multiply(obj->o2w, m1, m2);
/*   matrix_print(obj->o2w); */
}

void render_obj(scrobj_t *obj)

{
  int loop;
  u32 count[4];

  vu1_upload_data_copy(obj->o2w, 0, 4, DB_OFF);
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
scrobj_t g_box1;
scrobj_t g_box2;
scrobj_t g_box3;
scrobj_t g_box4;
scrobj_t g_box5;

void init_objs()

{
  g_box1.rotx = 0;
  g_box1.roty = 0;
  g_box1.rotz = 0;
  g_box1.trans.x = 0;
  g_box1.trans.y = 0;
  g_box1.trans.z = -100;
  g_box1.v = box0_data;
  g_box1.strips = &box0_strips;

  update_obj(&g_box1);

  g_box2.rotx = 0;
  g_box2.roty = 0;
  g_box2.rotz = 0;
  g_box2.trans.x = 20;
  g_box2.trans.y = 0;
  g_box2.trans.z = -100;
  g_box2.v = box0_data;
  g_box2.strips = &box0_strips;

  update_obj(&g_box2);

  g_box3.rotx = 0;
  g_box3.roty = 0;
  g_box3.rotz = 0;
  g_box3.trans.x = 0;
  g_box3.trans.y = -20;
  g_box3.trans.z = -100;
  g_box3.v = box0_data;
  g_box3.strips = &box0_strips;

  update_obj(&g_box3);

  g_box4.rotx = 0;
  g_box4.roty = 0;
  g_box4.rotz = 0;
  g_box4.trans.x = 0;
  g_box4.trans.y = 20;
  g_box4.trans.z = -100;
  g_box4.v = box0_data;
  g_box4.strips = &box0_strips;

  update_obj(&g_box4);

  g_box5.rotx = 0;
  g_box5.roty = 0;
  g_box5.rotz = 0;
  g_box5.trans.x = -20;
  g_box5.trans.y = 0;
  g_box5.trans.z = -100;
  g_box5.v = box0_data;
  g_box5.strips = &box0_strips;

  update_obj(&g_box5);
}

void init_demo()

{
  vucode_size = GET_VUADDR(vucodeend);
  init_gs();
  init_vu1();
  vu1_setup_db(32, 496);
  
  init_vudata(); 

  vu1_upload_code(&vucodebegin, 0, vucode_size); /* Always upload code */
  vu1_send_chain();
  vu1_wait_dma();
  init_objs();
}

void do_demo()

{
   int fb = 0;
   int angle;
   int fcount = 0;

   angle = 0;
   
   set_active_fb(fb);
   clr_scr(0);
   
   set_visible_fb(fb);
   fb ^= 1;
   set_active_fb(fb);
   
   while(init->frame_count > 0)
     {
       set_zbufcmp(1);
       clr_scr(0); 
       set_zbufcmp(3);
       
       angle = (angle + 2) & 1023;
       vu1_upload_data_copy((vertex *) &vu_gp, 0, sizeof(vu_gp_t) / 16, DB_OFF);
       g_box1.rotx = angle;
       g_box1.roty = angle;
       g_box1.rotz = angle;
       update_obj(&g_box1);
       render_obj(&g_box1);

       g_box2.rotx = angle;
       g_box2.rotz = angle;
       update_obj(&g_box2);
       render_obj(&g_box2);

       g_box3.rotx = angle;
       update_obj(&g_box3);
       render_obj(&g_box3);

       g_box4.rotz = angle;
       update_obj(&g_box4);
       render_obj(&g_box4);

       g_box5.roty = angle;
       update_obj(&g_box5);
       render_obj(&g_box5);

       vu1_send_chain();
       vu1_wait_dma();
       
       fcount++;
       
       wait_vsync();
       set_visible_fb(fb);
       fb ^= 1;
       set_active_fb(fb);
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
