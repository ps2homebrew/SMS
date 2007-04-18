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
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/raw.h"
#include "lwip/tcp.h"
#include "lwip/api.h"
#include "lwip/api_msg.h"
#include "lwip/tcpip.h"
#include "lwip/sys.h"
#include "lwip/stats.h"

struct memp {
 struct memp* next;
};

static struct memp* memp_tab[ MEMP_MAX ];

static const u16_t memp_sizes[ MEMP_MAX ] = {
 sizeof ( struct pbuf           ),
 sizeof ( struct raw_pcb        ),
 sizeof ( struct udp_pcb        ),
 sizeof ( struct tcp_pcb        ),
 sizeof ( struct tcp_pcb_listen ),
 sizeof ( struct tcp_seg        ),
 sizeof ( struct netbuf         ),
 sizeof ( struct netconn        ),
 sizeof ( struct api_msg        ),
 sizeof ( struct tcpip_msg      )
};

static const u16_t memp_num[ MEMP_MAX ] = {
 MEMP_NUM_PBUF,
 MEMP_NUM_RAW_PCB,
 MEMP_NUM_UDP_PCB,
 MEMP_NUM_TCP_PCB,
 MEMP_NUM_TCP_PCB_LISTEN,
 MEMP_NUM_TCP_SEG,
 MEMP_NUM_NETBUF,
 MEMP_NUM_NETCONN,
 MEMP_NUM_API_MSG,
 MEMP_NUM_TCPIP_MSG
};

static u8_t memp_memory[
 ( MEMP_NUM_PBUF           * MEM_ALIGN_SIZE(  sizeof ( struct pbuf           ) + sizeof ( struct memp )  ) +
   MEMP_NUM_RAW_PCB        * MEM_ALIGN_SIZE(  sizeof ( struct raw_pcb        ) + sizeof ( struct memp )  ) +
   MEMP_NUM_UDP_PCB        * MEM_ALIGN_SIZE(  sizeof ( struct udp_pcb        ) + sizeof ( struct memp )  ) +
   MEMP_NUM_TCP_PCB        * MEM_ALIGN_SIZE(  sizeof ( struct tcp_pcb        ) + sizeof ( struct memp )  ) +
   MEMP_NUM_TCP_PCB_LISTEN * MEM_ALIGN_SIZE(  sizeof ( struct tcp_pcb_listen ) + sizeof ( struct memp )  ) +
   MEMP_NUM_TCP_SEG        * MEM_ALIGN_SIZE(  sizeof ( struct tcp_seg        ) + sizeof ( struct memp )  ) +
   MEMP_NUM_NETBUF         * MEM_ALIGN_SIZE(  sizeof ( struct netbuf         ) + sizeof ( struct memp )  ) +
   MEMP_NUM_NETCONN        * MEM_ALIGN_SIZE(  sizeof ( struct netconn        ) + sizeof ( struct memp )  ) +
   MEMP_NUM_API_MSG        * MEM_ALIGN_SIZE(  sizeof ( struct api_msg        ) + sizeof ( struct memp )  ) +
   MEMP_NUM_TCPIP_MSG      * MEM_ALIGN_SIZE(  sizeof ( struct tcpip_msg      ) + sizeof ( struct memp )  )
 )
];

void memp_init ( void ) {

 struct memp* m, *memp;
 u16_t  i, j;
 u16_t  size;
      
 memp = ( struct memp* )&memp_memory[ 0 ];

 for( i = 0; i < MEMP_MAX; ++i ) {

  size = MEM_ALIGN_SIZE(  memp_sizes[ i ] + sizeof ( struct memp )  );

  if ( memp_num[ i ] > 0 ) {

   memp_tab[ i ] = memp;
   m             = memp;
      
   for ( j = 0; j < memp_num[ i ]; ++j ) {

    m -> next = ( struct memp* )MEM_ALIGN(  ( u8_t* )m + size  );
    memp      = m;
    m         = m -> next;

   }  /* end for */

   memp -> next = NULL;
   memp         = m;

  } else memp_tab[ i ] = NULL;

 }  /* end for */
  
}  /* end memp_init */

void* memp_malloc ( memp_t type ) {

 struct memp* memp;
 void*        mem;

 SYS_ARCH_DECL_PROTECT(old_level);
 SYS_ARCH_PROTECT( old_level );

 memp = memp_tab[ type ];
  
 if ( memp ) {

  memp_tab[ type ] = memp -> next;
  memp -> next     = NULL;
  mem              = MEM_ALIGN(  ( u8_t* )memp + sizeof ( struct memp )  );

 } else mem = NULL;

 SYS_ARCH_UNPROTECT( old_level );

 return mem;

}  /* end memp_malloc */

void memp_free ( memp_t type, void* mem ) {

 if ( mem ) {

  struct memp* memp;

  SYS_ARCH_DECL_PROTECT( old_level );
 
  memp = ( struct memp* )(  ( u8_t* )mem - sizeof ( struct memp )  );

  SYS_ARCH_PROTECT( old_level );

  memp -> next     = memp_tab[ type ];
  memp_tab[ type ] = memp;

  SYS_ARCH_UNPROTECT( old_level );

 }  /* end if */

}  /* end memp_free */
