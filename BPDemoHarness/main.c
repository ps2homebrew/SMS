#include <tamtypes.h>
#include <kernel.h>
#include <fileio.h>
#include <string.h>
#include <libpad.h>
#include <loadfile.h>
#include <sifrpc.h>
#include "demo_parts.h"
#include "dernc.h"
#include "harness.h"

#include "stream_ee/streamload.h"
int sound_enabled = 0;

#define DEF_PART "hdd:+PS2MENU"

#define BIN_LOADADDR 0x1000000
u32 *loadptr = (u32 *) BIN_LOADADDR;
typedef u32 (*main_func)(demo_init_t *p);
main_func call_main = (main_func) BIN_LOADADDR;
#define MAX_DEMOS 16
/* Bit of a hack, stops funny mallocs though */
u32 *rnc_buffer = (u32 *) 0x800000;
#define RNC_SIGNATURE 0x01434E52 

char hdd_part[64] = DEF_PART;
static u8 padBuf[256] __attribute__((aligned(64)));

void check_pad();

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
int pad_enabled = 0;
demo_entry_t demos[MAX_DEMOS];
u32 curr_demotime = 10;
u32 set_noprintf = 0;
int palmode;
u16 fft_nodata[1024];

extern int TOTAL_SYNCS;
extern u32 sync_samp[];

demo_init_t init;

int init_gs(int scr_mode);
int is_pal();

u16 *get_fft()
{
   if(sound_enabled)
   {
      return StreamLoad_GetFFT();
   }

   return fft_nodata;
}

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
      if(pad_enabled)
      { 
         check_pad();
      }
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
      if(pad_enabled)
      { 
         check_pad();
      }
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
   init.get_fft = get_fft;
   init.get_pos = StreamLoad_Position;
   init.curr_frame = 0;
   init.frame_count = 0;
   init.time_count = 0;
   init.time_count_i = 0;
   init.curr_time = 0;
   init.curr_time_i = 0;
   init.sync_points = NULL;
   init.no_syncs = 0;
}

void print_usage()

{
   printf("Demo Harness Usage:\n");
   printf("harness.elf <options> <demos>\n");
   printf("Options:\n");
   printf("-pal      : Set pal mode\n");
   printf("-ntsc     : Set NTSC mode\n");
   printf("-sound    : Enable Sound\n");
   printf("-tX       : Time in seconds to run each demo\n");
   printf("-sX       : Time in seconds to start tune at\n");
   printf("-dX       : Starts the demo at a number from sync_list.txt\n");
   printf("-noprintf : Disables the printf function\n");
   printf("-mPART    : Specifies the partition to mount for sound files\n");
   printf("-p        : Enables pad support\n");
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
            palmode = 1;
            printf("Set PAL mode\n");
         }
         else if(strcmp("ntsc", &argv[arg_loop][1]) == 0)
         {
            init.screen_mode = SCRMODE_NTSC;
            palmode = 0;
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
	 else if(argv[arg_loop][1] == 'm')
         {
            if(strlen(argv[arg_loop]) > 2)
            {
               strcpy(hdd_part, &argv[arg_loop][2]);
               printf("Set HDD Partition %s\n", hdd_part);
            }
            else
            {
              printf("Invalid argument %s\n", argv[arg_loop]);
              return -1;
            }
         }
         else if(argv[arg_loop][1] == 'p')
         {
            pad_enabled = 1;
            printf("Enable pad\n");
         }
         else if(argv[arg_loop][1] == 's')
         {  
            if((argv[arg_loop][2] >= '0') && (argv[arg_loop][2] <= '9'))
            {
              char *endp;
              demo_starttime = strtol(&argv[arg_loop][2], &endp, 10);
              demo_starttime *= 48000;
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
         else if(argv[arg_loop][1] == 'd')
         {  
            if((argv[arg_loop][2] >= '0') && (argv[arg_loop][2] <= '9'))
            {
              char *endp;
              int part;
              part = strtol(&argv[arg_loop][2], &endp, 10);
              if(part < MAX_PARTS)
              {
                 demo_starttime = demo_parts[part].start; 
                 curr_demotime = demo_parts[part].time;
                 printf("Set Demo Part %d start=%d time=%d\n", part, demo_starttime, curr_demotime);
              }
              else
              {
                 printf("Invalid part number %d\n", part);
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

void check_pad()

{
   int ret;
   u32 paddata;
   static u32 oldpad = 0;
   u32 newpad;
   struct padButtonStatus buttons;

   ret = padGetState(0, 0);
   if(ret == PAD_STATE_STABLE)
   {
      ret = padRead(0, 0, &buttons);
      if(ret != 0)
      {
         paddata = 0xFFFF ^ ((buttons.btns[0] << 8) | buttons.btns[1]);
         newpad = paddata & ~oldpad;
         oldpad = paddata;

         if(newpad & PAD_CROSS)
         {
            printf("Stream Position = %d\n", StreamLoad_Position());
         }
      }
   }
}

void setup_pad()

{
   int ret;

   SifLoadModule("rom0:SIO2MAN", 0, NULL);
   SifLoadModule("rom0:PADMAN", 0, NULL);
   padInit(0);
   if((ret = padPortOpen(0, 0, padBuf)) == 0)
   {
      printf("Pad open failed. Disabling pad support ret=%d\n", ret);
      pad_enabled = 0;
      return;
   }

   while((ret = padGetState(0, 0)) != PAD_STATE_STABLE)
   {
   }
}

void setup_syncs(u32 time)

{
  u32 start, end;
  int loop;

  start = StreamLoad_Position();
  end = start + time * 48000;

  for(loop = 0; loop < TOTAL_SYNCS; loop++)
  {
     if(sync_samp[loop] > start)
       break;
  } 

  if(loop == TOTAL_SYNCS)
  {
     init.sync_points = sync_samp;
     init.no_syncs = 0;
     return;
  }

  init.sync_points = &sync_samp[loop];

  for(loop += 1; loop < TOTAL_SYNCS; loop++)
  {
     if(sync_samp[loop] > end)
     {
       loop -= 1;
       break;
     }
  }

  if(loop == TOTAL_SYNCS)
  {
     loop--;
  }

  init.no_syncs = &sync_samp[loop] - init.sync_points + 1;
}

int main(int argc, char **argv)

{
   int fd;
   int demo_loop;

   SifInitRpc(0);

   if(is_pal())
   {
     init.screen_mode = SCRMODE_PAL;
     palmode = 1;
   }  
   else
   {
     init.screen_mode = SCRMODE_NTSC;
     palmode = 0;
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
  
   if(pad_enabled)
   {
     setup_pad();
   }

   if(sound_enabled)
   {
     StreamLoad_Init(0,hdd_part,palmode);
//     StreamLoad_SetupTune("HALFDEAPH");// hdd:+PS2MENU/HALFDEAPHL.RAW AND HALFDEAPHR.RAW
     StreamLoad_SetupTune("UNSEEN"); // hdd:+PS2MENU/UNSEENL.RAW AND UNSEENR.RAW
     StreamLoad_SetPosition(demo_starttime);
     //StreamLoad_Play(0x3fff);
   }

   if(sound_enabled)
   {
      u16 *ptr = StreamLoad_GetFFT();

      printf("%04X, %04X\n", ptr[0], ptr[1023]);
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
       u32 sig;

       len = fioLseek(fd, 0, SEEK_END);
       fioLseek(fd, 0, SEEK_SET);     
       fioRead(fd, &sig, 4);
       if(sig == RNC_SIGNATURE)
       {
          printf("Load RNCed file\n");
          rnc_buffer[0] = sig; 
          fioRead(fd, &rnc_buffer[1], len - 4);
          rnc_unpack(rnc_buffer, loadptr);
       }
       else
       {
         loadptr[0] = sig;
         fioRead(fd, &loadptr[1], len - 4);
       }

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
       setup_syncs(demos[demo_loop].demo_time);
       if(sound_enabled)
         StreamLoad_Play(0x3fff);
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
