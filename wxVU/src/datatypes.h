#ifndef _DATATYPES
#define _DATATYPES

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef char int8;
typedef short int16;
typedef int int32;

#ifndef WIN32 
typedef unsigned long long uint64;
typedef long long int64;
#else
typedef unsigned long uint64;
typedef long int64;
#endif

typedef struct int128 {
   uint64 lo, hi;
} int128;

typedef struct fvec {
   float x,y,z,w;
} fvec;

typedef struct ivec {
    uint32 x, y, z, w;
} ivec;

typedef struct ivec16 {
	int16 x,y,z,w;
}ivec16;

typedef union vec {
	int128 value;
	fvec vf;
	ivec mi;
	int16 vi;
}vec;

typedef struct dataquad {
	int32 x, y, z, w;
	bool gif;
}dataquad;
#endif
