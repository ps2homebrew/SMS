/*
 * Copyright (c) Tord Lindstrom (pukko@home.se)
 * Copyright (c) adresd ( adresd_ps2dev@yahoo.com )
 *
 */

#include <stdio.h>
#include <loadcore.h>
#include <thbase.h>
#include <thevent.h>
#include <thsemap.h>
#include <intrman.h>
#include <sifcmd.h>
#include <sifman.h>
#include <ps2ip.h>
#include <dev9.h>

#include "smap.h"
#include "../SMSUTILS/smsutils.h"

#define SMAP_EVENT_INTERRUPT 0x00000001
#define SMAP_EVENT_FLUSH     0x00000002
#define SMAP_EVENT_INIT      0x00000004
#define SMAP_EVENTS ( SMAP_EVENT_INTERRUPT | SMAP_EVENT_FLUSH | SMAP_EVENT_INIT )

static struct ip_addr s_IP, s_NM, s_GW;

struct netif g_NIF;
SMap         g_SMAP;

void smap_flush_queue ( void );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "smap_flush_queue:\n\t"
 "addiu     $sp, $sp, -28\n\t"
 "sw        $ra,  0($sp)\n\t"
 "sw        $s4,  4($sp)\n\t"
 "sw        $s3,  8($sp)\n\t"
 "sw        $s2, 12($sp)\n\t"
 "lui       $s2, %hi( g_SMAP )\n\t"
 "sw        $s1, 16($sp)\n\t"
 "sw        $s0, 20($sp)\n\t"
 "addiu     $s2, $s2, %lo( g_SMAP )\n\t"
 "jal       CpuSuspendIntr\n\t"
 "addiu     $a0, $sp, 24\n\t"
 "lw        $s3, 16($s2)\n\t"       /* s3 = m_IIdx */
 "lw        $s1, 20($s2)\n\t"       /* s1 = m_OIdx */
 "nop\n\t"
 "beq       $s3, $s1, 1f\n\t"       /* if ( m_IIdx == m_OIdx ) goto 1f */
 "addiu     $s4, $zero, 3\n\t"
 "2:\n\t"
 "sll       $v0, $s1, 2\n\t"
 "addu      $v0, $v0, $s2\n\t"
 "lw        $s0, 36($v0)\n\t"
 "jal       SMap_Send\n\t"
 "move      $a0, $s0\n\t"
 "beq       $v0, $s4, 1f\n\t"       /* if SMap_Send == SMap_TX goto 1f */
 "move      $a0, $s0\n\t"
 "jal       pbuf_free\n\t"
 "nop\n\t"
 "addiu     $v1, $s1, 1\n\t"
 "andi      $s1, $v1, 15\n\t"
 "bne       $s3, $s1, 2b\n\t"
 "sw        $s1, 20($s2)\n\t"
 "1:\n\t"
 "lw        $s3, 28($s2)\n\t"       /* s3 = SMap.m_fWait */
 "lw        $a0, 24($sp)\n\t"
 "jal       CpuResumeIntr\n\t"
 "nop\n\t"
 "bnez      $s3, 1f\n\t"            /* if SMap.m_fWait goto 1f */
 "lw        $ra,  0($sp)\n\t"
 "2:\n\t"
 "lw        $s4,  4($sp)\n\t"
 "lw        $s3,  8($sp)\n\t"
 "lw        $s2, 12($sp)\n\t"
 "lw        $s1, 16($sp)\n\t"
 "lw        $s0, 20($sp)\n\t"
 "jr        $ra\n\t"
 "addiu     $sp, $sp, 28\n\t"
 "1:\n\t"
 "lw        $a0, 32($s2)\n\t"       /* m_Sema      */
 "jal       SignalSema\n\t"
 "sw        $zero, 28($s2)\n\t"     /* m_fWait = 0 */
 "beq       $zero, $zero, 2b\n\t"
 "lw        $ra, 0($sp)\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);
static void smap_handle_interrupt ( void ) {

 int lFlags;

 __asm__ __volatile__(
  ".set noat\n\t"
  "lui  $at, 0xB000\n\t"
  "lhu  %0, 0x0028($at)\n\t"
  "nop\n\t"
  "andi %0, %0, 0x7C\n\t"
  ".set at\n\t"
  : "=r"( lFlags ) :: "at"
 );

 if ( lFlags & INTR_TXEND ) {

  SMap_HandleTXInt ();
  smap_flush_queue ();

 }  /* end if */

 if	( lFlags & INTR_EMAC3 ) __asm__ __volatile__(
  ".set noat\n\t"
  "lui     $at, 0xB000\n\t"
  "lw      $a0, 0x2014($at)\n\t"
  "addiu   $a0, $zero, 0x40\n\t"
  "sh      $a0, 0x0128($at)\n\t"
  "lui     $a0, 0x01C0\n\t"
  "sw      $a0, 0x2014($at)\n\t"
  "lw      $a0, 0x2014($at)\n\t"
  ".set at\n\t"
  ::: "at", "a0"
 );

 if ( lFlags & INTR_RXEND ) SMap_HandleRXInt ();

 if ( lFlags & INTR_RXDNV ) __asm__ __volatile__(
  ".set noat\n\t"
  "lui     $at, 0xB000\n\t"
  "addiu   $a0, $zero, 0x04\n\t"
  "sh      $a0, 0x0128($at)\n\t"
  ".set at\n\t"
  ::: "at", "a0"
 );

 dev9IntrEnable ( INTR_TXEND | INTR_EMAC3 | INTR_RXDNV | INTR_RXEND );

}  /* end smap_handle_interrupt */

static int SMAP_InterrupHandler ( int aFlags ) {

 dev9IntrDisable ( INTR_BITMSK );
 iSetEventFlag ( g_SMAP.m_Event, SMAP_EVENT_INTERRUPT );

 return 1;

}  /* end SMAP_InterrupHandler */

static void _smap_reset_queue ( SMap* apSMap ) {

 int lIIdx = apSMap -> m_IIdx;
 int lOIdx = apSMap -> m_OIdx;

 while ( lOIdx != lIIdx ) {

  pbuf_free ( apSMap -> m_TXQueue[ lOIdx ] );
  lOIdx = ( lOIdx + 1 ) & ( SMAP_TXQUEUE_SIZE - 1 );

 }  /* end while */

 apSMap -> m_IIdx   = 0;
 apSMap -> m_OIdx   = 0;
 apSMap -> m_TXFree = 4096;

 SMap_Reset (  apSMap, ( u16* )apSMap -> m_HWAddr  );

}  /* end _smap_reset_queue */

static err_t _smap_link_output ( struct netif* apNetIF, struct pbuf* apBuf ) {

 SMap* lpSMap;
 int   lIIdx;
 int   lNextIdx;
 int   lOIdx;
 int   lState;
 int   retVal;

 __asm__ __volatile__(
  "la   %0, g_SMAP\n\t"
  : "=r"( lpSMap )
 );

 CpuSuspendIntr ( &lState );

 lIIdx    = lpSMap -> m_IIdx;
 lOIdx    = lpSMap -> m_OIdx;
 lNextIdx = ( lIIdx + 1 ) & ( SMAP_TXQUEUE_SIZE - 1 );

 if (  lIIdx == lOIdx && SMap_Send ( apBuf ) == SMap_OK  )

  retVal = ERR_OK;

 else while ( 1 ) {

  if ( lNextIdx != lOIdx ) {

   lpSMap -> m_TXQueue[ lIIdx ] = apBuf;
   lpSMap -> m_IIdx             = lNextIdx;

   ++apBuf -> ref;

   CpuResumeIntr ( lState );

   SetEventFlag ( lpSMap -> m_Event, SMAP_EVENT_FLUSH );

   return ERR_OK;

  } else if (  !_smap_check_link ()  ) {

   _smap_reset_queue ( lpSMap );
   retVal = ERR_CONN;
   break;

  } else {

   lpSMap -> m_fWait = 1;
   CpuResumeIntr ( lState );
   WaitSema ( lpSMap -> m_Sema );
   CpuSuspendIntr ( &lState );
   lOIdx = lpSMap -> m_OIdx;

  }  /* end else */

 }  /* end while */

 CpuResumeIntr ( lState );

 return retVal;

}  /* end _smap_link_output */

static err_t _smap_output ( struct netif* apNetIF, struct pbuf* apOutput, struct ip_addr* apIPAddr ) {

 struct pbuf* lpBuf = etharp_output ( apNetIF, apIPAddr, apOutput );

 return lpBuf ? _smap_link_output ( apNetIF, lpBuf ) : ERR_OK;

}  /* end _smap_output */

static err_t _smap_interface_init ( struct netif* apNetIF ) {

 apNetIF -> state      = &g_NIF;
 apNetIF -> name[ 0 ]  = 's';
 apNetIF -> name[ 1 ]  = 'm';
 apNetIF -> output     = _smap_output;
 apNetIF -> linkoutput = _smap_link_output;
 apNetIF -> hwaddr_len = 6;
 apNetIF -> mtu        = 1500;

 mips_memcpy ( apNetIF -> hwaddr, g_SMAP.m_HWAddr, 6 );

 SMap_Start ();

 return ERR_OK;

}  /* end _smap_interface_init */

static void smap_module_init ( void ) {

 int i;
 int lCmdData[ 4 ];

 dev9IntrDisable ( INTR_BITMSK );
 EnableIntr ( IOP_IRQ_DEV9 );
 CpuEnableIntr ();

 *( u16 volatile* )0xBF801464 = 3;

 SMap_Init ();

 for ( i = 2; i < 7; ++i ) dev9RegisterIntrCb ( i, SMAP_InterrupHandler );

 netif_add ( &g_NIF, &s_IP, &s_NM, &s_GW, NULL, _smap_interface_init, tcpip_input );
 netif_set_default ( &g_NIF );

 lCmdData[ 3 ] = 4;

 do {
  i = sceSifSendCmd ( 18, lCmdData, 16, NULL, NULL, 0 );
  DelayThread ( 100 );
 } while ( !i );

 while (  sceSifDmaStat ( i ) >= 0  ) DelayThread ( 100 );

}  /* end smap_module_init */

static void _smap_io_thread ( void* apArg ) {

 while ( 1 ) {

  unsigned long lRes;

  WaitEventFlag (  ( int )apArg, SMAP_EVENTS, WEF_CLEAR | WEF_OR, &lRes  );

  if ( lRes & SMAP_EVENT_INTERRUPT ) smap_handle_interrupt ();
  if ( lRes & SMAP_EVENT_FLUSH     ) smap_flush_queue      ();
  if ( lRes & SMAP_EVENT_INIT      ) smap_module_init      ();

 }  /* end while */

}  /* end _smap_io_thread */

int _start ( int argc, char** argv ) {

 iop_event_t  lEvent;
 iop_thread_t lThread = { TH_C, 0, _smap_io_thread, 0x800, 0x40 };

 g_SMAP.m_Conf = 0x05E0;

 if ( argc >= 4 ) {

  int lConf;

  s_IP.addr = inet_addr ( argv[ 1 ] );
  s_NM.addr = inet_addr ( argv[ 2 ] );
  s_GW.addr = inet_addr ( argv[ 3 ] );

  if (  argc == 5 && mips_scan ( argv[ 4 ], &lConf )  ) {

   if ( lConf == 0 )
    g_SMAP.m_fAuto = 1;
   else if ( lConf > 1 ) g_SMAP.m_Conf = lConf;

  }  /* end if */

 } else {

  IP4_ADDR( &s_IP, 192, 168,   0, 80 );
  IP4_ADDR( &s_NM, 255, 255, 255,  0 );
  IP4_ADDR( &s_GW, 192, 168,   0,  1 );

 }  /* end else */

 lEvent.attr = 0;
 lEvent.bits = SMAP_EVENT_INIT;
 g_SMAP.m_Event = CreateEventFlag ( &lEvent );
 g_SMAP.m_Sema  = CreateMutex ( IOP_MUTEX_LOCKED );

 StartThread (  CreateThread ( &lThread ), ( void* )g_SMAP.m_Event  );

 return MODULE_RESIDENT_END;

}  /* end _start */
