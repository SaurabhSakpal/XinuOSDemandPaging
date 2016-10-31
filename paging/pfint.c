/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
void add_to_queue(int frm_no)
{
	STATWORD ps;
	disable(ps);
	if(fifo_head==NULL)
	{
		struct fifo* node = (struct fifo*)getmem(sizeof(struct fifo));
		node->frm_num = frm_no;
		node->next = NULL;
		fifo_head = node;
		fifo_tail = node;
		//kprintf("FIRST FIFO QUEUE FRAME: %d\n",frm_no);
	}
	else
	{
		struct fifo* node = (struct fifo*)getmem(sizeof(struct fifo));
		int prev_frm = fifo_tail->frm_num;
		node->frm_num = frm_no;
		node->next = NULL;
		fifo_tail->next = node;
		fifo_tail = node;
		//kprintf("FIFO QUEUE FRAME %d ADDED AFTER %d\n",frm_no,prev_frm);
	}
	restore(ps);
}

int  initialize_pte(unsigned long fault_address, unsigned long pd_offset, unsigned long pt_offset)
{
	//kprintf("ISNIDE INITIALIZE_PTE\n");
	int avail = 0;
	int bs_store = 0;
	int page_number = 0;
	
        int *ptr_avail = &avail;
        int *store = &bs_store;
	int *pageth = &page_number;
	
	update_refcount();

	get_frm(ptr_avail);
	bsm_lookup(currpid,fault_address,store,pageth);

	char * c = (avail+1024)*4096;
	read_bs(c,bs_store,page_number);
	
	STATWORD ps;
        disable(ps);
	frm_tab[avail].fr_status = FRM_MAPPED;
	frm_tab[avail].fr_pid = currpid;
	frm_tab[avail].fr_vpno = fault_address>>12;
	frm_tab[avail].fr_refcnt = ref_couter;
	frm_tab[avail].fr_type = FR_PAGE;
	restore(ps);
	if(page_replace_policy == FIFO)
	{
		add_to_queue(avail);
	}	
	//kprintf("PAGE PLACED IN FRAME %d VPNO:%d \n",avail,frm_tab[avail].fr_vpno);	
	return avail;
}


int initialize_pageTable(unsigned long fault_address, unsigned long pd_offset, unsigned long pt_offset)
{
	//kprintf("INSIDE INITIALIZE_PAGETBLE\n");
	int avail = 0;
	int i;
	int *ptr_avail = &avail;
        get_frm(ptr_avail);
	STATWORD ps;
        disable(ps);
        frm_tab[avail].fr_status = FRM_MAPPED;
        frm_tab[avail].fr_pid = currpid;
        frm_tab[avail].fr_vpno = 0;
        frm_tab[avail].fr_refcnt = 0;
        frm_tab[avail].fr_type = FR_TBL;
        restore(ps);

	
	int data_frame = initialize_pte(fault_address,pd_offset,pt_offset);
	
	//kprintf("PAGE TABLE PLACED IN FRAME %d\n",avail);
	for(i=0;i<1024;i++)
	{
		struct pt_t* page;
		unsigned long addr = (avail+1024)*4096 + (i*4);
		page = addr;
		if(i==pt_offset)
		{
			page->pt_base  = 1024+data_frame;
			page->pt_pres  = 1;
			page->pt_acc   = 1;
			//kprintf("3. Page Table Entry Address: %ul\n",addr);
		}
		else
		{
			page->pt_base  = 0;
			page->pt_acc   = 0;
			page->pt_pres  = 0;
		}
                page->pt_write = 1;            /* page is writable?            */
                page->pt_user  = 1;            /* is use level protection?     */
                page->pt_pwt   = 1;            /* write through cachine for pt?*/
                page->pt_pcd   = 1;            /* cache disable for this pt?   */
                page->pt_mbz   = 0;            /* must be zero                 */
                page->pt_dirty = 0;            /* four MB pages?               */
                page->pt_global= 1;            /* global (ignored)             */
                page->pt_avail = 7;            /* for programmer's use         */
	}
	return avail;
}

SYSCALL pfint()
{
	//kprintf("INSIDE PFINT\n");
	unsigned long fault_address = read_cr2();
	//kprintf("Address Causing Fault: %ul\n",fault_address);
	unsigned long page_directory = read_cr3();
	unsigned long pd_offset = fault_address>>22;
	unsigned long pt_offset = (fault_address>>12)& 0x000003ff;
	unsigned long offset = fault_address & 0x00000fff;
	//kprintf("	PD: %ul  PD-Off: %ul  PT-Off: %ul Offset: %ul \n",page_directory,pd_offset,pt_offset,offset);
	
	int bs_store = 0;
        int page_number = 0;
        int *store = &bs_store;
        int *pageth = &page_number;
        int result = bsm_lookup(currpid,fault_address,store,pageth);
	if(result==SYSERR)
	{
		//kprintf("	SYSERR: Process: %d killed!\n",currpid);
		kill(currpid);
		return SYSERR;
	}

	struct pd_t* dir_base = page_directory;
	struct pd_t* dir = dir_base + pd_offset;

	if(dir->pd_pres==1)
	{
		//kprintf("		ONLY PAGE NOT FOUND\n");
		int data_frame = initialize_pte(fault_address,pd_offset,pt_offset);
			
		unsigned long page_table_base = dir->pd_base*4096;
		struct pt_t* page = page_table_base;
		struct pt_t* curr_page = page + pt_offset;
		curr_page->pt_base = (data_frame+1024);
		curr_page->pt_pres = 1;
		curr_page->pt_acc =1;
		//kprintf("2. Page Table Entry Address: %ul\n",curr_page);
		return OK;
	}
	else
	{
		//kprintf("               PAGE TABLE NOT FOUND\n");
		int pt_frame = initialize_pageTable(fault_address,pd_offset,pt_offset);	/*Initialize Page Tables*/
		dir->pd_pres = 1;
		dir->pd_base = 1024+pt_frame;	
		//kprintf("\nINITIALIZE: PT_FRAME: %d PD_OFFSET: %ul DIR: %ul PDBR: %ul \n\n",pt_frame,pd_offset,dir,page_directory);
		return OK;
	}	
	return OK;
}


