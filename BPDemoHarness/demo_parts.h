#ifndef __DEMO_PARTS_H__
#define __DEMO_PARTS_H__

#define MAX_PARTS 11

typedef struct 
{
   u32 start;
   float time;
} part_t;

part_t demo_parts[MAX_PARTS] = { 
  { 0, 56.0f}, 
  { 2688000, 26.0f},
  { 3936000, 14.0f},
  { 4608000, 26.0f},
  { 5856000, 26.0f},
  { 7104000, 14.0f},
  { 7776000, 33.0f},
  { 9312000, 26.0f},
  { 10608000, 26.0f},
  { 11856000, 26.0f},
  { 13104000, 32.0f}
};
#endif
