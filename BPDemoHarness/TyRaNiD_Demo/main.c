#include <tamtypes.h>
#include "../harness.h"
#include "PbScreen.h"
#include "PbPrim.h"

u32 start_demo(const demo_init_t *pbInit)

{
   u16 *fft_data;

   PbScreenSetup( SCR_W, SCR_H, SCR_PSM );

   while(pbInit->frame_count > 0)
   {
     PbScreenClear(0xFE80);
     fft_data = pbInit->get_fft();

     PbScreenSyncFlip();

   }
 
   return pbInit->screen_mode;
}
