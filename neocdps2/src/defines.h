//---------------------------------------------------------------------------
// File:	defines.h
// Author:	Tony Saveski, t_saveski@yahoo.com
// Notes:	Commonly used data-type definitions.
//---------------------------------------------------------------------------
#ifndef _DEFINES_H
#define _DEFINES_H

#define SUCCESS		1
#define FAILURE		0


typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned long int	uint64;
typedef unsigned int		uint; 

#ifdef _EE
typedef unsigned int		uint128 __attribute__(( mode(TI) ));
#endif

typedef char			int8;
typedef short			int16;
typedef int			int32;
typedef long int		int64;

#ifdef _EE
typedef int			int128 __attribute__(( mode(TI) ));
#endif 
/*
#ifndef NULL
#define NULL	(void *)0
#endif 
*/

#endif // DEFINES_H
