/*
 * main.c - Sample of a simple vu1 drawing program.
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include <tamtypes.h>
#include "../../harness.h"
#include "PbScreen.h"

///////////////////////////////////////////////////////////////////////////////
// u32 start_demo
///////////////////////////////////////////////////////////////////////////////

u32 start_demo( const demo_init_t* pInfo )
{
  /////////////////////////////////////////////////////////////////////////////
  // Setup screen

  PbScreenSetup( SCR_W, SCR_H, SCR_PSM );

  /////////////////////////////////////////////////////////////////////////////
  // Loop of the demo (just clears the screen, vsync and flip buffer

  while( pInfo->time_count > 0 )
  {
    PbScreenClear( 30<<16|20<<8|40 );
    PbScreenSyncFlip();
  }
  
  return pInfo->screen_mode;
}
