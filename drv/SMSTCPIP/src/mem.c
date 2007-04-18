/** @file
 *
 * Dynamic memory manager
 *
 */
/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
#include "lwip/arch.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/sys.h"
#include "lwip/stats.h"

struct mem {
 mem_size_t next, prev;
 u32_t      used;
};

static u8_t        ram[ MEM_SIZE + sizeof ( struct mem ) + MEM_ALIGNMENT ];
static struct mem* ram_end;
static struct mem* lfree;
static sys_sem_t   mem_sem;

#define MIN_SIZE 12
#define SIZEOF_STRUCT_MEM (                                          \
 sizeof ( struct mem ) +                                             \
 (    (   (  sizeof ( struct mem ) % MEM_ALIGNMENT  ) == 0   ) ? 0 : \
      (   4 - (  sizeof ( struct mem ) % MEM_ALIGNMENT  )   )        \
 )                                                                   \
)

static void plug_holes ( struct mem* mem ) {

 struct mem* nmem;
 struct mem* pmem;

 nmem = ( struct mem* )&ram[ mem -> next ];

 if (  mem != nmem && nmem -> used == 0 && ( u8_t* )nmem != ( u8_t* )ram_end  ) {

  if ( lfree == nmem ) lfree = mem;

  mem -> next = nmem -> next;
  (  ( struct mem* )&ram[ nmem -> next ]  ) -> prev = ( u8_t* )mem - ram;

 }  /* end if */

 pmem = ( struct mem* )&ram[ mem -> prev ];

 if ( pmem != mem && pmem -> used == 0 ) {

  if ( lfree == mem ) lfree = pmem;

  pmem -> next = mem -> next;
  (  ( struct mem* )&ram[ mem -> next ]  ) -> prev = ( u8_t* )pmem - ram;

 }  /* end if */

}  /* end plug_holes */

void mem_init ( void ) {

 struct mem* mem;

 mips_memset ( ram, 0, MEM_SIZE );

 mem = ( struct mem* )ram;
 mem -> next = MEM_SIZE;
 mem -> prev = 0;
 mem -> used = 0;

 ram_end = ( struct mem* )&ram[ MEM_SIZE ];
 ram_end -> used = 1;
 ram_end -> next = MEM_SIZE;
 ram_end -> prev = MEM_SIZE;

 mem_sem = sys_sem_new ( 1 );
 lfree   = ( struct mem* )ram;

}  /* end mem_init */

void mem_free ( void* rmem ) {

 struct mem* mem;

 if ( rmem ) {

  sys_sem_wait ( mem_sem );

  if (   !(  ( u8_t* )rmem < ( u8_t* )ram || ( u8_t* )rmem >= ( u8_t* )ram_end  )   ) {

   mem = (struct mem *)((u8_t *)rmem - SIZEOF_STRUCT_MEM);
   mem -> used = 0;

   if ( mem < lfree ) lfree = mem;

   plug_holes ( mem );

  }  /* end if */

  sys_sem_signal ( mem_sem );

 }  /* end if */

}  /* end mem_free */

void* mem_realloc ( void* rmem, mem_size_t newsize ) {

 mem_size_t  size;
 mem_size_t  ptr, ptr2;
 struct mem* mem, *mem2;

 if ( newsize % MEM_ALIGNMENT )
  newsize += MEM_ALIGNMENT - (  ( newsize + SIZEOF_STRUCT_MEM ) % MEM_ALIGNMENT  );

 if ( newsize > MEM_SIZE ) return NULL;

 sys_sem_wait ( mem_sem );

 if (   !(  ( u8_t* )rmem < ( u8_t* )ram || ( u8_t* )rmem >= ( u8_t* )ram_end  )   ) {

  mem  = ( struct mem* )(  ( u8_t* )rmem - SIZEOF_STRUCT_MEM  );
  ptr  = ( u8_t* )mem - ram;
  size = mem -> next - ptr - SIZEOF_STRUCT_MEM;

  if ( newsize + SIZEOF_STRUCT_MEM + MIN_SIZE < size ) {

   ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
   mem2 = ( struct mem* )&ram[ ptr2 ];
   mem2 -> used = 0;
   mem2 -> next = mem -> next;
   mem2 -> prev = ptr;
   mem  -> next = ptr2;

   if ( mem2 -> next != MEM_SIZE ) (  ( struct mem* )&ram[ mem2 -> next ] ) -> prev = ptr2;

   plug_holes ( mem2 );

  }  /* end if */

 }  /* end if */

 sys_sem_signal ( mem_sem );

 return rmem;

}  /* end mem_realloc */

void* mem_malloc ( mem_size_t size ) {

 mem_size_t  ptr,  ptr2;
 struct mem* mem, *mem2;
 void*       retVal = NULL;

 if ( size  ) {

  if ( size % MEM_ALIGNMENT )
   size += MEM_ALIGNMENT - (  ( size + SIZEOF_STRUCT_MEM ) % MEM_ALIGNMENT  );

  if ( size <= MEM_SIZE ) {

   sys_sem_wait ( mem_sem );

   for (  ptr = ( u8_t* )lfree - ram; ptr < MEM_SIZE; ptr = (  ( struct mem* )&ram[ ptr ] ) -> next  ) {

    mem = ( struct mem* )&ram[ ptr ];

    if (  !mem -> used && mem -> next - ( ptr + SIZEOF_STRUCT_MEM ) >= size + SIZEOF_STRUCT_MEM  ) {

     ptr2 = ptr + SIZEOF_STRUCT_MEM + size;
     mem2 = ( struct mem* )&ram[ ptr2 ];

     mem2 -> prev = ptr;
     mem2 -> next = mem -> next;
     mem  -> next = ptr2;

     if ( mem2 -> next != MEM_SIZE ) (  ( struct mem* )&ram[ mem2 -> next ]  ) -> prev = ptr2;

     mem2 -> used = 0;
     mem  -> used = 1;

     if ( mem == lfree ) while ( lfree -> used && lfree != ram_end )
      lfree = ( struct mem* )&ram[ lfree -> next ];

     retVal = ( u8_t* )mem + SIZEOF_STRUCT_MEM;
     break;

    }  /* end if */

   }  /* end for */

   sys_sem_signal ( mem_sem );

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end mem_malloc */
