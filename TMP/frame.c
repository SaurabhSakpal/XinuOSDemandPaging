/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  	//kprintf("Initialize Frames\n");
	struct fr_map_t *fr;
	fr = &frm_tab[0];
	int i;
	for(i=0;i<FREEPAGE;i++)
	{
		fr->fr_status = FRM_UNMAPPED;
		fr->fr_pid = 0;
		fr->fr_vpno = 0;
		fr->fr_refcnt =0; 
		fr->fr_type = FR_PAGE;
		fr->fr_dirty = 0;
		fr->fr_loadtime = 0;
	}
  	return OK;
}

void print_frame_refcount()
{
	int i = 0;
	for(i = 0;i<1024;i++)
	{
		if(frm_tab[i].fr_status == FRM_MAPPED &&  frm_tab[i].fr_type == FR_PAGE)
		{
			kprintf("		FRAME: %d   REFCOUNT: %d VPNO: %d \n",i,frm_tab[i].fr_refcnt,frm_tab[i].fr_vpno);
		}	
	}
}

void delete_pde(unsigned long vaddr)
{
	STATWORD ps;
        disable(ps);
	int i = 0;
	unsigned long pd_offset = vaddr>>22;;
        unsigned long pt_offset = (vaddr>>12)& 0x000003ff;
        unsigned long offset = vaddr & 0x00000fff;
        struct pd_t* dir_base = read_cr3();
        struct pd_t* dir = dir_base + pd_offset;
        unsigned int pt_frame = dir->pd_base;
        struct pt_t* pt_base = pt_frame*4096;
	//kprintf("%ul ::: %ul ::: %ul ::: %ul  ::: %ul ::: %ul\n",dir_base,pd_offset,pt_offset,dir,pt_frame,pt_base);
        for(i = 0;i<1024;i++)
        {
                struct pt_t* page;
                page = pt_base+((unsigned long)4*i);
                if(page->pt_pres == 1)
                {
			//kprintf("1. Page Table Entry Address: %ul \n",page);
                        return;
                }
        }
        //int frame_number = vpno_to_frames(dir->pd_base);
	int frame_number = pt_frame-1024;
        free_frm(frame_number);	
        dir->pd_pres = 0;
        dir->pd_base = 0;
	restore(ps);
	//kprintf("PAGE TABLE FREED FROM FRAME: %d \n ",frame_number);
}
void delete_pte(unsigned long vaddr)
{
	STATWORD ps;
	disable(ps);
	unsigned long virtadr   = vaddr;
        unsigned long pd_offset = virtadr>>22;;
        unsigned long pt_offset = (virtadr>>12)& 0x000003ff;
        unsigned long offset = virtadr & 0x00000fff;
        struct pd_t* dir_base = read_cr3();
        struct pd_t* dir = dir_base + pd_offset;
        unsigned int pt_frame = dir->pd_base;
        struct pt_t* pt_base = pt_frame*4096;
        struct pt_t* pt = pt_base + pt_offset;
        pt->pt_pres = 0;
        pt->pt_write = 0;
        pt->pt_dirty = 0;
        pt->pt_base = 0;
	restore(ps);
}

int swap_frame()
{
	int i = 0;
	if(page_replace_policy == FIFO)
	{
		STATWORD ps;
		disable(ps);
		struct fifo* node = fifo_head;	
		fifo_head = fifo_head->next;
		i = node->frm_num;
		node->next = NULL;
		restore(ps); 
		//kprintf("Swapped Frame: %d\n",i);	
		freemem(node,sizeof(struct fifo));
	}
	else 
	{
		if(page_replace_policy == LRU)
		{
			int j = 0;
			int min = ref_couter;
			int min_frame = 0; 
			for(j=8;j<1024;j++)
			{
				
				if( frm_tab[j].fr_type == FR_PAGE && frm_tab[j].fr_refcnt < min)	
				{
					min = frm_tab[j].fr_refcnt;
					min_frame = j;
				}
			}
			i = min_frame;
			//kprintf("FRAME SWAPPED: %d\n",i);
		}
	}
		
	int bs = 0;
        int page = 0;
        int *pageth = &page;
        int *store = &bs;
        unsigned long vaddr = frm_tab[i].fr_vpno*4096;
        bsm_lookup(currpid,vaddr,store,pageth);
        unsigned long physical_add = (1024+i)*4096;
        write_bs(physical_add,bs,page);
	//kprintf("%d ::: %d ::: %ul \n",bs,page,vaddr);
	free_frm(i);
	delete_pte(vaddr);
	delete_pde(vaddr);
	return i;
}
/*
void print_accessbit(unsigned long vaddr)
{
	unsigned long pd_offset = (vaddr>>22);
        unsigned long pt_offset = (vaddr>>12)& 0x000003ff;
	unsigned long pdbr = proctab[currpid].pdbr;
	struct pd_t* directory_base = pdbr; 
        struct pd_t* dir = directory_base+pd_offset;
        unsigned long pt_base = dir->pd_base*4096;
        struct pt_t* page = pt_base;
        struct pt_t* curr_page = page + pt_offset;
        kprintf("VPNO: %ul ACCESS_BIT:%d DIRTYBIT: %d PT_BASE: %d\n",vaddr,curr_page->pt_acc,curr_page->pt_dirty,curr_page->pt_base);
}
*/
void update_refcount()
{
	int i = 0;
        for(i = 0;i<1024;i++)
        {
                if(frm_tab[i].fr_status == FRM_MAPPED &&  frm_tab[i].fr_type == FR_PAGE)
                {
                        unsigned long vaddr = (frm_tab[i].fr_vpno)*4096;
			unsigned long pdbr = proctab[frm_tab[i].fr_pid].pdbr;
			unsigned long pd_offset = (vaddr>>22);
			unsigned long pt_offset = (vaddr>>12)& 0x000003ff;
			struct pd_t* directory_base = pdbr;
			struct pd_t* dir = directory_base+pd_offset;
			unsigned long pt_base = dir->pd_base*4096;
			struct pt_t* page = pt_base;
                	struct pt_t* curr_page = page + pt_offset;
			if(curr_page->pt_acc==1)
			{
				frm_tab[i].fr_refcnt = ref_couter;
				curr_page->pt_acc=0;
			}	
                }
        }
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
	int i;
	ref_couter = ref_couter+1;
	for(i=0;i<FREEPAGE;i++)
	{
		if(frm_tab[i].fr_status==FRM_UNMAPPED)
		{
			*avail = i;
			//kprintf("	FRAME RETURNED is: %d \n",i);
			return;
		}
	}
	if(i>=FREEPAGE)
	{
		
		int frame = swap_frame();
		*avail = frame;
		//kprintf("       SWAPPED FRAME RETURNED is: %d \n",frame);
	}
  	return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
        struct fr_map_t *fr;
        fr = &frm_tab[i];

        fr->fr_status = FRM_UNMAPPED;
        fr->fr_pid = 0;
        fr->fr_vpno = 0;
        fr->fr_refcnt =0;
        fr->fr_type = FR_PAGE;
        fr->fr_dirty = 0;
        fr->fr_loadtime = 0;

 	//kprintf("Frame %d freed successfully \n",i);
  	return OK;
}

int vpno_to_frames(int vpno)
{
	//kprintf("INSIDE VPNO_TO_FRAMES\n");
	int i;
	for(i = 0;i<1024;i++)
	{
		if(frm_tab[i].fr_status != FRM_UNMAPPED && frm_tab[i].fr_pid == currpid && frm_tab[i].fr_vpno == vpno)
		{
			//kprintf("	VirtualPage %d located in Feame %d\n",vpno,i);
			return i;
		}
	}
}
