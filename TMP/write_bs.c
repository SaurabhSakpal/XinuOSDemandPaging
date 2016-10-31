#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

int write_bs(char *src, bsd_t bs_id, int page) {

  /* write one page of data from src
     to the backing store bs_id, page
     page.
  */
	//kprintf("INSIDE read_bs:(%ul)+(%ul)+(%ul)\n",src,bs_id,page);
   	char * phy_addr = ((int)BACKING_STORE_BASE) + (((int)BACKING_STORE_UNIT_SIZE)*bs_id) + page*NBPG;
   	//kprintf("	Copying from PhysicalAddress: %ul to BackingStoreAddress: %ul\n",src,phy_addr);
	bcopy((void*)src, phy_addr, NBPG);

}

