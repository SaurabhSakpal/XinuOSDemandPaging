/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	int i;
	STATWORD ps;
	disable(ps);
	for(i=0;i<=15;i++)
	{
		bsm_tab[i].bs_status = BSM_UNMAPPED;
		bsm_tab[i].bs_npages = 0;
		bsm_tab[i].first = NULL;
		bsm_tab[i].last = NULL;
		bsm_tab[i].is_private = 0;
		bsm_tab[i].bs_mapping = 0;
	}
	restore(ps);
	//kprintf("BS Table  Initialized\n");
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	int i;
        for(i=0;i<=MAX_ID;i++)
        {
                if(bsm_tab[i].bs_status == BSM_UNMAPPED)
		{
			*avail = i;
			return OK;
		}
         
        }
	//kprintf("SYSERR: NO FREE  BS\n");
        return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	//kprintf("INSIDE FREE_BSM\n");
	STATWORD ps;
	if(bsm_tab[i].bs_status == BSM_UNMAPPED)
	{
		//kprintf("SYSERR: Trying to free UNMAPPED BS\n");
		return SYSERR;
	}
	if(bsm_tab[i].bs_mapping==1)
	{
		disable(ps);
		bsm_tab[i].bs_status = BSM_UNMAPPED;
		int a =  bsm_tab[i].bs_npages;
                bsm_tab[i].bs_npages = 0;
		struct bs_node* node = bsm_tab[i].first;
		freemem(node,sizeof(struct bs_node));
		bsm_tab[i].first = NULL;
                bsm_tab[i].last = NULL;
                bsm_tab[i].is_private = 0;
                bsm_tab[i].bs_mapping = 0;
		//kprintf("BS: %d UNMAPPED && Last PID:%d  NPAGES:%d \n",i,currpid,a);
		restore(ps);
	}
	else
	{
		//kprintf("SYSERR: Multiple Mappings to BS\n");
		return SYSERR;
	}	
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	//kprintf("INSIDE BSM_LOOKUP:(%ul)\n",vaddr);
	STATWORD ps;
	disable(ps);
	int i; 
	for(i = 0;i<=MAX_ID;i++)
	{
		if(bsm_tab[i].bs_status == BSM_MAPPED)
		{
			struct bs_node* node = bsm_tab[i].first;
			while(node->next!=NULL)
			{
				if(node->pid == pid)
				{
					unsigned long page_number = ((vaddr>>12)&0x000fffff);;
					int starting_vp = node->bs_vpno;	
					int pages = bsm_tab[i].bs_npages;
					if(starting_vp<=page_number && page_number<=(pages+starting_vp))
					{
						int page = page_number%starting_vp;		
						*store = i;
						*pageth = page;
						//kprintf("	1. VirtualAddress: %ul Mapped to BS: %d  PAGE: %d \n",vaddr,i,page);
						restore(ps);
						return OK;
					}
				}
				node = node->next;
			}
			if(node->pid == pid)
			{
				
				if(bsm_tab[i].is_private!=0)
				{
					unsigned long page_number = ((vaddr>>12)&0x000fffff);;
					int starting_vp = node->bs_vpno;
					int pages = bsm_tab[i].bs_npages;
					//kprintf("*************PAGE_NUMBER: %ul STARTING_VP: %d PAGES: %d ",page_number,starting_vp,pages);
					if(starting_vp<=page_number && page_number<=(pages+starting_vp))
					{	
						int page = page_number%starting_vp;
						*store = i;
						*pageth = page;
						//kprintf("	2. VirtualAddress: %ul Mapped to BS: %d  PAGE: %d \n",vaddr,i,page);
						restore(ps);
						return OK;
					}
				}
				else
				{
					unsigned long page_number = ((vaddr>>12)&0x000fffff);
                                        int starting_vp = node->bs_vpno;
                                        int pages = bsm_tab[i].bs_npages;
					//kprintf("*************PAGE_NUMBER: %ul STARTING_VP: %d PAGES: %d \n ",page_number,starting_vp,pages);
                                        if(starting_vp<=page_number && page_number<=(pages+starting_vp))
                                        {
                                                int page = page_number%starting_vp;
                                                *store = i;
                                                *pageth = page;
                                                //kprintf("	3. VirtualAddress: %ul Mapped to BS: %d  PAGE: %d \n",vaddr,i,page);
                                                restore(ps);
                                                return OK;
                                        }

				}
			}	
		}
	}
	//kprintf("SYSERR: No Mapping for given Virtual Address: %ul \n",vaddr);
	restore(ps);
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	STATWORD ps;
	disable(ps);
	struct bs_node* node = bsm_tab[source].first;
	/*if(npages>bsm_tab[source].bs_npages)
	{
		return SYSERR;
	}*/
	if(node==NULL)
	{
		//kprintf("SYSERR: BS: %d Kill Process: %d present in \n",source,currpid);
		return SYSERR;
	}
	while(node->next!=NULL)
	{
		if(node->pid == pid && node->bs_vpno!=0)
		{
			//kprintf("SYSERR: BS: %d Mapping for PID: %d present in \n",source,currpid);
			restore(ps);
			return SYSERR;
		}
		else if(node->pid == pid && node->bs_vpno==0)
		{
			break;
		}
		node = node->next;
	}
	if(node->pid==pid && node->bs_vpno!=0)
	{
		//kprintf("SYSERR: BS: %d Mapping for PID: %d present in \n",source,currpid);
		restore(ps);
		return SYSERR;
	}
	else if(node->pid==pid && node->bs_vpno==0)
	{
		node->bs_vpno = vpno;
		//kprintf("BS: %d Mapping Updated for PID: %d VPNO: %d\n",source,currpid,vpno);
		restore(ps);
		return OK;
	}
	else
	{
		struct bs_node* new_map = (struct bs_node*)getmem(sizeof(struct bs_node));
		new_map->pid = pid;
		new_map->bs_vpno = vpno;
		new_map->bs_npages = npages;
		struct bs_node* last = bsm_tab[source].last;
		last->next = new_map;
		new_map->prev = last;
		bsm_tab[source].bs_mapping = bsm_tab[source].bs_mapping +1;
		bsm_tab[source].last = new_map;
		//kprintf("BS: %d Mapping Newly Added for PID: %d\n",source,currpid);
	}
	restore(ps);
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
}


