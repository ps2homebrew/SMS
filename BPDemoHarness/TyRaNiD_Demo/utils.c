#include <tamtypes.h>
#include "shapes.h"
#include "../harness.h"
#include "utils.h"

extern demo_init_t *init;

/* Borrowed from emoon */
const char* getasbits( unsigned int value )
{
  static char name[128];
  int i;
  
  int string_pos = 0;
  unsigned int shift_reg = 1<<31;

  name[0] = 0;

  for(i = 0; i < 32; i++ )
  {
    if( (shift_reg & value) == shift_reg )
      name[string_pos] = '1';
    else
      name[string_pos] = '0';
  
    if( !((i+1) % 8) )
    {
      string_pos++;
      name[string_pos] = ' ';
    }
    
    string_pos++;
    shift_reg >>= 1;
  }

  name[string_pos+1] = 0;
  return (const char*)&name;
}

void util_print_vertex(const vertex *v)

{
  init->printf("(%f, %f, %f, %f)", v->x, v->y, v->z, v->w);
}
