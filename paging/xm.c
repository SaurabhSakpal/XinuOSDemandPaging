/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
	//kprintf("INSIDE XMMAP(%d : %d)\n",virtpage,source);

	if ( (virtpage < 4096) || ( source < 0 ) || ( source > MAX_ID) ||(npages < 1) || ( npages >128))
	{
		//kprintf("xmmap call error: parameter error! \n");
		return SYSERR;
	}
	if(npages > bsm_tab[source].bs_npages)
	{
		//kprintf("XMMAP Requested more pages(%d) than mapped(%d) \n",npages,bsm_tab[source].bs_npages);
		return SYSERR;
	}
	int result = bsm_map(currpid,virtpage,source,npages);
  	if(result==SYSERR)
	{
		return SYSERR;
	}
	else
	{
		return OK;
	}
}





void free_page_table(unsigned long pdbr,unsigned long start_vp, int npages)
{
	//kprintf("FREE PAGE TABLE\n");
	int i;
	for(i = 0;i<npages;i++)
	{
		unsigned long virtadr   = (start_vp+i)*4096; 
		unsigned long pd_offset = virtadr>>22;
		unsigned long pt_offset = (virtadr>>12)& 0x000003ff;
		unsigned long offset = virtadr & 0x00000fff;
		struct pd_t* dir_base = pdbr;
        	struct pd_t* dir = dir_base + pd_offset;
		unsigned int pt_frame = dir->pd_base;
		struct pt_t* pt_base = pt_frame*4096;
		struct pt_t* pt = pt_base + pt_offset;
		pt->pt_pres = 0;
		pt->pt_write = 0;
		pt->pt_dirty = 0;
		//kprintf("		FRAME FREED: %d PID: %d PT: %ul\n",pt->pt_base,currpid,pt_base);
		pt->pt_base = 0;
	}
}

void free_page_directory(unsigned long pdbr,unsigned long vadr1,unsigned long vadr2)
{
	//kprintf("FREE PAGE DIRECTORY\n");
	int  i;
	unsigned long page_number1 = vadr1>>12;
	unsigned long page_number2  = vadr2>>12;
 
      	unsigned long pd_offset = vadr1>>22;;
      	unsigned long pt_offset = (vadr1>>12)& 0x000003ff;
      	unsigned long offset = vadr1 & 0x00000fff;
      	struct pd_t* dir_base = pdbr;
     	struct pd_t* dir = dir_base + pd_offset;
      	unsigned int pt_frame = dir->pd_base;
	struct pt_t* pt_base = pt_frame*4096;
	for(i = 0;i<1024;i++)
        {
                struct pt_t* page;
                page = (pt_base)+(4*i);
                if(page->pt_pres == 1)
		{
			return;
		}
        }
	int frame_number = vpno_to_frames(dir->pd_base);
	//kprintf("		1. Page Table Frame: %d\n",frame_number);
	free_frm(frame_number);
	dir->pd_pres = 0;
	dir->pd_base = 0;
	if(page_number1==page_number2)
	{
		return;
	}

	unsigned long pd_offset2 = vadr2>>22;;
        unsigned long pt_offset2 = (vadr2>>12)& 0x000003ff;
        unsigned long offset2 = vadr2 & 0x00000fff;
        struct pd_t* dir_base2 = pdbr;
        struct pd_t* dir2 = dir_base2 + pd_offset2;
        unsigned int pt_frame2 = dir2->pd_base;
        struct pt_t* pt_base2 = pt_frame2*4096;
        for(i = 0;i<1024;i++)
        {
                struct pt_t* page2;
                page2 = (pt_base2)+(4*i);
                if(page2->pt_pres == 1)
                {
                        return;
                }
        }
	int frame_number2 = vpno_to_frames(dir2->pd_base);
	//kprintf("               2. Page Table Frame: %d\n",frame_number);
        free_frm(frame_number2);
        dir2->pd_pres = 0;
        dir2->pd_base = 0;
	return;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage )
{
	//kprintf("INSIDE XMUNMAP\n");
	if ( (virtpage < 4096) )
	{ 
		//kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
		return SYSERR;
  	}
	int start_vp = 0;
	int npages = 0;
	int bs_id = 0;
	int page = 0;
	int *pageth = &page;
	int *store = &bs_id;
	unsigned long vaddress = virtpage*4096;
	int result = bsm_lookup(currpid,vaddress,store,pageth);
	if(result == SYSERR)
	{
		//kprintf("SYSERR: Error in UNPMAPPING VPAGE: %d\n",virtpage);
		return SYSERR;
	}
	else
	{
		struct bs_node* node = bsm_tab[bs_id].first;
		while(node->next!=NULL)
		{
			if(node->pid == currpid)
			{		
				start_vp = node->bs_npages;
				npages =  node->bs_vpno;
				node->bs_npages = 0;
				node->bs_vpno = 0;
				break;
				//kprintf("HERE XYZ: (%d,%d)\n",start_vp,npages);
			}
			//kprintf("XYZ\n");
			node = node->next;
		}
		if(node->pid==currpid)
		{
			npages = node->bs_npages;
                        start_vp =  node->bs_vpno;
                        node->bs_npages = 0;
                        node->bs_vpno = 0;
		}
		int i = 0;
                for(i = 0;i<1024;i++)
                {
                        if(frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_pid == currpid && frm_tab[i].fr_type == FR_PAGE)
                        {
                                if(frm_tab[i].fr_vpno>=virtpage && frm_tab[i].fr_vpno<=(virtpage+npages))
				{
					int frame = vpno_to_frames(frm_tab[i].fr_vpno);
					char* phy = (frame+1024)*4096;
					int pg_off = frm_tab[i].fr_vpno-virtpage;
					write_bs(phy,bs_id,pg_off);
					free_frm(frame);
				}

                        }

                }
		unsigned long pdbr = read_cr3();
		unsigned long vadr1 = start_vp *4096;
		unsigned long vadr2 = (start_vp+npages)*4096;  
		//kprintf("HERE: (%ul,%ul,%ul)\n",pdbr,vadr1,vadr2);
		free_page_table(pdbr,start_vp,npages);
		free_page_directory(pdbr,vadr1,vadr2);
		return OK;
	}
}
 
