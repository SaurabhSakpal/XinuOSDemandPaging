#include <conf.h>
#include <kernel.h>
#include <mark.h>
#include <bufpool.h>
#include <proc.h>
#include <paging.h>

SYSCALL read_bs(char *dst, bsd_t bs_id, int page) {

  /* fetch page page from map map_id
     and write beginning at dst.
  */
	//kprintf("INSIDE read_bs:(%ul)+(%ul)+(%ul)\n",dst,bs_id,page);
   	void * phy_addr = ((int) BACKING_STORE_BASE) + (((int)BACKING_STORE_UNIT_SIZE)*bs_id) + page*NBPG;	  
	//kprintf("	Copying BackingStoreAddress: %ul to PhysicalAddress %ul \n",phy_addr,dst);
 	bcopy(phy_addr, (void*)dst, NBPG);
}


