#include <tamtypes.h>
#include <kernel.h>
#include <fileio.h>
#include <string.h>
#include "harness.h"

#include "stream_ee/streamload.h"
int sound_enabled = 0;

#define BIN_LOADADDR 0x1000000
u8 *loadptr = (u8 *) BIN_LOADADDR;
typedef u32 (*main_func)(demo_init_t *p);
main_func call_main = (main_func) BIN_LOADADDR;
#define MAX_DEMOS 16

typedef struct _demo_entry

{
   char *name;
   u32 demo_time;
} demo_entry_t;

#define TH_STACK_SIZE (8 * 1024)
u8 th_stack[TH_STACK_SIZE];
int vb_id;
int th_id;
int th_sema = -1;
int demo_count = 0;
int demo_starttime = 0;
demo_entry_t demos[MAX_DEMOS];
u32 curr_demotime = 10;
u32 set_noprintf = 0;

typedef float *(*get_fft)();
demo_init_t init;

int init_gs(int scr_mode);
int is_pal();

int vblank_handler(int cause)
{
   iSignalSema(th_sema);
   asm("sync.l ; ei");

   return 0;
}

void enable_vblank()
{
   DI();
   vb_id = AddIntcHandler(2, vblank_handler, 0);
   EnableIntc(2);
   EI();
}

void disable_vblank()
{
   DisableIntc(2);
   RemoveIntcHandler(2, vb_id);
}

int update_thread_pal(void *arg)

{
   for(;;)
   {
      WaitSema(th_sema); 
      init.curr_frame++;
      init.frame_count--;
      init.time_count -= (1.0f / 50.0f);
      init.time_count_i -= (65536 / 50);
      init.curr_time += (1.0f / 50.0f);
      init.curr_time_i += (65536 / 50);
   }
}

int update_thread_ntsc(void *arg)

{
   for(;;)
   {
      WaitSema(th_sema); 
      init.curr_frame++;
      init.frame_count--;
      init.time_count -= (1.0f / 60.0f);
      init.time_count_i -= (65536 / 60);
      init.curr_time += (1.0f / 60.0f);
      init.curr_time_i += (65536 / 60);
   }
}

void create_updateth(int scr_mode)

{
   extern void *_gp;
   ee_thread_t th;
   ee_sema_t sema;
   
   sema.init_count = 0;
   sema.max_count = 1;
   th_sema = CreateSema(&sema);
   if(th_sema < 0)
   {
      printf("Error creating sema\n");
      SleepThread();
   }

   if(scr_mode == SCRMODE_PAL)
   {
      th.func = update_thread_pal;
   }
   else
   {
      th.func = update_thread_ntsc;
   }
   th.stack = th_stack;
   th.stack_size = TH_STACK_SIZE;
   th.gp_reg = _gp;
   th.initial_priority = 42;
   if((th_id = CreateThread(&th)) < 0)
   {
      printf("Error creating thread\n");
      SleepThread();
   }

   StartThread(th_id, NULL);
}
   
int dummy()

{
   return 0;
}
 
void reset_init()

{
   if(set_noprintf)
   {
      init.printf = (printf_t) dummy;
   } 
   else
   {
      init.printf = printf;
   }
   init.get_fft = (get_fft) dummy;
   init.curr_frame = 0;
   init.frame_count = 0;
   init.time_count = 0;
   init.time_count_i = 0;
   init.curr_time = 0;
   init.curr_time_i = 0;
   init.sync_points = NULL;
   init.sync_points_i = NULL;
   init.no_syncs = 0;
}

void print_usage()

{
   printf("Demo Harness Usage:\n");
   printf("harness.elf <options> <demos>\n");
   printf("Options:\n");
   printf("-pal      : Set pal mode\n");
   printf("-ntsc     : Set NTSC mode\n");
   printf("-sound : Enable Sound\n");
   printf("-tX       : Time in seconds to run each demo\n");
   printf("-sX       : Time in seconds to start tune at\n");
   printf("-noprintf : Disables the printf function\n");
   printf("Demos:\n");
   printf("List of host files to run.\n");
}

int process_args(int argc, char **argv)

{ 
   int arg_loop;
   

   if(argc < 2)
   {
      return -1;
   }

   for(arg_loop = 1; arg_loop < argc; arg_loop++)
   {
      if(argv[arg_loop][0] == '-')
      {
         if(strcmp("pal", &argv[arg_loop][1]) == 0)   
         {
            init.screen_mode = SCRMODE_PAL;
            printf("Set PAL mode\n");
         }
         else if(strcmp("ntsc", &argv[arg_loop][1]) == 0)
         {
            init.screen_mode = SCRMODE_NTSC;
            printf("Set NTSC mode\n");
         }
         else if(strcmp("noprintf", &argv[arg_loop][1]) == 0)
         {
            set_noprintf = 1;
            printf("Disabling printf\n");
         }
         else if(strcmp("sound", &argv[arg_loop][1]) == 0)
         {
		sound_enabled = 1;
            printf("Enable Sound\n");
         }
         else if(argv[arg_loop][1] == 's')
         {  
            if((argv[arg_loop][2] >= '0') && (argv[arg_loop][2] <= '9'))
            {
              char *endp;
              demo_starttime = strtol(&argv[arg_loop][2], &endp, 10);
              printf("Set start time %d\n", demo_starttime);
            } 
            else
            {
              printf("Invalid argument %s\n", argv[arg_loop]);
              return -1;
            }
         }
         else if(argv[arg_loop][1] == 't')
         {  
            if((argv[arg_loop][2] >= '0') && (argv[arg_loop][2] <= '9'))
            {
              char *endp;
              curr_demotime = strtol(&argv[arg_loop][2], &endp, 10);
              printf("Set time %d\n", curr_demotime);
            } 
            else
            {
              printf("Invalid argument %s\n", argv[arg_loop]);
              return -1;
            }
         }
         else
         {
            printf("Invalid argument %s\n", argv[arg_loop]);
            return -1;
         }
      }
      else
      {
         if((argv[arg_loop] != 0) && (demo_count < MAX_DEMOS))
         {
            demos[demo_count].demo_time = curr_demotime;
            demos[demo_count++].name = argv[arg_loop];
            printf("Adding %s\n", argv[arg_loop]);
         }
         else
         { 
            printf("Invalid file name %s\n", argv[arg_loop]);
            return -1;
         }
      }
   }
         
   if(demo_count == 0)
   {
      return -1;
   }

   return 0;
}

int main(int argc, char **argv)

{
   int fd;
   int demo_loop;

   if(is_pal())
   {
     init.screen_mode = SCRMODE_PAL;
   }  
   else
   {
     init.screen_mode = SCRMODE_NTSC;
   }

   if(process_args(argc, argv) < 0)
   {
      print_usage();
      SleepThread();
   }

   ResetEE(0xFF);
   init_gs(init.screen_mode);
   reset_init();
   create_updateth(init.screen_mode);
  
   if(sound_enabled)
   {
     StreamLoad_Init(0,"hdd:+PS2MENU");
//     StreamLoad_SetupTune("HALFDEAPH");// hdd:+PS2MENU/HALFDEAPHL.RAW AND HALFDEAPHR.RAW
     StreamLoad_SetupTune("UNSEEN"); // hdd:+PS2MENU/UNSEENL.RAW AND UNSEENR.RAW
     StreamLoad_SetPosition(demo_starttime*48000);
     StreamLoad_Play(0x3fff);
   }

   for(demo_loop = 0; demo_loop < demo_count; demo_loop++)
   { 
     fd = fioOpen(demos[demo_loop].name, O_RDONLY);
     if(fd < 0)
     {
       printf("Couldn't open file %s\n", demos[demo_loop].name);
     }
     else
     {
       u32 len;
       u32 ret;

       len = fioLseek(fd, 0, SEEK_END);
       fioLseek(fd, 0, SEEK_SET);     
       fioRead(fd, loadptr, len);
       fioClose(fd);
     
       if(init.screen_mode == SCRMODE_PAL)
       {
         init.frame_count = demos[demo_loop].demo_time * 50;
       } 
       else 
       {
         init.frame_count = demos[demo_loop].demo_time * 60;
       }

       init.time_count = (float) demos[demo_loop].demo_time;
       init.time_count_i = (demos[demo_loop].demo_time << 16);
       ResetEE(0xFF);
       enable_vblank();
 
       FlushCache(0);
       FlushCache(2);
       ret = call_main(&init);
       printf("%s ret=%d\n", demos[demo_loop].name, ret);
       disable_vblank();
     }
   }

   SleepThread();
   
   return 0;
}
