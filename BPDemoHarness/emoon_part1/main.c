#include <tamtypes.h>
#include <loadfile.h>
#include "../harness.h"
#include "PbGfx.h"
#include "PbPart1.h"
#include "PbGlobal.h"

const demo_init_t* gp_Info;

///////////////////////////////////////////////////////////////////////////////
// DoStuff
///////////////////////////////////////////////////////////////////////////////

u32 start_demo( const demo_init_t* pInfo )
{
  float time       = 0.0f;
  float old_time   = 0.0f;
  float delta_time = 0.0f;

  gp_Info = pInfo;

  /////////////////////////////////////////////////////////////////////////////
  // Init screen, zbuffer, send vu1 programs, init textures, etc

  PbGfx_Setup();
  PbPart1_SetupVu1();
  PbPart1_SetupTextures(); 
  PbPart1_SetupGeneral();

  /////////////////////////////////////////////////////////////////////////////
  // This value can be tweaked so everything is in sync with the music.

  PbPart1_SetInitalTime( 0.0f );

  old_time = pInfo->curr_time;

  /////////////////////////////////////////////////////////////////////////////
  // Main loop

  while( pInfo->time_count > 0 )
  {
    // Calculate delta time to use for our own counters

    delta_time = pInfo->curr_time-old_time;
    old_time = pInfo->curr_time;
    
    PbPart1_Update( delta_time );
  }
  
  return pInfo->screen_mode;
}

