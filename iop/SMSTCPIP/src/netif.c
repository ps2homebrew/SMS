/**
 * @file
 *
 * lwIP network interface abstraction
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"

struct netif *netif_list = NULL;
struct netif *netif_default = NULL;
/**
 * Add a network interface to the list of lwIP netifs.
 *
 * @param ipaddr IP address for the new netif
 * @param netmask network mask for the new netif
 * @param gw default gateway IP address for the new netif
 * @param state opaque data passed to the new netif
 * @param init callback function that initializes the interface
 * @param input callback function that...
 *
 * @return netif, or NULL if failed.
 */
struct netif* netif_add (
               struct netif* netif, struct ip_addr* ipaddr,
               struct ip_addr* netmask, struct ip_addr* gw,
               void* state, err_t ( *init ) ( struct netif* ),
               err_t ( *input ) ( struct pbuf*, struct netif* )
              ) {

 static int netifnum = 0;
#if LWIP_DHCP
 netif -> dhcp = NULL;
#endif  /* LWIP_DHCP */
 netif -> state = state;
 netif -> num   = netifnum++;
 netif -> input = input;

 netif_set_addr ( netif, ipaddr, netmask, gw );

 if (  init ( netif ) != ERR_OK  ) return NULL;

 netif -> next = netif_list;
 netif_list    = netif;

 return netif;

}  /* end netif_add */

void netif_set_addr (
      struct netif* netif, struct ip_addr* ipaddr,
      struct ip_addr* netmask, struct ip_addr* gw
     ) {

 netif_set_ipaddr  ( netif, ipaddr  );
 netif_set_netmask ( netif, netmask );
 netif_set_gw      ( netif, gw      );

}  /* end netif_set_addr */

void netif_remove ( struct netif* netif ) {

 if ( netif_list == netif )
  netif_list = netif -> next;
 else {
  struct netif* tmpNetif;
  for ( tmpNetif = netif_list; tmpNetif; tmpNetif = tmpNetif -> next ) {
   if ( tmpNetif -> next == netif ) {
    tmpNetif -> next = netif -> next;
    break;
   }  /* end if */
  }  /* end for */
  if ( !tmpNetif ) return;
 }  /* end else */

 if ( netif_default == netif ) netif_default = NULL;

}  /* end netif_remove */

struct netif* netif_find ( char* name ) {

 struct netif* netif;
 u8_t          num;

 num = name[ 2 ] - '0';

 for ( netif = netif_list; netif; netif = netif -> next )
  if ( num == netif -> num             &&
       name[ 0 ] == netif -> name[ 0 ] &&
       name[ 1 ] == netif -> name[ 1 ]
  ) return netif;

 return NULL;

}  /* end netif_find */

void netif_set_ipaddr ( struct netif* netif, struct ip_addr* ipaddr ) {
#if LWIP_TCP
 struct tcp_pcb*        pcb;
 struct tcp_pcb_listen* lpcb;

  /* address is actually being changed? */
  if ((ip_addr_cmp(ipaddr, &(netif->ip_addr))) == 0)
  {
    extern struct tcp_pcb *tcp_active_pcbs;
    LWIP_DEBUGF(NETIF_DEBUG | 1, ("netif_set_ipaddr: netif address being changed\n"));
    pcb = tcp_active_pcbs;
    while (pcb != NULL) {
      /* PCB bound to current local interface address? */
      if (ip_addr_cmp(&(pcb->local_ip), &(netif->ip_addr))) {
        /* this connection must be aborted */
        struct tcp_pcb *next = pcb->next;
        LWIP_DEBUGF(NETIF_DEBUG | 1, ("netif_set_ipaddr: aborting TCP pcb %p\n", (void *)pcb));
        tcp_abort(pcb);
        pcb = next;
      } else {
        pcb = pcb->next;
      }
    }
    for (lpcb = tcp_listen_pcbs; lpcb != NULL; lpcb = lpcb->next) {
      /* PCB bound to current local interface address? */
      if (ip_addr_cmp(&(lpcb->local_ip), &(netif->ip_addr))) {
        /* The PCB is listening to the old ipaddr and
         * is set to listen to the new one instead */
        ip_addr_set(&(lpcb->local_ip), ipaddr);
      }
    }
  }
#endif
 ip_addr_set (  &( netif -> ip_addr ), ipaddr  );

}

void netif_set_gw ( struct netif* netif, struct ip_addr* gw ) {
 ip_addr_set (  &( netif -> gw), gw  );
}  /* end netif_set_gw */

void netif_set_netmask ( struct netif* netif, struct ip_addr* netmask ) {
 ip_addr_set (  &( netif -> netmask ), netmask  );
}  /* end netif_set_netmask */

void netif_set_default ( struct netif* netif ) {
 netif_default = netif;
}  /* end netif_set_default */

void netif_init ( void ) {
 netif_list = netif_default = NULL;
}  /* end netif_init */
