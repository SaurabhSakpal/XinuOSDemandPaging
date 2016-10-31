#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id)
{
	STATWORD ps;
	//kprintf("INSIDE RELEASE_BS(%d)\n",bs_id);
	if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED)
	{
		//kprintf("SYSERR: FREE UNMAPPED BS:%d \n",bs_id);
		return SYSERR;
	}
	else
	{
		if(bsm_tab[bs_id].bs_mapping ==1)
		{
			free_bsm(bs_id);
			return OK;
		}
		disable(ps);
		struct bs_node* n = bsm_tab[bs_id].first;
		while(n->next != NULL)
		{
			if(n->pid == currpid)
			{
				struct bs_node* prev = n->prev;
				struct bs_node* next = n->next;
				prev->next = next;
				next->prev = prev;
				n->next = NULL;
				n->prev = NULL;
				freemem(n,sizeof(struct bs_node*));
				bsm_tab[bs_id].bs_mapping = bsm_tab[bs_id].bs_mapping -1;
				restore(ps);
				//kprintf("BS: %d Released by PID:%d NPAGES:%d\n",bs_id,currpid,bsm_tab[bs_id].bs_npages);
				return OK;
			}
			n = n->next;
		}
		if(n->pid==currpid)
		{
			struct bs_node* prev = n->prev;
			prev->next = NULL;
			n->next = NULL;
                        n->prev = NULL;
                        freemem(n,sizeof(struct bs_node));
			bsm_tab[bs_id].bs_mapping = bsm_tab[bs_id].bs_mapping -1;
			restore(ps);
			//kprintf("BS: %d Released by PID:%d NPAGES:%d\n",bs_id,currpid,bsm_tab[bs_id].bs_npages);
			return OK;
		}
	}
   	return SYSERR;
}

