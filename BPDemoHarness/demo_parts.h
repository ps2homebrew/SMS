#ifndef __DEMO_PARTS_H__
#define __DEMO_PARTS_H__

#define MAX_PARTS 14

typedef struct 
{
   u32 start;
   float time;
} part_t;

part_t demo_parts[MAX_PARTS] = { 
  { 0, 54.0f}, 
  { 2592000, 26.0f},
  { 3840000, 14.0f},
  { 4512000, 26.0f},
  { 5760000, 26.0f},
  { 7008000, 14.0f},
  { 7680000, 81.0f},
  { 11568000, 33.0f},
  { 13152000, 25.0f},
  { 14448000, 28.0f},
  { 15792000, 26.0f},
  { 17040000, 27.0f},
  { 18336000, 26.0f},
  { 19584000, 35.0f}
};
#endif
