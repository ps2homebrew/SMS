#include <tamtypes.h>
#include "ee_malloc.h"

#define FREE_START 0x1800000

static u32 curr_p = FREE_START;

void *ee_malloc(u32 size)

{
   u32 buf;
  
   buf = curr_p;
   curr_p += size;
 
   return (void *) buf;
}

void *ee_malloc_aligned(u32 size, u32 align)

{
   u32 buf;

   buf = (curr_p + (align - 1)) & ~(align - 1);
   curr_p = buf + size;

   return (void *) buf;
}
