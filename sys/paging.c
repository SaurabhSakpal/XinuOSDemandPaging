#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>


void init_pageDirectory(int a, int pid)
{
	int i =0;
	int j = a+1024; 
	for(i = 0 ;i<1024;i++)
	{
		struct pd_t* dir;
		dir = (j*4096)+(4*i);
		if(i<4)
		{
			dir->pd_pres  = 1;            /* page table present?          */
  		}
		else
		{
			dir->pd_pres  = 0;
		}
		dir->pd_write = 1;            /* page is writable?            */
  		dir->pd_user  = 1;            /* is use level protection?     */
  		dir->pd_pwt   = 1;            /* write through cachine for pt?*/
  		dir->pd_pcd   = 1;            /* cache disable for this pt?   */
  		dir->pd_acc   = 0;            /* page table was accessed?     */
  		dir->pd_mbz   = 1;            /* must be zero                 */
  		dir->pd_fmb   = 1;            /* four MB pages?               */
  		dir->pd_global= 1;            /* global (ignored)             */
  		dir->pd_avail = 7;            /* for programmer's use         */
  		dir->pd_base  = (1024+i); 
	}
	frm_tab[a].fr_status = FRM_MAPPED;
        frm_tab[a].fr_pid = pid;
       	frm_tab[a].fr_vpno = 0;
        frm_tab[a].fr_refcnt =0;
        frm_tab[a].fr_type = FR_DIR;
        frm_tab[a].fr_dirty = 0;
        frm_tab[a].fr_loadtime = 0;
	//kprintf("FOR PROCESS %d Free Frame for Directory: %d (Actual: %d)\n",pid,a,j);
}


void initFreeFrames()
{
	int i;
	for(i=0;i<4;i++)
	{
		frm_tab[i].fr_status = FRM_MAPPED;
                frm_tab[i].fr_pid = 0;
                frm_tab[i].fr_vpno = 0;
                frm_tab[i].fr_refcnt =0;
                frm_tab[i].fr_type = FR_TBL;
                frm_tab[i].fr_dirty = 0;
                frm_tab[i].fr_loadtime = 0;
	}
}

void init_commonpages()
{
	int i;
	for(i = 0;i<4096;i++)
	{
		struct pt_t* page; 
		page = (1024*4096)+(4*i);	
		page->pt_pres  = 1;            /* page table present?          */
  		page->pt_write = 1;            /* page is writable?            */
  		page->pt_user  = 1;            /* is use level protection?     */
  		page->pt_pwt   = 1;            /* write through cachine for pt?*/
  		page->pt_pcd   = 1;            /* cache disable for this pt?   */
  		page->pt_acc   = 0;            /* page table was accessed?     */
	  	page->pt_mbz   = 0;            /* must be zero                 */
  		page->pt_dirty = 0;            /* four MB pages?               */
  		page->pt_global= 1;            /* global (ignored)             */
  		page->pt_avail = 7;            /* for programmer's use         */
  		page->pt_base  = i;
	}
	initFreeFrames();	
}

void free_frames_kill(int pid)
{
	//kprintf("	INSIDE FREE_FRAMES_KILL PID: %d\n",pid);
	int i = 0;
	int j = 0;
	for(i = 0;i<1024;i++)
	{
		if(frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_pid == pid && frm_tab[i].fr_type == FR_PAGE)
		{
			//kprintf("--------------> FOR FRAME: %d \n",i);
			int bs = 0;
			int page = 0;
			int *pageth = &page;
			int *store = &bs;
			unsigned long vaddr = frm_tab[i].fr_vpno*4096;
			bsm_lookup(pid,vaddr,store,pageth);
			xmunmap(frm_tab[i].fr_vpno);
			release_bs(bs);
			//kprintf("FRAME: %d STORING PAGE RELEASED FOR PID:%d\n",i,pid);
		}
		
	}
	for(j = 0;j<1024;j++)
	{
		if(frm_tab[j].fr_status == FRM_MAPPED && frm_tab[j].fr_pid == pid && frm_tab[j].fr_type == FR_TBL)
        	{
                        frm_tab[j].fr_status = FRM_UNMAPPED;
                        frm_tab[j].fr_pid = 0;
                        frm_tab[j].fr_vpno = 0;
                        frm_tab[j].fr_refcnt =0;
                        frm_tab[j].fr_type = 0;
                        frm_tab[j].fr_dirty = 0;
                        frm_tab[j].fr_loadtime = 0;
        	}
	}
	unsigned long page_directory = proctab[pid].pdbr;
	int dir_frame = (page_directory/4096)-1024;
	frm_tab[dir_frame].fr_status = FRM_UNMAPPED;
        frm_tab[dir_frame].fr_pid = 0;
        frm_tab[dir_frame].fr_vpno = 0;
        frm_tab[dir_frame].fr_refcnt =0;
        frm_tab[dir_frame].fr_type = 0;
        frm_tab[dir_frame].fr_dirty = 0;
        frm_tab[dir_frame].fr_loadtime = 0;
	//kprintf("FRAME: %d STORING PAGE DIRECTORY RELEASED FOR PID:%d\n",dir_frame,pid);
}
	
void free_dirty_frames(int old_pid, int new_pid)
{
	int i = 0;
	//kprintf("	INSIDE FREE DIRTY FRAMES OLD_PID: %d NEW_PID: %d",old_pid,new_pid);
	for(i = 0;i<1024;i++)
        {
                if(old_pid != 0 && frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_pid == old_pid && frm_tab[i].fr_type == FR_PAGE)
                {
			//kprintf("#### FRAME: %d \n",i);
                        int bs = 0;
                        int page = 0;
                        int *pageth = &page;
                        int *store = &bs;
                        unsigned long vaddr = frm_tab[i].fr_vpno*4096;
                        bsm_lookup(old_pid,vaddr,store,pageth);
			unsigned long physical_add = (1024+i)*4096;
			write_bs(physical_add,bs,page);
                        //kprintf("################### FRAME: %d COPIED BACK TO BS: %d FOR PID: %d FR_VPNO: %d \n",i,bs,old_pid,frm_tab[i].fr_vpno);
                }
		else
		{
			 if(new_pid != 0 && frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_pid == new_pid && frm_tab[i].fr_type == FR_PAGE)
			{
				int bs = 0;
                        	int page = 0;
                        	int *pageth = &page;
                        	int *store = &bs;
                        	unsigned long vaddr = frm_tab[i].fr_vpno*4096;
                        	bsm_lookup(new_pid,vaddr,store,pageth);
                        	unsigned long physical_add = (1024+i)*4096;
                        	read_bs(physical_add,bs,page);
                        	//kprintf("################ FRAME: %d COPIED FROM  BS: %d FOR PID: %d FR_VPNO: %d \n",i,bs,new_pid,frm_tab[i].fr_vpno);		
			}
		}
        }	
}
