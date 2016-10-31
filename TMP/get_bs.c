#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages)
{
	STATWORD ps;
	if(npages>128 || npages<=0)
	{
		//kprintf("SYSERR: NPAGES > 128 OR <=0 \n");
		return SYSERR;
	}
	if(bsm_tab[bs_id].bs_status==BSM_UNMAPPED || bsm_tab[bs_id].bs_status!=BSM_MAPPED)
	{
		//kprintf("BS: %d  NPAGES: %d MAPPPED? %d\n",bs_id,npages,bsm_tab[bs_id].bs_status);
		disable(ps);
		bsm_tab[bs_id].bs_status = BSM_MAPPED;
		bsm_tab[bs_id].bs_mapping = 1;
		bsm_tab[bs_id].bs_npages = npages;
		bsm_tab[bs_id].is_private =0;
		struct bs_node* node = (struct bs_node*)getmem(sizeof(struct bs_node));
		node->pid = currpid;
		node->bs_npages = npages;
		node->bs_vpno = 0;
		node->next = NULL;
		node->prev = NULL;
		bsm_tab[bs_id].first = node;
		bsm_tab[bs_id].last = node;
		restore(ps);
	
		//kprintf("~~~~~~~~~~~~First Mapping to BS:%d for PROCESS: %d PAGES: %d\n",bs_id,currpid,npages);
		return npages;
	}
	else
	{
		//kprintf("BS: %d  NPAGES: %d MAPPPED? %d\n",bs_id,npages,bsm_tab[bs_id].bs_status);
		if(bsm_tab[bs_id].is_private==1)
		{
			//kprintf("SYSERR: BS: %d has PRIVATE HEAP\n",bs_id);
			return SYSERR;
		}
		disable(ps);
		struct bs_node* node = (struct bs_node*)getmem(sizeof(struct bs_node));
		node->pid = currpid;
                node->bs_npages = bsm_tab[bs_id].bs_npages;
                node->bs_vpno = 0;
                node->prev = NULL;
		npages = bsm_tab[bs_id].bs_npages;
		struct bs_node* n = bsm_tab[bs_id].last;
		n->next = node;
		node->prev = n;
		bsm_tab[bs_id].last = node;
		//kprintf("~~~~~~~~~~~~~~~BS: %d PID: %d NPAGES: %d \n",bs_id,node->pid,node->bs_npages);
		bsm_tab[bs_id].bs_mapping = bsm_tab[bs_id].bs_mapping +1;
		restore(ps);
	}
    	return npages;
}


