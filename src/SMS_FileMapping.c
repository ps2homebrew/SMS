/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2001-2004, ps2dev - http://www.ps2dev.org
# (c) 2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include <kernel.h>
#include <malloc.h>
#include <stdio.h>

#include "SMS_EE.h"
#include "SMS_FileContext.h"

#define MAP_WINDOW_SIZE  16384
#define MAP_WINDOW_SIZE2 ( MAP_WINDOW_SIZE * 2 )

typedef struct FileMapping {

 struct FileMapping* m_pNext;          /*   0 */
 struct FileMapping* m_pPrev;          /*   4 */
 unsigned int        m_Start;          /*   8 */
 unsigned int        m_End;            /*  12 */
 u128                m_Regs[ 32 ];     /*  16 */
 unsigned int        m_EPC;            /* 528 */
 unsigned int        m_Addr;           /* 532 */
 unsigned int        m_TlbIdx;         /* 536 */
 FileContext*        m_pFile;          /* 540 */
 void*               m_pWindow;        /* 544 */
 unsigned int        m_EntryLo0;       /* 548 */
 unsigned int        m_EntryLo1;       /* 552 */

} FileMapping;

static u128         s_RegSave[ 3 ] __attribute__(   (  section( ".bss" ), unused  )   );
static unsigned int s_VTLBRefillHandlerOrg;
static FileMapping* s_pMapChain;

void _tlb_refill_handler_r ( void );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_tlb_refill_handler_r:\n\t"
 "lui   $k0, %hi( s_RegSave )\n\t"
 "sq    $at, %lo( s_RegSave )+ 0($k0)\n\t"  /* save at                      */
 "lui   $at, %hi( s_pMapChain )\n\t"
 "sq    $v0, %lo( s_RegSave )+16($k0)\n\t"  /* save v0                      */
 "mfc0  $v0, $8\n\t"                        /* v0 = BadVAddr                */
 "lw    $at, %lo( s_pMapChain )($at)\n\t"   /* at = FileMapping pointer     */
 "sq    $v1, %lo( s_RegSave )+32($k0)\n\t"  /* save v1                      */
 "2:\n\t"
 "beql  $at, $zero, 1f\n\t"                 /* if no mapping found then     */
 "lq    $v1, %lo( s_RegSave )+32($k0)\n\t"  /*  restore v1 and goto 1       */
 "lw    $v1, 8($at)\n\t"                    /* v1 = map start               */
 "sltu  $v1, $v0, $v1\n\t"                  /* v0 = BadVAddr < map start    */
 "bnel  $v1, $zero, 2b\n\t"                 /* if yes then try next mapping */
 "lw    $at, 0($at)\n\t"
 "lw    $v1, 12($at)\n\t"                   /* v1 = map end                 */ 
 "subu  $v1, $v0, $v1\n\t"                  /* v1 = BadVAddr - map end      */
 "bgezl $v1, 2b\n\t"                        /* if v1 >= 0 then try next     */
 "lw    $at, 0($at)\n\t"
 "sw    $v0, 532($at)\n\t"                  /* m_Addr = v0                  */
 "addiu $sp, $sp, -16\n\t"
 "sw    $at, 0($sp)\n\t"                    /* save lpFileMap in the stack  */
 "lq    $at, %lo( s_RegSave )+ 0($k0)\n\t"  /* restore at                   */
 "lq    $v0, %lo( s_RegSave )+16($k0)\n\t"  /* restore v0                   */
 "lq    $v1, %lo( s_RegSave )+32($k0)\n\t"  /* restore v1                   */
 "lw    $k0, 0($sp)\n\t"                    /* load lpFileMap into k0       */
 "addiu $sp, $sp, 16\n\t"
 "sq    $at,  16($k0)\n\t"
 "sq    $v0,  32($k0)\n\t"
 "sq    $v1,  48($k0)\n\t"
 "sq    $a0,  64($k0)\n\t"
 "sq    $a1,  80($k0)\n\t"
 "sq    $a2,  96($k0)\n\t"
 "sq    $a3, 112($k0)\n\t"
 "sq    $t0, 128($k0)\n\t"
 "sq    $t1, 144($k0)\n\t"
 "sq    $t2, 160($k0)\n\t"
 "sq    $t3, 176($k0)\n\t"
 "sq    $t4, 192($k0)\n\t"
 "sq    $t5, 208($k0)\n\t"
 "sq    $t6, 224($k0)\n\t"
 "sq    $t7, 240($k0)\n\t"
 "sq    $t8, 256($k0)\n\t"
 "sq    $t9, 272($k0)\n\t"
 "sq    $s0, 288($k0)\n\t"
 "sq    $s1, 304($k0)\n\t"
 "sq    $s2, 320($k0)\n\t"
 "sq    $s3, 336($k0)\n\t"
 "sq    $s4, 352($k0)\n\t"
 "sq    $s5, 368($k0)\n\t"
 "sq    $s6, 384($k0)\n\t"
 "sq    $s7, 400($k0)\n\t"
 "sq    $fp, 416($k0)\n\t"
 "sq    $gp, 432($k0)\n\t"
 "sq    $sp, 448($k0)\n\t"
 "pmfhi $v0\n\t"
 "pmflo $v1\n\t"
 "sq    $v0, 464($k0)\n\t"
 "sq    $v1, 480($k0)\n\t"
 "mfsa  $v0\n\t"
 "sd    $v0, 496($k0)\n\t"
 "sd    $ra, 504($k0)\n\t"
 "lui   $v0, %hi( _file_mapping_remap )\n\t"
 "addiu $v0, $v0, %lo( _file_mapping_remap )\n\t"
 "mfc0  $v1, $12\n\t"                   /* v1 = Status                  */
 "mfc0  $a0, $14\n\t"                   /* a0 = EPC                     */
 "ori   $v1, $v1, 0x0013\n\t"           /* User mode (1) | EXL | IE (3) */
 "sw    $a0, 528($k0)\n\t"              /* save EPC                     */
 "or    $a0, $zero, $k0\n\t"            /* a0 = lpFileMap               */
 "lui   $gp, %hi( _gp )\n\t"
 "addiu $gp, $gp, %lo( _gp )\n\t"       /* load gp                      */
 "mtc0  $v0, $14\n\t"                   /* EPC                          */
 "mtc0  $v1, $13\n\t"                   /* Status                       */
 "sync.p\n\t"
 "eret\n\t"                             /* jump to _file_mapping_remap  */
 "1:\n\t"
 "lq    $v0, %lo( s_RegSave )+16($k0)\n\t"
 "lq    $at, %lo( s_RegSave )+ 0($k0)\n\t"
 "lui   $k0, %hi( s_VTLBRefillHandlerOrg )\n\t"
 "lw    $k0, %lo( s_VTLBRefillHandlerOrg )($k0)\n\t"
 "jr    $k0\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void _kreturn_from_remap ( FileMapping* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_kreturn_from_remap:\n\t"
 "or    $k0, $zero, $a0\n\t"
 "lq    $at,  16($k0)\n\t"
 "lq    $v0,  32($k0)\n\t"
 "lq    $v1,  48($k0)\n\t"
 "lq    $a0,  64($k0)\n\t"
 "lq    $a1,  80($k0)\n\t"
 "lq    $a2,  96($k0)\n\t"
 "lq    $a3, 112($k0)\n\t"
 "lq    $t0, 128($k0)\n\t"
 "lq    $t1, 144($k0)\n\t"
 "lq    $t2, 160($k0)\n\t"
 "lq    $t3, 176($k0)\n\t"
 "lq    $t4, 192($k0)\n\t"
 "lq    $t5, 208($k0)\n\t"
 "lq    $t6, 224($k0)\n\t"
 "lq    $t7, 240($k0)\n\t"
 "lq    $t8, 256($k0)\n\t"
 "lq    $t9, 272($k0)\n\t"
 "lq    $s0, 288($k0)\n\t"
 "lq    $s1, 304($k0)\n\t"
 "lq    $s2, 320($k0)\n\t"
 "lq    $s3, 336($k0)\n\t"
 "lq    $s4, 352($k0)\n\t"
 "lq    $s5, 368($k0)\n\t"
 "lq    $s6, 384($k0)\n\t"
 "lq    $s7, 400($k0)\n\t"
 "lq    $fp, 416($k0)\n\t"
 "lq    $gp, 432($k0)\n\t"
 "lq    $sp, 448($k0)\n\t"
 "lq    $ra, 464($k0)\n\t"
 "pmthi $ra\n\t"
 "lq    $ra, 480($k0)\n\t"
 "pmtlo $ra\n\t"
 "ld    $ra, 496($k0)\n\t"
 "mtsa  $ra\n\t"
 "ld    $ra, 504($k0)\n\t"
 "lw    $k0, 528($k0)\n\t"
 "mtc0  $k0, $14\n\t"           /* EPC    */
 "mfc0  $k0, $12\n\t"           /* Status */
 "ori   $k0, $k0, 0x0013\n\t"
 "mtc0  $k0, $12\n\t"
 "sync.p\n\t"
 "eret\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void _return_from_remap ( FileMapping* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_return_from_remap:\n\t"
 "ori   $v1, $zero, 251\n\t"
 "syscall\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void SMS_FileMappingInit ( void ) {

 unsigned int lpVTLBRefillHandler;
 int          lState;

 lState = SMS_EEDIntr ();
  ee_kmode_enter ();
   __asm__ __volatile__(
    ".set noat\n\t"
    "lui     $at, 0x8000\n\t"
    "lw      %0,  12($at)\n\t"
    "lw      $at, 24($at)\n\t"
    "sll     %0, %0, 16\n\t"
    "andi    $at, $at, 0xFFFF\n\t"
    "or      %0, %0, $at\n\t"
    "lw      %0, 8(%0)\n\t"
    ".set at\n\t"
    : "=r"( lpVTLBRefillHandler ) :: "at"
   );
  ee_kmode_exit  ();
 SMS_EEIntr ( lState );

 s_VTLBRefillHandlerOrg = lpVTLBRefillHandler;

 SetVTLBRefillHandler ( 2, _tlb_refill_handler_r );
 SetSyscall ( 251, _kreturn_from_remap );

}  /* end SMS_FileMappingInit */

void SMS_FileMappingDestroy ( void );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".globl SMS_FileMappingDestroy\n\t"
 ".text\n\t"
 "SMS_FileMappingDestroy:\n\t"
 "lw    $a1, s_VTLBRefillHandlerOrg\n\t"
 "j     SetVTLBRefillHandler\n\t"
 "ori   $a0, $zero, 2\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void _file_mapping_remap ( FileMapping* apMap ) {

 unsigned int   lOffset = ( apMap -> m_Addr - apMap -> m_Start ) & ~( MAP_WINDOW_SIZE2 - 1 );
 unsigned int   lVBase  = apMap -> m_Start + lOffset;
 FileContext*   lpFile  = apMap -> m_pFile;
 SMS_EETlbEntry lEntry;

 if ( lOffset != lpFile -> m_CurPos ) lpFile -> Seek ( lpFile, lOffset );

 lpFile -> Read ( lpFile, apMap -> m_pWindow, MAP_WINDOW_SIZE2 );

 lEntry.m_PageMask = MAP_WINDOW_SIZE - 4096;
 lEntry.m_EntryHi  = lVBase;
 lEntry.m_EntryLo0 = apMap -> m_EntryLo0;
 lEntry.m_EntryLo1 = apMap -> m_EntryLo1;

 SMS_EETlbSet ( apMap -> m_TlbIdx, &lEntry );

 _return_from_remap ( apMap );

}  /* end _file_mapping_remap */

void* SMS_FileMappingMap ( FileContext* apFileCtx ) {

 void* retVal = ( void* )0x32000000;  /* implement VAddr allocation here */

 if ( retVal ) {

  FileMapping* lpMap  = ( FileMapping* )calloc (  1, sizeof ( FileMapping )  );
  int          lState = SMS_EEDIntr ();

  if ( !s_pMapChain )
   s_pMapChain = lpMap;
  else {
   FileMapping* lpChain = s_pMapChain;
   while ( lpChain -> m_pNext ) lpChain = lpChain -> m_pNext;
   lpChain -> m_pNext = lpMap;
   lpMap -> m_pPrev   = lpChain;
  }  /* end else */

  SMS_EEIntr ( lState );

  lpMap -> m_Start    = ( unsigned int )retVal;
  lpMap -> m_End      = (  ( unsigned int )retVal + apFileCtx -> m_Size + MAP_WINDOW_SIZE2 + MAP_WINDOW_SIZE2 - 1  ) & ~( MAP_WINDOW_SIZE2 - 1 );
  lpMap -> m_TlbIdx   = SMS_EETlbAlloc ();
  lpMap -> m_pFile    = apFileCtx;
  lpMap -> m_pWindow  = memalign ( MAP_WINDOW_SIZE, MAP_WINDOW_SIZE2 );
  lpMap -> m_EntryLo0 = (   (  ( unsigned int )lpMap -> m_pWindow +               0  ) >> 6   ) | ( 3U << 3 ) | 7;
  lpMap -> m_EntryLo1 = (   (  ( unsigned int )lpMap -> m_pWindow + MAP_WINDOW_SIZE  ) >> 6   ) | ( 3U << 3 ) | 7;

 }  /* end if */

 return retVal;

}  /* end SMS_FileMappingMap */

void SMS_FileMappingUnMap ( void* apAddr ) {

 FileMapping* lpIt   = s_pMapChain;
 int          lState = SMS_EEDIntr ();

 while ( lpIt ) {

  if (  lpIt -> m_Start == ( unsigned int )apAddr  ) {

   SMS_EETlbFree ( lpIt -> m_TlbIdx );

   if ( lpIt == s_pMapChain )
    s_pMapChain = lpIt -> m_pNext;
   else {
    lpIt -> m_pPrev -> m_pNext = lpIt -> m_pNext;
    if ( lpIt -> m_pNext ) lpIt -> m_pNext -> m_pPrev = lpIt -> m_pPrev;
   }  /* end else */

   break;

  }  /* end if */

  lpIt = lpIt -> m_pNext;

 }  /* end while */

 SMS_EEIntr ( lState );

 if ( lpIt ) {
  free ( lpIt -> m_pWindow );
  free ( lpIt );
 }  /* end if */

}  /* end SMS_FileMappingUnMap */
