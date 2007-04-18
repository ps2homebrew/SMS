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
#include <vblank.h>
#include <intrman.h>

#include "ps2ip.h"
#include "smap.h"
#include "dev9.h"

#include "../SMSUTILS/smsutils.h"

#define	UNKN_1464   *(u16 volatile*)0xbf801464

#define	IFNAME0	's'
#define	IFNAME1	'm'

#define	TIMER_INTERVAL		(100*1000)
#define	TIMEOUT				(300*1000)
#define	MAX_REQ_CNT			16

typedef struct ip_addr IPAddr;
typedef struct netif   NetIF;
typedef struct SMapIF  SMapIF;
typedef struct pbuf    PBuf;


static int		iSendReqMutex;
static int		iSendReq = 0x80000000;
static int 		iReqNR=0;
static int 		iReqCNT=0;
static PBuf*	apReqQueue[MAX_REQ_CNT];
static int      s_Event;
static int      s_Sema;

struct SMapIF {

 NetIF*	pNetIF;

};

static SMapIF SIF;

NetIF NIF;

#define	ERR_OK    0
#define	ERR_CONN -6
#define	ERR_IF  -11

static SMapStatus AddToQueue ( PBuf* pBuf ) {

 SMapStatus retVal;
 int        lSema = s_Sema;

 WaitSema ( lSema );

 if ( !iReqCNT ) {

  retVal = SMap_Send ( pBuf );

  if ( retVal == SMap_TX ) goto storeLast;

 } else if ( iReqCNT < MAX_REQ_CNT ) {
storeLast:
  apReqQueue[ ( iReqNR + iReqCNT ) % MAX_REQ_CNT ] = pBuf;
  ++iReqCNT;
  ++pBuf -> ref;

  retVal = SMap_OK;

 } else retVal = SMap_TX;

 SignalSema ( lSema );

 return	retVal;

}  /* end AddToQueue */

static void QueueHandler ( void ) {

 int lSema = s_Sema;

 WaitSema ( lSema );

 while ( iReqCNT > 0 ) {

  PBuf*      lpReq = apReqQueue[ iReqNR ];
  SMapStatus lSts  = SMap_Send ( lpReq );

  if ( lSts == SMap_TX ) goto end;

  pbuf_free ( lpReq );

  iReqNR = ( iReqNR + 1 ) % MAX_REQ_CNT;
  --iReqCNT;

 }  /* end while */

 if ( iSendReq >= 0 ) {

  SignalSema ( iSendReq );
  iSendReq = 0x80000000;

 }  /* end if */
end:
 SignalSema ( lSema );

}  /* end QueueHandler */

static int SMapInterrupt ( int iFlag ) {

 dev9IntrDisable ( INTR_BITMSK );
 iSetEventFlag ( s_Event, EVENT_INTR );

 return 1;

}  /* end SMapInterrupt */

static unsigned int Timer ( void* pvArg ) {

 if ( iReqCNT ) iSetEventFlag ( s_Event, EVENT_TIMER );

 return	( unsigned int )pvArg;

}  /* end Timer */

static err_t SMapLowLevelOutput ( NetIF* pNetIF, PBuf* pOutput ) {

 while ( 1 ) {

  int        iFlags;
  SMapStatus Res;

  if (   (  Res = AddToQueue ( pOutput )  ) == SMap_OK   ) return ERR_OK;
  if (      Res                             == SMap_Err  ) return ERR_IF;
  if (      Res                             == SMap_Con  ) return ERR_CONN;

  CpuSuspendIntr ( &iFlags );

  if ( iReqCNT == MAX_REQ_CNT ) {

   iSendReq = iSendReqMutex;
   CpuResumeIntr ( iFlags );

   WaitSema ( iSendReqMutex );

  } else CpuResumeIntr ( iFlags );

 }  /* end while */

}  /* end SMapLowLevelOutput */

void SMap_Thread ( void* apArg ) {

 int lSema = s_Sema;

 while ( 1 ) {

  unsigned long lRes;
  int           lfTXBD = 0;

  WaitEventFlag (  ( int )apArg, EVENT_MASK, WEF_CLEAR | WEF_OR, &lRes  );

  if ( lRes & EVENT_TIMER ) {
   QueueHandler ();
   lfTXBD = SMap_HandleTXInterrupt ( lSema );
  }  /* end if */

  if ( lRes & EVENT_INTR ) {

   int lFlags = SMap_GetIRQ ();

   if ( lFlags & INTR_EMAC3 ) SMap_HandleEMACInterrupt ();
   if ( lFlags & INTR_RXEND ) SMap_HandleRXInterrupt   ();
   if ( lFlags & INTR_RXDNV ) SMap_ClearIRQ ( INTR_RXDNV );
   if ( lFlags & INTR_TXDNV ) SMap_HandleTXInterrupt   ( lSema );

   QueueHandler ();

   lfTXBD = SMap_HandleTXInterrupt ( lSema );

   dev9IntrEnable ( INTR_EMAC3 | INTR_RXEND | INTR_RXDNV );

  }  /* end if */

  if ( lfTXBD ) dev9IntrEnable ( INTR_TXDNV );

 }  /* end while */

}  /* end SMap_Thread */

static err_t SMapOutput ( NetIF* pNetIF, PBuf* pOutput, IPAddr* pIPAddr ) {

 PBuf* pBuf = etharp_output ( pNetIF, pIPAddr, pOutput );

 return pBuf ? SMapLowLevelOutput ( pNetIF, pBuf ) : ERR_OK;

}  /* end SMapOutput */

static err_t SMapIFInit ( NetIF* pNetIF ) {

 SIF.pNetIF = pNetIF;
 pNetIF -> state      = &NIF;
 pNetIF -> name[ 0 ]  = IFNAME0;
 pNetIF -> name[ 1 ]  = IFNAME1;
 pNetIF -> output     = SMapOutput;
 pNetIF -> linkoutput = SMapLowLevelOutput;
 pNetIF -> hwaddr_len = 6;
 pNetIF -> mtu        = 1500;

 mips_memcpy (  pNetIF -> hwaddr, SMap_GetMACAddress (), 6  );

 SMap_Start ();

 return ERR_OK;

}  /* end SMapIFInit */

static int SMapInit ( IPAddr IP, IPAddr NM, IPAddr GW ) {

 int             i;
 iop_sys_clock_t ClockTicks;
 iop_event_t     lEvent;
 iop_thread_t    lThread = { TH_C, 0, SMap_Thread, 0x800, 0x18 };

 dev9IntrDisable ( INTR_BITMSK );
 EnableIntr ( IOP_IRQ_DEV9 );
 CpuEnableIntr ();

 UNKN_1464 = 3;

 if (   (  s_Sema        = CreateMutex ( IOP_MUTEX_UNLOCKED )  ) < 0   ) return	0;
 if (   (  iSendReqMutex = CreateMutex ( IOP_MUTEX_UNLOCKED )  ) < 0   ) return	0;
 if	(   !SMap_Init ()                                                  ) return	0;

 lEvent.attr = 0;
 lEvent.bits = 0;
 s_Event = CreateEventFlag ( &lEvent );
 StartThread (  CreateThread ( &lThread ), ( void* )s_Event  );

 for ( i = 2; i < 7; ++i ) dev9RegisterIntrCb ( i, SMapInterrupt );

 USec2SysClock ( TIMER_INTERVAL, &ClockTicks );
 SetAlarm (  &ClockTicks, Timer, ( void* )ClockTicks.lo  );

 netif_add ( &NIF, &IP, &NM, &GW, NULL, SMapIFInit, tcpip_input );
 netif_set_default ( &NIF );

 return 1;

}  /* end SMapInit */

int _start ( int iArgC,char** ppcArgV ) {

 IPAddr IP;
 IPAddr NM;
 IPAddr GW;

 if ( iArgC >= 4 ) {

  IP.addr = inet_addr ( ppcArgV[ 1 ] );
  NM.addr = inet_addr ( ppcArgV[ 2 ] );
  GW.addr = inet_addr ( ppcArgV[ 3 ] );

 } else {

  IP4_ADDR( &IP, 192, 168,   0, 80 );
  IP4_ADDR( &NM, 255, 255, 255,  0 );
  IP4_ADDR( &GW, 192, 168,   0,  1 );

 }  /* end else */

 return !SMapInit( IP, NM, GW );

}  /* end _start */
