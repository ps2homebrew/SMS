/*
 * tge_defs.h - TGE definitions header file
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */


#ifndef TGE_DEFS_H
#define TGE_DEFS_H

#include "tge_types.h"

/* Convenience macros.  */
#define ALIGNED(x)	__attribute__((aligned((x))))
#define PACKED		__attribute__((packed))

#ifndef NULL
#define NULL	((void *)0)
#endif

static inline u8  _lb(u32 addr) { return *(volatile u8 *)addr; }
static inline u16 _lh(u32 addr) { return *(volatile u16 *)addr; }
static inline u32 _lw(u32 addr) { return *(volatile u32 *)addr; }
static inline u64 _ld(u32 addr) { return *(volatile u64 *)addr; }

static inline void _sb(u8 val, u32 addr) { *(volatile u8 *)addr = val; }
static inline void _sh(u16 val, u32 addr) { *(volatile u16 *)addr = val; }
static inline void _sw(u32 val, u32 addr) { *(volatile u32 *)addr = val; }
static inline void _sd(u64 val, u32 addr) { *(volatile u64 *)addr = val; }

#if defined(R5900) || defined(_R5900)
static inline u128 _lq(u32 addr) { return *(volatile u128 *)addr; }
static inline void _sq(u128 val, u32 addr) { *(volatile u128 *)addr = val; }
#endif

#endif /* TGE_DEFS_H */
