#ifndef __VIF_H__
#define __VIF_H__

#define VIF_TAG(CMD, NUM, IMM) ((((u32) (CMD) << 24) | ((u32) (NUM) << 16)) | ((u32) IMM))

/* Define VIF tag values */
#define NOP 0
#define STCYCL 1
#define OFFSET 2
#define BASE 3
#define ITOP 4
#define STMOD 5
#define MSKPATH3 6
#define MARK 7
#define FLUSHE 0x10
#define FLUSH 0x11
#define FLUSHA 0x13
#define MSCAL 0x14
#define MSCNT 0x17
#define MSCALF 0x15
#define STMASK 0x20
#define STROW 0x30
#define STCOL 0x31
#define MPG 0x4A
#define DIRECT 0x50
#define DIRECTHL 0x51
#define UNPACK 0x60   /* Needs extra qualifiers to do actual command */

#define UNPACK_V4_32 0x6C

#define UNPACK_DB 0x8000

#endif
