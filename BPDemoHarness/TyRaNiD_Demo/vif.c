#include <tamtypes.h>
#include <kernel.h>
#include <string.h>
#include "shapes.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "ee_malloc.h"
#include "../harness.h"
#include "utils.h"

// Size of dmabuffer in 32bit units
#define DMABUF_SIZE 16384*4
#define CHAINBUF_SIZE 64

static u32 buf_no;
static u32 buf_loc;
static u32 *dma_data[2];
extern demo_init_t *init;

void vu1_clearmicro()
{
  int t;
  volatile u64 *dest = (u64 *)vu1_micro;
  for(t=0; t < 2048; t++)
    {
      dest[t] = 0;
    }
}

void vu1_dumpmicro(u32 addr, u32 len)

{
   int loop;
   volatile u64 *data = (u64 *)vu1_micro;

   for(loop = addr; loop < (addr+len); loop++)
   {
      init->printf("%04X: %08X - %08X\n", loop, data[loop] >> 32, data[loop] & 0xFFFFFFFF);
   }
}

void vu1_cleardata()
{
  int t;
  volatile vertex *dest = (vertex *)vu1_data;
  for(t=0; t < 1024; t++)
    {
      dest[t].x = 0;
      dest[t].y = 0;
      dest[t].z = 0;
      dest[t].w = 0;
    }
}

void vu1_dumpdata(u32 addr, u32 len)

{
  int loop;
  volatile vertex *data = (vertex *)vu1_data;

  for(loop = addr; loop < (addr+len); loop++)
    {
      init->printf("%04X: %f, %f, %f, %f\n", loop, data[loop].x, data[loop].y, data[loop].z, data[loop].w);
    }
}

void vu1_dumpdata_i(u32 addr, u32 len)

{
  int loop;
  volatile u32 *data = (u32 *)vu1_data;

  for(loop = addr; loop < (addr+len); loop++)
    {
      init->printf("%04X: %08X, %08X, %08X, %08X\n", loop, data[loop*4], data[loop*4 + 1], data[loop* 4 + 2], data[loop*4 + 3]);
    }
}

void vu1_wait()
{
   //2 nops before coprocessor 2 wait?
asm __volatile__(
   "nop\n" \
   "nop\n" \
   "0:\n" \
   "bc2t 0b\n" \
   "nop\n" \
   "nop");
}

void init_vu1()

{
  vu1_clearmicro();
  vu1_cleardata();
  
  dma_data[0] = ee_malloc_aligned(DMABUF_SIZE, 16);
  dma_data[1] = ee_malloc_aligned(DMABUF_SIZE, 16);
  buf_loc = 0;
  buf_no = 0;
 
  memset(dma_data[0], 0, DMABUF_SIZE * 4);
  memset(dma_data[1], 0, DMABUF_SIZE * 4);
  SET_CHCR(VIF1_CHCR, 1, 1, 2, 1, 0, 0, 0);
  *(u32 *) vif1_fbrst = 1;
  *(u32 *) vif1_cycle = 0x404;

  //*((u32 *) vif1_err) = 0x7; /* Kluge factor 9. Remove error stalls */
}

void vu1_upload_code(const u64 *code, u32 addr, u32 len)
     /* len in 8 byte units, code should be qword aligned and padded */

{
  init->printf("*code = %p, addr = %08X, len = %d\n", code, addr, len);

  len = (len + 1) & ~1; /* Adjust data size to next 128bit boundary */

  while(len > 0)
    {
      if(buf_loc > (DMABUF_SIZE - 4)) /* Cannot fit in another block of data */
	{
	  vu1_send_chain();
	}

      if(len >= 256)
	{
	  dma_data[buf_no][buf_loc++] = CTAG(128, REF);
	  dma_data[buf_no][buf_loc++] = (u32) code;
	  dma_data[buf_no][buf_loc++] = VIF_TAG(NOP, 0, 0);
	  dma_data[buf_no][buf_loc++] = VIF_TAG(MPG, 0, addr);
	  len -= 256;
	  addr += 256;
	  code += 256;
	}
      else
	{
	  dma_data[buf_no][buf_loc++] = CTAG(len / 2, REF);
	  dma_data[buf_no][buf_loc++] = (u32) code;
	  dma_data[buf_no][buf_loc++] = VIF_TAG(NOP, 0, 0);
	  dma_data[buf_no][buf_loc++] = VIF_TAG(MPG, len, addr);
	  len = 0;	  
	}
    }
}
 
void dump_dma()

{
   int loop;

   for(loop = 0; loop < buf_loc; loop++)
   {
      init->printf("%d: %08X %s\n", loop, dma_data[buf_no][loop], getasbits(dma_data[buf_no][loop]));
   }
}

void vu1_upload_data(const vertex *data, u32 addr, u32 len, int db_mode)
     /* len in 128bit units, uses UNPACK V4-32 */

{
  u32 addr_flags = 0;

  addr &= 0x3FF;
  if(db_mode)
    {
      addr_flags = UNPACK_DB;
    }

  //init->printf("data %p, addr %04X, len %d\n", data, addr, len);
  while(len > 0)
    {
      if(buf_loc > (DMABUF_SIZE - 4)) /* Cannot fit in another block of data */
	{
	  vu1_send_chain();
	}
      
      if(len >= 256)
	{
	  dma_data[buf_no][buf_loc++] = CTAG(256, REF);
	  dma_data[buf_no][buf_loc++] = (u32) data;
	  dma_data[buf_no][buf_loc++] = VIF_TAG(STCYCL, 0, 0x404);
	  dma_data[buf_no][buf_loc++] = VIF_TAG(UNPACK_V4_32, 0, addr | addr_flags);
	  len -= 256;
	  addr += 256;
	  data += 256;
	}
      else
	{
	  dma_data[buf_no][buf_loc++] = CTAG(len, REF);
	  dma_data[buf_no][buf_loc++] = (u32) data;
	  dma_data[buf_no][buf_loc++] = VIF_TAG(STCYCL, 0, 0x404);
	  dma_data[buf_no][buf_loc++] = VIF_TAG(UNPACK_V4_32, len, addr | addr_flags);
	  len = 0;
	}
    }
}

void vu1_upload_data_copy(const vertex *data, u32 addr, u32 len, int db_mode)

{  
  u32 addr_flags = 0;
  u32 *p_data;

  addr &= 0x3FF;
  p_data = (u32 *) data;
  if(db_mode)
    {
      addr_flags = UNPACK_DB;
    }

  if(buf_loc > (DMABUF_SIZE - 4 - (len << 2))) /* Cannot fit in another block of data */
    {
      vu1_send_chain();
    }
  
  dma_data[buf_no][buf_loc++] = CTAG(len, CNT);
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = VIF_TAG(STCYCL, 0, 0x404);
  dma_data[buf_no][buf_loc++] = VIF_TAG(UNPACK_V4_32, len, addr | addr_flags);
  while(len)
    {
      dma_data[buf_no][buf_loc++] = *p_data++;
      dma_data[buf_no][buf_loc++] = *p_data++;
      dma_data[buf_no][buf_loc++] = *p_data++;
      dma_data[buf_no][buf_loc++] = *p_data++;
      len--;
    }
}

/* mode sets whether you do a MSCAL, MSCNT or MSCALF */
void vu1_exec_code(u32 addr, int mode)

{
  u32 viftag_cmd;

  switch(mode)
    {
    case EXEC_MSCAL: viftag_cmd = MSCAL;
      break;
    case EXEC_MSCNT: viftag_cmd = MSCNT;
      break;
    case EXEC_MSCALF: viftag_cmd = MSCALF;
      break;
    default: init->printf("Invalid exec call mode %d\n", mode);
      return;
    }

  if(buf_loc > (DMABUF_SIZE - 8))
    {
      vu1_send_chain();
    }

  dma_data[buf_no][buf_loc++] = CTAG(1, CNT);
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = VIF_TAG(viftag_cmd, 0, addr);
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
}

void vu1_setup_db(u32 base, u32 offset)

{
  if(buf_loc > (DMABUF_SIZE - 8))
    {
      vu1_send_chain();
    }

  dma_data[buf_no][buf_loc++] = CTAG(1, CNT);
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = VIF_TAG(BASE, 0, base);
  dma_data[buf_no][buf_loc++] = VIF_TAG(OFFSET, 0, offset);
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
}

void vu1_start()
{
  asm __volatile__(
		   "ctc2 $0,$31");	
}

void vu1_add_flush()

{
  if(buf_loc > (DMABUF_SIZE - 8))
    {
      vu1_send_chain();
    }

  dma_data[buf_no][buf_loc++] = CTAG(1, CNT);
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = VIF_TAG(FLUSH, 0, 0);
  dma_data[buf_no][buf_loc++] = VIF_TAG(FLUSHE, 0, 0);
  dma_data[buf_no][buf_loc++] = 0;
  dma_data[buf_no][buf_loc++] = 0;
}

void vu1_send_chain()

{
   FlushCache(0);
   dma_data[buf_no][buf_loc++] = CTAG(0, END);
   dma_data[buf_no][buf_loc++] = 0;
   dma_data[buf_no][buf_loc++] = 0;
   dma_data[buf_no][buf_loc++] = 0;

   //vu1_dumpregs();
   DMA_WAIT(VIF1_CHCR);
   SET_TADR(VIF1_TADR, dma_data[buf_no], 0);
   SET_QWC(VIF1_QWC, 0); 
   SET_CHCR(VIF1_CHCR, 1, 1, 0, 1, 0, 1, 0);
   buf_no ^= 1;
   buf_loc = 0;
   //DMA_WAIT(VIF1_CHCR);
}

void vu1_wait_dma()

{
   DMA_WAIT(VIF1_CHCR);
}

#define PRINT_REG(x) init->printf(#x "\t %08X %s\n", *((u32 *) x), getasbits(*((u32 *) x)))
#define REG(x) *((u32 *) x)

void vu1_dumpregs()

{
   PRINT_REG(vif1_stat);
   PRINT_REG(vif1_fbrst);
   PRINT_REG(vif1_err);
   PRINT_REG(vif1_mark);
   PRINT_REG(vif1_cycle);
   PRINT_REG(vif1_mode);
   PRINT_REG(vif1_num);
   PRINT_REG(vif1_mask);
   PRINT_REG(vif1_code);
   PRINT_REG(vif1_itops);
   PRINT_REG(vif1_base);
   PRINT_REG(vif1_ofst);
   PRINT_REG(vif1_tops);
   PRINT_REG(vif1_itop);
   PRINT_REG(vif1_top);
}
