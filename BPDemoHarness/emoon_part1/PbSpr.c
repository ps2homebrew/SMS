#include "PbSpr.h"
#include "PbGlobal.h"

static void* gp_Spr = (void*)0x70000000;

/////////////////////////////////////////////////////////////////////
// void* PbSpr_Alloc
// This function is used so we can easy get SPR memory that isnt in
// use, if one for example was DMAong something from the SPR and
// one tried to changed the data while dma is obtaining it funky
// stuff may happen
/////////////////////////////////////////////////////////////////////

void* PbSpr_Alloc( int Size, int AutoWrap )
{
  void* p_spr = gp_Spr;

  if( gp_Spr+Size >= ( ((void*)0x70000000)+16*1024 ) )
  {
    if( TRUE == AutoWrap )
      gp_Spr = (void*)0x70000000;
    else
      gp_Spr = NULL;
  }

  p_spr = gp_Spr;
  gp_Spr += Size;

  return p_spr;
}

