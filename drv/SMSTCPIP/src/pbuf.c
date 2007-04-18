/**
 * @file
 * Packet buffer management
 *
 * Packets are built from the pbuf data structure. It supports dynamic
 * memory allocation for packet contents or can reference externally
 * managed packet contents both in RAM and ROM. Quick allocation for
 * incoming packets is provided through pools with fixed sized pbufs.
 *
 * A packet may span over multiple pbufs, chained as a singly linked
 * list. This is called a "pbuf chain".
 *
 * Multiple packets may be queued, also using this singly linked list.
 * This is called a "packet queue". So, a packet queue consists of one
 * or more pbuf chains, each of which consist of one or more pbufs.
 * The differences between a pbuf chain and a packet queue are very
 * subtle. Currently, queues are only supported in a limited section
 * of lwIP, this is the etharp queueing code. Outside of this section
 * no packet queues are supported as of yet.
 *
 * The last pbuf of a packet has a ->tot_len field that equals the
 * ->len field. It can be found by traversing the list. If the last
 * pbuf of a packet has a ->next field other than NULL, more packets
 * are on the queue.
 *
 * Therefore, looping through a pbuf of a single packet, has an
 * loop end condition (tot_len == p->len), NOT (next == NULL).
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

#include "lwip/stats.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"

#include "lwip/sys.h"

#include "sysclib.h"

static u8_t pbuf_pool_memory[(PBUF_POOL_SIZE * MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE + sizeof(struct pbuf)))];

#if !SYS_LIGHTWEIGHT_PROT
static volatile u8_t pbuf_pool_free_lock, pbuf_pool_alloc_lock;
static sys_sem_t pbuf_pool_free_sem;
#endif

static struct pbuf *pbuf_pool = NULL;

/**
 * Initializes the pbuf module.
 *
 * A large part of memory is allocated for holding the pool of pbufs.
 * The size of the individual pbufs in the pool is given by the size
 * parameter, and the number of pbufs in the pool by the num parameter.
 *
 * After the memory has been allocated, the pbufs are set up. The
 * ->next pointer in each pbuf is set up to point to the next pbuf in
 * the pool.
 *
 */
void
pbuf_init(void)
{
  struct pbuf *p, *q = NULL;
  u16_t i;

  pbuf_pool = (struct pbuf *)&pbuf_pool_memory[0];
  LWIP_ASSERT("pbuf_init: pool aligned", (long)pbuf_pool % MEM_ALIGNMENT == 0);

#if PBUF_STATS
  lwip_stats.pbuf.avail = PBUF_POOL_SIZE;
#endif /* PBUF_STATS */

  /* Set up ->next pointers to link the pbufs of the pool together */
  p = pbuf_pool;

  for(i = 0; i < PBUF_POOL_SIZE; ++i) {
    p->next = (struct pbuf *)((u8_t *)p + PBUF_POOL_BUFSIZE + sizeof(struct pbuf));
    p->len = p->tot_len = PBUF_POOL_BUFSIZE;
    p->payload = MEM_ALIGN((void *)((u8_t *)p + sizeof(struct pbuf)));
    p->flags = PBUF_FLAG_POOL;
    q = p;
    p = p->next;
  }

  /* The ->next pointer of last pbuf is NULL to indicate that there
     are no more pbufs in the pool */
  q->next = NULL;

#if !SYS_LIGHTWEIGHT_PROT
  pbuf_pool_alloc_lock = 0;
  pbuf_pool_free_lock = 0;
  pbuf_pool_free_sem = sys_sem_new(1);
#endif
}

/**
 * @internal only called from pbuf_alloc()
 */
static struct pbuf *
pbuf_pool_alloc(void)
{
  struct pbuf *p = NULL;

  SYS_ARCH_DECL_PROTECT(old_level);
  SYS_ARCH_PROTECT(old_level);

    p = pbuf_pool;
    if (p) {
      pbuf_pool = p->next;
    }

  SYS_ARCH_UNPROTECT(old_level);
  return p;
}


/**
 * Allocates a pbuf.
 *
 * The actual memory allocated for the pbuf is determined by the
 * layer at which the pbuf is allocated and the requested size
 * (from the size parameter).
 *
 * @param flag this parameter decides how and where the pbuf
 * should be allocated as follows:
 *
 * - PBUF_RAM: buffer memory for pbuf is allocated as one large
 *             chunk. This includes protocol headers as well.
 * - PBUF_ROM: no buffer memory is allocated for the pbuf, even for
 *             protocol headers. Additional headers must be prepended
 *             by allocating another pbuf and chain in to the front of
 *             the ROM pbuf. It is assumed that the memory used is really
 *             similar to ROM in that it is immutable and will not be
 *             changed. Memory which is dynamic should generally not
 *             be attached to PBUF_ROM pbufs. Use PBUF_REF instead.
 * - PBUF_REF: no buffer memory is allocated for the pbuf, even for
 *             protocol headers. It is assumed that the pbuf is only
 *             being used in a single thread. If the pbuf gets queued,
 *             then pbuf_take should be called to copy the buffer.
 * - PBUF_POOL: the pbuf is allocated as a pbuf chain, with pbufs from
 *              the pbuf pool that is allocated during pbuf_init().
 *
 * @return the allocated pbuf. If multiple pbufs where allocated, this
 * is the first pbuf of a pbuf chain.
 */
struct pbuf* pbuf_alloc ( pbuf_layer l, u16_t length, pbuf_flag flag ) {

 struct pbuf* p, *q, *r;
 u16_t        offset;
 s32_t        rem_len; /* remaining length */

 offset = 0;

 switch ( l ) {

  case PBUF_TRANSPORT: offset += PBUF_TRANSPORT_HLEN;
  case PBUF_IP       : offset += PBUF_IP_HLEN;
  case PBUF_LINK     : offset += PBUF_LINK_HLEN;
  case PBUF_RAW      : break;
  default            : return NULL;

 }  /* end switch */

 switch ( flag ) {

  case PBUF_POOL:

   p = pbuf_pool_alloc ();

   if ( !p ) return NULL;

   p -> next    = NULL;
   p -> payload = MEM_ALIGN(    ( void* )(   ( u8_t* )p + (  sizeof ( struct pbuf ) + offset  )   )    );
   p -> tot_len = length;
   p -> len     = length;

  break;

  case PBUF_RAM:

   p = mem_malloc (   MEM_ALIGN_SIZE(  sizeof ( struct pbuf ) + offset  ) + MEM_ALIGN_SIZE( length )   );

   if ( !p ) return NULL;

   p -> payload = MEM_ALIGN(   ( void* )(  ( u8_t* )p + sizeof ( struct pbuf ) + offset  )   );
   p -> len     = p -> tot_len = length;
   p -> next    = NULL;
   p -> flags   = PBUF_FLAG_RAM;

  break;

  case PBUF_ROM:
  case PBUF_REF:

   p = memp_malloc ( MEMP_PBUF );

   if ( !p ) return NULL;

   p -> payload = NULL;
   p -> len     = p -> tot_len = length;
   p -> next    = NULL;
   p -> flags   = ( flag == PBUF_ROM ? PBUF_FLAG_ROM: PBUF_FLAG_REF );

  break;

  default: return NULL;

 }  /* end switch */

 p -> ref = 1;

 return p;

}  /* end pbuf_alloc */

#define PBUF_POOL_FAST_FREE( p ) p -> next = pbuf_pool; pbuf_pool = p;
#define PBUF_POOL_FREE( p ) { SYS_ARCH_DECL_PROTECT( old_level ); \
                              SYS_ARCH_PROTECT( old_level );      \
                              PBUF_POOL_FAST_FREE( p );           \
                              SYS_ARCH_UNPROTECT(old_level);      \
                            }
/**
 * Shrink a pbuf chain to a desired length.
 *
 * @param p pbuf to shrink.
 * @param new_len desired new length of pbuf chain
 *
 * Depending on the desired length, the first few pbufs in a chain might
 * be skipped and left unchanged. The new last pbuf in the chain will be
 * resized, and any remaining pbufs will be freed.
 *
 * @note If the pbuf is ROM/REF, only the ->tot_len and ->len fields are adjusted.
 * @note May not be called on a packet queue.
 *
 * @bug Cannot grow the size of a pbuf (chain) (yet).
 */
void pbuf_realloc ( struct pbuf* p, u16_t new_len ) {

 struct pbuf* q;
 u16_t  rem_len;
 s16_t  grow;

 if ( new_len >= p -> tot_len ) return;

 grow    = new_len - p -> tot_len;
 rem_len = new_len;
 q       = p;

 while ( rem_len > q -> len ) {
  rem_len    -= q -> len;
  q->tot_len += grow;
  q           = q -> next;
 }  /* end while */

 if (  ( q -> flags == PBUF_FLAG_RAM ) && ( rem_len != q -> len )  )

  mem_realloc (  q, ( u8_t* )q -> payload - ( u8_t* )q + rem_len  );

 q -> len     = rem_len;
 q -> tot_len = q -> len;

 if ( q -> next ) pbuf_free ( q -> next );

 q -> next = NULL;

}  /* end pbuf_realloc */
/**
 * Adjusts the payload pointer to hide or reveal headers in the payload.
 *
 * Adjusts the ->payload pointer so that space for a header
 * (dis)appears in the pbuf payload.
 *
 * The ->payload, ->tot_len and ->len fields are adjusted.
 *
 * @param hdr_size Number of bytes to increment header size which
 * increases the size of the pbuf. New space is on the front.
 * (Using a negative value decreases the header size.)
 *
 * PBUF_ROM and PBUF_REF type buffers cannot have their sizes increased, so
 * the call will fail. A check is made that the increase in header size does
 * not move the payload pointer in front of the start of the buffer.
 * @return 1 on failure, 0 on success.
 *
 * @note May not be called on a packet queue.
 */
u8_t pbuf_header ( struct pbuf* p, s16_t header_size ) {

 void* payload = p -> payload;

 if ( p -> flags == PBUF_FLAG_RAM || p -> flags == PBUF_FLAG_POOL ) {

  p -> payload = ( u8_t* )p -> payload - header_size;

  if (  ( u8_t* )p -> payload < ( u8_t* )p + sizeof ( struct pbuf )  ) {
   p -> payload = payload;
   return 1;
  }  /* end if */
 
 } else if ( p -> flags == PBUF_FLAG_REF || p -> flags == PBUF_FLAG_ROM ) {

  if (  ( header_size < 0 ) && ( header_size - p -> len <= 0 )  )
   p -> payload = ( u8_t* )p -> payload - header_size;
  else return 1;

 }  /* end if */

 p -> len     += header_size;
 p -> tot_len += header_size;

 return 0;

}  /* end pbuf_header */
/**
 * Dereference a pbuf (chain) and deallocate any no-longer-used
 * pbufs at the head of this chain.
 *
 * Decrements the pbuf reference count. If it reaches
 * zero, the pbuf is deallocated.
 *
 * For a pbuf chain, this is repeated for each pbuf in the chain,
 * up to a pbuf which has a non-zero reference count after
 * decrementing. (This might de-allocate the whole chain.)
 *
 * @param pbuf The pbuf (chain) to be dereferenced.
 *
 * @return the number of pbufs that were de-allocated
 * from the head of the chain.
 *
 * @note MUST NOT be called on a packet queue.
 * @note the reference counter of a pbuf equals the number of pointers
 * that refer to the pbuf (or into the pbuf).
 *
 * @internal examples:
 *
 * Assuming existing chains a->b->c with the following reference
 * counts, calling pbuf_free(a) results in:
 * 
 * 1->2->3 becomes ...1->3
 * 3->3->3 becomes 2->3->3
 * 1->1->2 becomes ......1
 * 2->1->1 becomes 1->1->1
 * 1->1->1 becomes .......
 *
 */
u8_t pbuf_free ( struct pbuf* p ) {

 struct pbuf* q;
 u8_t         count;

 SYS_ARCH_DECL_PROTECT( old_level );

 if ( !p ) return 0;

 count = 0;

 SYS_ARCH_PROTECT( old_level );

 while ( p ) {

  --p -> ref;

  if ( p->ref == 0 ) {
 
   q = p -> next;

   if ( p -> flags == PBUF_FLAG_POOL ) {
    p -> len     = p -> tot_len = PBUF_POOL_BUFSIZE;
    p -> payload = ( void* )(  ( u8_t* )p + sizeof ( struct pbuf )  );
    PBUF_POOL_FAST_FREE( p );
   } else if ( p -> flags == PBUF_FLAG_ROM || p -> flags == PBUF_FLAG_REF )
    memp_free ( MEMP_PBUF, p );
   else mem_free ( p );

   ++count;
   p = q;

  } else p = NULL;

 }  /* end while */

 SYS_ARCH_UNPROTECT( old_level );

 return count;

}  /* end pbuf_free */
/**
 * Count number of pbufs in a chain
 *
 * @param p first pbuf of chain
 * @return the number of pbufs in a chain
 */
u8_t pbuf_clen ( struct pbuf* p ) {

 u8_t len = 0;

 while ( p ) {
  ++len;
  p = p -> next;
 }  /* end while */

 return len;

}  /* end pbuf_clen */
/**
 * Increment the reference count of the pbuf.
 *
 * @param p pbuf to increase reference counter of
 *
 */
void pbuf_ref ( struct pbuf* p ) {

 if ( p ) {
  SYS_ARCH_DECL_PROTECT( old_level );
  SYS_ARCH_PROTECT( old_level );
  ++p -> ref;
  SYS_ARCH_UNPROTECT( old_level );
 }  /* end if */

}  /* end pbuf_ref */
/**
 * Concatenate two pbufs (each may be a pbuf chain) and take over
 * the caller's reference of the tail pbuf.
 * 
 * @note The caller MAY NOT reference the tail pbuf afterwards.
 * Use pbuf_chain() for that purpose.
 * 
 * @see pbuf_chain()
 */
void pbuf_cat ( struct pbuf* h, struct pbuf* t ) {

 if ( h && t ) {

  struct pbuf* p;

  for ( p = h; p -> next; p = p -> next ) p -> tot_len += t -> tot_len;

  p -> tot_len += t -> tot_len;
  p -> next     = t;

 }  /* end if */

}  /* end pbuf_cat */

/**
 * Chain two pbufs (or pbuf chains) together.
 * 
 * The caller MUST call pbuf_free(t) once it has stopped
 * using it. Use pbuf_cat() instead if you no longer use t.
 * 
 * @param h head pbuf (chain)
 * @param t tail pbuf (chain)
 * @note The pbufs MUST belong to the same packet.
 * @note MAY NOT be called on a packet queue.
 *
 * The ->tot_len fields of all pbufs of the head chain are adjusted.
 * The ->next field of the last pbuf of the head chain is adjusted.
 * The ->ref field of the first pbuf of the tail chain is adjusted.
 *
 */
void pbuf_chain ( struct pbuf* h, struct pbuf* t ) {

 pbuf_cat ( h, t );
 pbuf_ref ( t );

}  /* end pbuf_chain */
/**
 *
 * Create PBUF_POOL (or PBUF_RAM) copies of PBUF_REF pbufs.
 *
 * Used to queue packets on behalf of the lwIP stack, such as
 * ARP based queueing.
 *
 * Go through a pbuf chain and replace any PBUF_REF buffers
 * with PBUF_POOL (or PBUF_RAM) pbufs, each taking a copy of
 * the referenced data.
 *
 * @note You MUST explicitly use p = pbuf_take(p);
 * The pbuf you give as argument, may have been replaced
 * by pbuf_take()!
 *
 * @note Any replaced pbufs will be freed through pbuf_free().
 * This may deallocate them if they become no longer referenced.
 *
 * @param p Head of pbuf chain to process
 *
 * @return Pointer to head of pbuf chain
 */
struct pbuf* pbuf_take ( struct pbuf* p ) {

 struct pbuf* q , *prev, *head;

 prev = NULL;
 head = p;

 do {

  if ( p -> flags == PBUF_FLAG_REF ) {
   if ( p -> len <= PBUF_POOL_BUFSIZE )
    q = pbuf_alloc ( PBUF_RAW, p -> len, PBUF_POOL );
   else q = NULL;

   if ( !q ) q = pbuf_alloc ( PBUF_RAW, p -> len, PBUF_RAM );

   if ( q ) {

    q -> next = p -> next;
    p -> next = NULL;

    if ( prev )
     prev -> next = q;
    else head = q;

    mips_memcpy ( q -> payload, p -> payload, p -> len );

    q -> tot_len = p -> tot_len;
    q -> len     = p -> len;

    pbuf_free ( p );
    p = q;

   } else {

    pbuf_free ( head );
    return NULL;

   }  /* end else */

  }  /* end if */

  prev = p;
  p    = p -> next;

 } while ( p );

 return head;

}  /* end pbuf_take */
