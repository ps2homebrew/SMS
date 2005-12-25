/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 by Eugene Plotnikov <e-plotnikov@operamail.com>
#
# Licensed under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/
#include "SMSAudrv_US.h"

static void _demux_stereo ( const void* apMux, void* apDemux ) {

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  "li       $t0, 128\n\t"
  "1:\n\t"
  "lw       $v0, 0($a0)\n\t"
  "lw       $v1, 4($a0)\n\t"
  "lw       $a2, 1024($a0)\n\t"
  "lw       $a3, 1028($a0)\n\t"
  "addiu    $t0, $t0, -1\n\t"
  "addiu    $a0, $a0, 8\n\t"
  "sh       $v0, 0($a1)\n\t"
  "sh       $v1, 2($a1)\n\t"
  "sh       $a2, 1024($a1)\n\t"
  "sh       $a3, 1026($a1)\n\t"
  "srl      $v0, $v0, 16\n\t"
  "srl      $v1, $v1, 16\n\t"
  "srl      $a2, $a2, 16\n\t"
  "srl      $a3, $a3, 16\n\t"
  "sh       $v0, 512($a1)\n\t"
  "sh       $v1, 514($a1)\n\t"
  "sh       $a2, 1536($a1)\n\t"
  "sh       $a3, 1538($a1)\n\t"
  "bgtz     $t0, 1b\n\t"
  "addiu    $a1, $a1, 4\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  ::
 );

}  /* end _demux_stereo */

static void _demux_mono ( const void* apMux, void* apDemux ) {

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  "li       $t0, 64\n\t"
  "1:\n\t"
  "lw       $v0, 0($a0)\n\t"
  "lw       $v1, 4($a0)\n\t"
  "lw       $a2, 512($a0)\n\t"
  "lw       $a3, 516($a0)\n\t"
  "addiu    $a0, $a0, 8\n\t"
  "addiu    $t0, $t0, -1\n\t"
  "sw       $v0, 0($a1)\n\t"
  "sw       $v1, 4($a1)\n\t"
  "sw       $v0, 512($a1)\n\t"
  "sw       $v1, 516($a1)\n\t"
  "sw       $a2, 1024($a1)\n\t"
  "sw       $a3, 1028($a1)\n\t"
  "sw       $a2, 1536($a1)\n\t"
  "sw       $a3, 1540($a1)\n\t"
  "bgtz     $t0, 1b\n\t"
  "addiu    $a1, $a1, 8\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  ::
 );

}  /* end _demux_mono */

static void _direct_copy ( const void* apMux, void* apDemux ) {

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  "li   $t0, 128\n\t"
  "1:\n\t"
  "lw       $a2,  0($a0)\n\t"
  "lw       $a3,  4($a0)\n\t"
  "lw       $v0,  8($a0)\n\t"
  "lw       $v1, 12($a0)\n\t"
  "addiu    $t0, $t0, -1\n\t"
  "addiu    $a0, $a0, 16\n\t"
  "sw       $a2,  0($a1)\n\t"
  "sw       $a3,  4($a1)\n\t"
  "sw       $v0,  8($a1)\n\t"
  "sw       $v1, 12($a1)\n\t"
  "bgtz     $t0, 1b\n\t"
  "addiu    $a1, $a1, 16\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
 );

}  /* end _direct_copy */

static void _no_sound ( const void* apMux, void* apDemux ) {

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  "li   $t0, 128\n\t"
  "1:\n\t"
  "addiu    $t0, $t0, -1\n\t"
  "sw       $zero,  0($a1)\n\t"
  "sw       $zero,  4($a1)\n\t"
  "sw       $zero,  8($a1)\n\t"
  "sw       $zero, 12($a1)\n\t"
  "bgtz     $t0, 1b\n\t"
  "addiu    $a1, $a1, 16\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
 );

}  /* end _no_sound */

extern int  _init_ups   ( int, int           );
extern void _ups_stereo ( const void*, void* );
extern void _ups_mono   ( const void*, void* );

UPSFunc SMSAudrv_GetUPS ( int aFreq, int aBS, int anChannels, int* apBlockSize ) {

 if ( aBS != 16 ) {

  *apBlockSize = 2048;
  return _no_sound;

 }  /* end if */

 if ( aFreq == 48000 )

  switch ( anChannels ) {

   case 1 : *apBlockSize = 1024; return _demux_mono;
   case 2 : *apBlockSize = 2048; return _demux_stereo;
   case 5 : *apBlockSize = 2048; return _direct_copy;
   default: *apBlockSize = 2048; return _no_sound;

  }  /* end switch */

 else {

  *apBlockSize = _init_ups ( aFreq, anChannels );

  return anChannels == 2 ? _ups_stereo : _ups_mono;

 }  /* end else */

}  /* end SMSAudrv_GetUPS */
