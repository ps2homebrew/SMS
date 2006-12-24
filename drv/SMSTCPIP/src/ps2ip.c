/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id: ps2ip.c 577 2004-09-14 14:41:46Z pixel $
# PS2 TCP/IP STACK FOR IOP
*/

#include <types.h>
#include <stdio.h>
#include <intrman.h>
#include <loadcore.h>
#include <thbase.h>
#include <vblank.h>
#include <modload.h>
#include <sysclib.h>
#include <thevent.h>
#include <thsemap.h>
#include <libsd.h>
#include <sysmem.h>
#include <lwip/memp.h>

#include <lwip/sys.h>
#include <lwip/tcpip.h>
#include <lwip/netif.h>
#include <lwip/dhcp.h>
#include <netif/loopif.h>
#include <netif/etharp.h>

#include "ps2ip_internal.h"
#include "arch/sys_arch.h"


#if		defined(DEBUG)
#define	dbgprintf(args...)	printf(args)
#else
#define	dbgprintf(args...)	((void)0)
#endif

#define	SYS_MBOX_SIZE				64

struct sys_mbox_msg
{
	struct sys_mbox_msg*		pNext;
	void*							pvMSG;
};

struct sys_mbox
{
	u16_t			u16First;
	u16_t			u16Last;
	void*			apvMSG[SYS_MBOX_SIZE];
	sys_sem_t	Mail;
	sys_sem_t	Mutex;
	int			iWaitPost;
	int			iWaitFetch;
};

typedef struct pbuf		PBuf;
typedef struct netif		NetIF;
typedef struct ip_addr	IPAddr;

#define MODNAME	"TCP/IP Stack"
IRX_ID(MODNAME,1,3);

extern struct irx_export_table	_exp_ps2ip;


static int		iTimerARP=0;
#if		defined(PS2IP_DHCP)
static int		iTimerDHCP=0;
#endif	//defined(PS2IP_DHCP)
static NetIF	LoopIF;


static u16_t inline
GenNextMBoxIndex(u16_t u16Index)
{
	return	(u16Index+1)%SYS_MBOX_SIZE;
}


static int inline IsMessageBoxFull ( sys_mbox_t apMBox ) {
 return GenNextMBoxIndex ( apMBox -> u16Last ) == apMBox -> u16First;
}  /* end IsMessageBoxFull */

static int inline IsMessageBoxEmpty ( sys_mbox_t apMBox ) {
 return apMBox -> u16Last == apMBox -> u16First;
}  /* end IsMessageBoxEmpty */

void PostInputMSG ( sys_mbox_t pMBox, void* pvMSG ) {

 pMBox -> apvMSG[ pMBox -> u16Last ] = pvMSG;
 pMBox -> u16Last = GenNextMBoxIndex ( pMBox -> u16Last );

 if	( pMBox -> iWaitFetch > 0 ) iSignalSema ( pMBox -> Mail );

}  /* end PostInputMSG */

int
ps2ip_getconfig(char* pszName,t_ip_info* pInfo)
{
	NetIF*	pNetIF=netif_find(pszName);

	if (pNetIF==NULL)
	{

		//Net interface not found.

		memset(pInfo,0,sizeof(*pInfo));
		return	0;
	}
	strcpy(pInfo->netif_name,pszName);
	pInfo->ipaddr.s_addr=pNetIF->ip_addr.addr;
	pInfo->netmask.s_addr=pNetIF->netmask.addr;
	pInfo->gw.s_addr=pNetIF->gw.addr;

	memcpy(pInfo->hw_addr,pNetIF->hwaddr,sizeof(pInfo->hw_addr));

#if		LWIP_DHCP

	if (pNetIF->dhcp)
	{
		pInfo->dhcp_enabled=1;
		pInfo->dhcp_status=pNetIF->dhcp->state;
	}
	else
	{
		pInfo->dhcp_enabled=0;
		pInfo->dhcp_status=0;
	}

#else

	pInfo->dhcp_enabled=0;

#endif

	return	1;
}


int
ps2ip_setconfig(t_ip_info* pInfo)
{
	NetIF*	pNetIF=netif_find(pInfo->netif_name);

	if	(pNetIF==NULL)
	{
		return	0;
	}
	netif_set_ipaddr(pNetIF,(IPAddr*)&pInfo->ipaddr);
	netif_set_netmask(pNetIF,(IPAddr*)&pInfo->netmask);
	netif_set_gw(pNetIF,(IPAddr*)&pInfo->gw);

#if	LWIP_DHCP

	//Enable dhcp here

	if (pInfo->dhcp_enabled)
	{
		if (!pNetIF->dhcp)
		{

			//Start dhcp client

			dhcp_start(pNetIF);
		}
	}
	else
	{
		if (pNetIF->dhcp)
		{

			//Stop dhcp client

			dhcp_stop(pNetIF);
		}
	}

#endif

	return	1;
}


static void
InitDone(void* pvArg)
{
	sys_sem_t*	pSem=(sys_sem_t*)pvArg;

	sys_sem_signal(*pSem);
}


static void
Timer(void* pvArg)
{

	//TCP timer.

	tcp_tmr();

	//ARP timer.

	iTimerARP+=TCP_TMR_INTERVAL;
	if	(iTimerARP>=ARP_TMR_INTERVAL)
	{
		iTimerARP-=ARP_TMR_INTERVAL;
		etharp_tmr();
	}
	
#if		defined(PS2IP_DHCP)

	//DHCP timer.

	iTimerDHCP+=TCP_TMR_INTERVAL;
	if ((iTimerDHCP-TCP_TMR_INTERVAL)/DHCP_FINE_TIMER_MSECS!=iTimerDHCP/DHCP_FINE_TIMER_MSECS)
	{
		dhcp_fine_tmr();
	}

	if (iTimerDHCP>=DHCP_COARSE_TIMER_SECS*1000)
	{
		iTimerDHCP-=DHCP_COARSE_TIMER_SECS*1000;
		dhcp_coarse_tmr();
	}
#endif
}


static void
TimerThread(void* pvArg)
{
	while (1)
	{
		tcpip_callback(Timer,NULL);
		DelayThread(TCP_TMR_INTERVAL*1000);
	}
}


static void
InitTimer(void)
{
	iop_thread_t	Thread={TH_C,0,TimerThread,0x800,0x22};
	int				iTimerThreadID=CreateThread(&Thread);

	if (iTimerThreadID<0)
	{
		printf("InitTimer: Fatal error - Failed to create tcpip timer-thread!\n");
		ExitDeleteThread();
	}

	StartThread(iTimerThreadID,NULL);
}


static void
SendARPReply(NetIF* pNetIF,PBuf* pBuf)
{

	//Send out the ARP reply or ARP queued packet.

	if	(pBuf!=NULL)
	{
		pNetIF->linkoutput(pNetIF,pBuf);
		pbuf_free(pBuf);
	}
}


typedef struct InputMSG
{
	PBuf*		pInput;
	NetIF*	pNetIF;
} InputMSG;

#define	MSG_QUEUE_SIZE		16

static InputMSG	aMSGs[MSG_QUEUE_SIZE];
static u8_t			u8FirstMSG=0;
static u8_t			u8LastMSG=0;


static u8_t inline
GetNextMSGQueueIndex(u8_t u8Index)
{
	return	(u8Index+1)%MSG_QUEUE_SIZE;
}


static int inline
IsMSGQueueFull(void)
{
	return	GetNextMSGQueueIndex(u8LastMSG)==u8FirstMSG;
}


static void InputCB ( void* pvArg ) {

 InputMSG* pMSG   = ( InputMSG* )pvArg;
 PBuf*     pInput = pMSG -> pInput;
 NetIF*    pNetIF = pMSG -> pNetIF;
 PBuf*     pARP;
 int       iFlags;

 CpuSuspendIntr ( &iFlags );
  u8FirstMSG = GetNextMSGQueueIndex ( u8FirstMSG );
 CpuResumeIntr ( iFlags );

 switch (   (  ( struct eth_hdr* )( pInput -> payload )  ) -> type   ) {

  case ETHTYPE_IP:

   pARP = etharp_ip_input ( pNetIF, pInput );
   pbuf_header ( pInput, -14 );
   ip_input ( pInput, pNetIF );
   SendARPReply ( pNetIF, pARP );

  break;

  case ETHTYPE_ARP:

   pARP = etharp_arp_input (
    pNetIF, ( struct eth_addr* )&pNetIF -> hwaddr, pInput
   );
   SendARPReply ( pNetIF, pARP );

  break;

  default: pbuf_free ( pInput );

 }  /* end switch */

}  /* end InputCB */

extern sys_mbox_t g_TCPIPMBox;

err_t
ps2ip_input(PBuf* pInput,NetIF* pNetIF)
{

	//When ps2smap receive data, it invokes this function. It'll be called directly by the interrupthandler, which means we are
	//running in an interrupt-context. We'll pass on the data to the tcpip message-thread by adding a callback message. If the
	//messagebox is full, we can't wait for the tcpip-thread to process a message to make room for our message, since we're in an
	//interrupt-context. If the messagebox or messagequeue is full, drop the packet.

	InputMSG*				pIMSG;
	struct tcpip_msg*		pMSG;

	//Is the messagebox or the messagequeue full?

	if	(IsMessageBoxFull(g_TCPIPMBox)||IsMSGQueueFull())
	{

		pbuf_free(pInput);
		return	ERR_OK;
	}

	//Allocate messagequeue entry.

	pIMSG=&aMSGs[u8LastMSG];
	u8LastMSG=GetNextMSGQueueIndex(u8LastMSG);

	//Initialize the InputMSG.

	pIMSG->pInput=pInput;
	pIMSG->pNetIF=pNetIF;

	pMSG=(struct tcpip_msg*)memp_malloc(MEMP_TCPIP_MSG);
	if	(!pMSG)
	{

		pbuf_free(pInput);
		return	ERR_MEM;  
	}
	pMSG->type=TCPIP_MSG_CALLBACK;
	pMSG->msg.cb.f=InputCB;
	pMSG->msg.cb.ctx=pIMSG;

	PostInputMSG(g_TCPIPMBox,pMSG);
	return	ERR_OK;
}


void
ps2ip_Stub(void)
{
}


int
ps2ip_ShutDown(void)
{
	return	1;
}


static void
AddLoopIF(void)
{
	IPAddr	IP;
	IPAddr	NM;
	IPAddr	GW;

	IP4_ADDR(&IP,127,0,0,1);
	IP4_ADDR(&NM,255,0,0,0);
	IP4_ADDR(&GW,127,0,0,1);

	netif_add(&LoopIF,&IP,&NM,&GW,NULL,loopif_init,tcpip_input);
}


int
_start(int argc,char** argv)
{
	sys_sem_t	Sema;

	RegisterLibraryEntries(&_exp_ps2ip);

	sys_init();
	mem_init();
	memp_init();
	pbuf_init();

	netif_init();


	Sema=sys_sem_new(0);
	tcpip_init(InitDone,&Sema);

	sys_arch_sem_wait(Sema,0);
	sys_sem_free(Sema);


	AddLoopIF();
	InitTimer();

	return	MODULE_RESIDENT_END; 
}

#define	SYS_THREAD_PRIO_BASE		0x22

typedef struct Timeout	Timeout;

struct Timeout
{
	struct sys_timeouts	Timeouts;
	int						iTID;
	Timeout*					pNext;
};


#define	SYS_TIMEOUT_MAX	10


static Timeout		aTimeouts[SYS_TIMEOUT_MAX];
static Timeout*	pFreeTimeouts;
static Timeout*	pActiveTimeouts;

sys_thread_t sys_thread_new (
              void ( *pFunction )(  void* ),void* pvArg,int iPrio
             ) {

 iop_thread_t Info = {
  TH_C, 0, pFunction, 0x900, iPrio + SYS_THREAD_PRIO_BASE
 };
 int iThreadID;

 iThreadID = CreateThread ( &Info );

 if	( iThreadID < 0 ) return -1;

 StartThread ( iThreadID, pvArg );

 return iThreadID;

}  /* end sys_thread_new */

sys_mbox_t sys_mbox_new ( void ) {

 sys_mbox_t pMBox;

 pMBox = ( sys_mbox_t )AllocSysMemory (  0, sizeof ( struct sys_mbox ), 0  );

 if ( !pMBox ) return NULL;

 pMBox -> u16First  = pMBox -> u16Last = 0;
 pMBox -> Mail      = sys_sem_new ( 0 );
 pMBox -> Mutex     = sys_sem_new ( 1 );
 pMBox -> iWaitPost = pMBox -> iWaitFetch = 0;

 return	pMBox;

}  /* end sys_mbox_new */

void sys_mbox_free ( sys_mbox_t pMBox ) {

 if	( !pMBox ) return;

 sys_arch_sem_wait ( pMBox -> Mutex, 0 );

 sys_sem_free ( pMBox -> Mail  );
 sys_sem_free ( pMBox -> Mutex );

 FreeSysMemory ( pMBox );

}  /* end sys_mbox_free */

void sys_mbox_post ( sys_mbox_t pMBox, void* pvMSG ) {

 sys_prot_t Flags;

 if	( !pMBox ) return;

 CpuSuspendIntr ( &Flags );

 while (  IsMessageBoxFull ( pMBox )  ) {

  u32_t u32WaitTime;

  ++pMBox -> iWaitPost;

  CpuResumeIntr ( Flags );

  u32WaitTime = sys_arch_sem_wait ( pMBox -> Mail, 0 );

  CpuSuspendIntr ( &Flags );

  --pMBox -> iWaitPost;

  if ( u32WaitTime == SYS_ARCH_TIMEOUT )  {

   CpuResumeIntr ( Flags );
   return;

  }  /* end if */

 }  /* end while */

 pMBox -> apvMSG[ pMBox -> u16Last ] = pvMSG;
 pMBox -> u16Last = GenNextMBoxIndex ( pMBox -> u16Last );

 if	( pMBox -> iWaitFetch > 0 ) SignalSema ( pMBox -> Mail );

 CpuResumeIntr ( Flags );

}  /* end sys_mbox_post */


u32_t
sys_arch_mbox_fetch(sys_mbox_t pMBox,void** ppvMSG,u32_t u32Timeout)
{

	sys_prot_t	Flags;
	u32_t			u32Time=0;

	if	(pMBox==NULL)
	{
		if	(ppvMSG!=NULL)
		{
			*ppvMSG=NULL;
		}
		return	SYS_ARCH_TIMEOUT;
	}

	dbgprintf("sys_arch_mbox_fetch: MBox fetch (TID: %d, MTX: %x)\n",GetThreadId(),pMBox->Mutex);

	CpuSuspendIntr(&Flags);

	while	(IsMessageBoxEmpty(pMBox))
	{
		u32_t		u32WaitTime;

		++pMBox->iWaitFetch;
		CpuResumeIntr(Flags);

		u32WaitTime=sys_arch_sem_wait(pMBox->Mail,u32Timeout);

		CpuSuspendIntr(&Flags);
		--pMBox->iWaitFetch;

		if	(u32WaitTime==SYS_ARCH_TIMEOUT) 
		{

			CpuResumeIntr(Flags);
			return	SYS_ARCH_TIMEOUT;
		}

		u32Time+=u32WaitTime;

		u32Timeout-=u32WaitTime;
	}

	if	(ppvMSG!=NULL)
	{
		*ppvMSG=pMBox->apvMSG[pMBox->u16First];
	}
	pMBox->u16First=GenNextMBoxIndex(pMBox->u16First);

	//Is there a thread waiting for the mbox to become non-full?

	if	(pMBox->iWaitPost>0)
	{

		SignalSema(pMBox->Mail);
	}    

	CpuResumeIntr(Flags);

	return	u32Time;
}     


sys_sem_t
sys_sem_new(u8_t u8Count)
{

	//Create a new semaphore.

	iop_sema_t	Sema={1,1,u8Count,1};
	int			iSema;

	iSema=CreateSema(&Sema);
	if	(iSema<=0)
	{
		return	SYS_SEM_NULL;
	}

	return	iSema;
}


static unsigned int TimeoutHandler ( void* pvArg ) {

 iReleaseWaitThread (  ( int )pvArg  );
 return 0;

}  /* end TimeoutHandler */


static u32_t
ComputeTimeDiff(iop_sys_clock_t* pStart,iop_sys_clock_t* pEnd)
{
	iop_sys_clock_t	Diff;
	int					iSec;
	int					iUSec;
	int					iDiff;

	Diff.lo=pEnd->lo-pStart->lo;
	Diff.hi=pEnd->hi-pStart->hi;

	SysClock2USec(&Diff, (u32 *)&iSec, (u32 *)&iUSec);
	iDiff=(iSec*1000)+(iUSec/1000);

	return	iDiff!=0 ? iDiff:1;
}


u32_t
sys_arch_sem_wait(sys_sem_t Sema,u32_t u32Timeout)
{

	//Wait u32Timeout msec for the Sema to receive a signal.

	dbgprintf("sys_arch_sem_wait: Sema: %d, Timeout: %x (TID: %d)\n",Sema,u32Timeout,GetThreadId());

	if	(u32Timeout==0)
	{ 

		//Wait with no timeouts.

		return	WaitSema(Sema)==0 ? 0:SYS_ARCH_TIMEOUT;
	}
	else if	(u32Timeout==1)
	{

		//Poll.

		return	PollSema(Sema)==0 ? 0:SYS_ARCH_TIMEOUT;
	}
	else
	{

		//Use alarm to timeout.

		iop_sys_clock_t	ClockTicks;
		iop_sys_clock_t	Start;
		iop_sys_clock_t	End;
		int					iPID=GetThreadId();
		u32_t					u32WaitTime;

		GetSystemTime(&Start);
		USec2SysClock(u32Timeout*1000,&ClockTicks);
		SetAlarm(&ClockTicks,TimeoutHandler,(void*)iPID);

		if	(WaitSema(Sema)!=0)
		{
			return	SYS_ARCH_TIMEOUT;
		}
		CancelAlarm(TimeoutHandler,(void*)iPID);
		GetSystemTime(&End);

		u32WaitTime=ComputeTimeDiff(&Start,&End);
		return	u32WaitTime<=u32Timeout ? u32WaitTime:u32Timeout;
	}
}


void
sys_sem_free(sys_sem_t Sema)
{
	if	(Sema==SYS_SEM_NULL)
	{
		return;
	}
	DeleteSema(Sema);
}


void
sys_init(void)
{
	int			iA;
	Timeout**	ppTimeout=&pFreeTimeouts;

	for	(iA=0;iA<SYS_TIMEOUT_MAX;++iA)
	{
		Timeout*		pTimeout=&aTimeouts[iA];
		
		*ppTimeout=pTimeout;
		ppTimeout=&pTimeout->pNext;
	}
	*ppTimeout=NULL;
}


static Timeout**
FindTimeout(int iThreadID)
{

	//Find the Timeout for the thread-id, iThreadID.

	Timeout**	ppTimeout=&pActiveTimeouts;

	while	(*ppTimeout!=NULL)
	{
		if	((*ppTimeout)->iTID==iThreadID)
		{

			//Found it.

			return	ppTimeout;
		}
		ppTimeout=&(*ppTimeout)->pNext;
	}

	//Didn't find it.

	return	ppTimeout;
}


static Timeout**
AllocTimeout(void)
{

	//Allocate a Timeout-struct. Is there any left in the free-list?

	Timeout**	ppTimeout;

	if	(pFreeTimeouts!=NULL)
	{

		//Yes, use the first entry in the free-list.

		return	&pFreeTimeouts;
	}

	//There are no free entries. Then we'll return the LRU-entry, which is the last entry in the active-list.

	ppTimeout=&pActiveTimeouts;
	while	((*ppTimeout)->pNext!=NULL)
	{
		ppTimeout=&(*ppTimeout)->pNext;
	}

	//Before we return the LRU-entry, remove/free the timeout-list.

	while	((*ppTimeout)->Timeouts.next!=NULL)
	{
		struct sys_timeout*	pTimeout=(*ppTimeout)->Timeouts.next;

		(*ppTimeout)->Timeouts.next=pTimeout->next;
		memp_free(MEMP_SYS_TIMEOUT,pTimeout);
	}
	return	ppTimeout;
}


struct sys_timeouts*
sys_arch_timeouts(void)
{

	//Return the timeout-list for this thread.

	int			iThreadID=GetThreadId();
	Timeout**	ppTimeout;
	Timeout*		pTimeout;
	sys_prot_t	Flags;

    CpuSuspendIntr(&Flags);

	//Does it exist an entry for this thread?

	ppTimeout=FindTimeout(iThreadID);
	if	(*ppTimeout==NULL)
	{

		//No, allocate an entry for this thread.

		ppTimeout=AllocTimeout();
		(*ppTimeout)->iTID=iThreadID;
	}

	//The active entries are listed in MRU order. The entry for this thread is the MRU and therefore should be first in the
	//active-list.

	pTimeout=*ppTimeout;
	*ppTimeout=pTimeout->pNext;
	pTimeout->pNext=pActiveTimeouts;
	pActiveTimeouts=pTimeout;

	CpuResumeIntr(Flags);

	//Return the timeout-list.

	return	&pTimeout->Timeouts;
}
