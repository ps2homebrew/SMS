/* Functions to do simple vram allocation */

#include <tamtypes.h>

static u32 curr_p = 0;

void reset_malloc()

{
  curr_p = 0;
}

u32 vram_malloc(u32 size)
     /* Allocate VRAM only on page boundries */

{
  u32 new_p;

  new_p = curr_p;
  curr_p += (size);

  if(curr_p & 0x1FFF)
    {
      curr_p = (curr_p & 0xFFFFE000) + 0x2000;
    }

  return new_p;
}
