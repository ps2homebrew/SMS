#include <sifdma.h>
#include <sifrpc.h>
#include <kernel.h>
#include <iopheap.h>
#include <loadfile.h>

////////////////////////////////////////////////////////////////////////
// Wrapper to load module from disc/rom/mc
// Max irx size hardcoded to 300kb atm..
static void
loadMemModule (u8 *src_mem, unsigned int size, int argc, char *argv)
{
	void *iop_mem;
	int ret, i;
	//struct t_SifDmaTransfer sdt;
	SifDmaTransfer_t sdt;
	
	//SifInitRpc (0);

	//we have to make sure size is a multiple of 16
	size = (((int)(size/16))+1)*16;

	iop_mem = SifAllocIopHeap (size/*1024 * 300*/);

	if (iop_mem == NULL) {
		printf ("allocIopHeap failed\n");
		SleepThread ();
	}
	//REPLACE THIS, load from ptr

	sdt.src = (void *) src_mem;
	sdt.dest = (void *) iop_mem;
	sdt.size = size;
	sdt.attr = 0;

	//FlushCache (0);
scr_printf("starting DMA\n");
	i = SifSetDma (&sdt, 1);	// start dma transfer
	while (SifDmaStat (i) >= 0);	// wait for completion of dma transfer
scr_printf("DMA done\n");
	ret = SifLoadModuleBuffer (iop_mem, argc, argv);
	if (ret < 0) {
		printf ("loadModuleBuffer ret %d\n", ret);
		SleepThread ();
	}

	SifFreeIopHeap (iop_mem);
	scr_printf("done\n");
}