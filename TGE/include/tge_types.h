/*
 * tge_types.h - TGE types header file
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_TYPES_H
#define TGE_TYPES_H

typedef	unsigned char 		u8;
typedef unsigned short 		u16;
typedef unsigned int 		u32;
typedef unsigned long int	u64;

typedef signed char 		s8;
typedef signed short 		s16;
typedef	signed int 		s32;
typedef signed long int		s64;

#if defined(R5900) || defined(_R5900)
typedef unsigned int		u128 __attribute__((mode(TI)));
typedef int			s128 __attribute__((mode(TI)));
#endif

#endif /* TGE_TYPES_H */
