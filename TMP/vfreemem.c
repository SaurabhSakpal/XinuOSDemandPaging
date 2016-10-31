/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(struct	mblock	*block, unsigned size)
{
	STATWORD ps;	    
	
	struct	mblock	*p, *q;	
	unsigned top;	
	struct pentry *ptr;
	ptr = &proctab[currpid];

    	unsigned long temp = (unsigned)block;
	int bs_heap = proctab[currpid].store;
	unsigned long vir_add_start = 4096*4096; 
    	temp = temp +((int)BACKING_STORE_BASE + (bs_heap * 128))- vir_add_start;
    	block = (struct mblock *)temp;

	unsigned long end = (bs_heap * 128)*4096 + (int) BACKING_STORE_BASE;
	unsigned long max_address = end + (proctab[currpid].vhpnpages)*4096; 

	if (size==0 || (unsigned)block>max_address
	    || ((unsigned)block)<(end))
		return(SYSERR);
	size = (unsigned)roundmb(size);
	disable(ps);
	for( p=ptr->vmemlist.mnext,q= &ptr->vmemlist;
	     p != (struct mblock *) NULL && p < block ;
	     q=p,p=p->mnext )
		;
	if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= &ptr->vmemlist) ||
	    (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
		restore(ps);
		return(SYSERR);
	}
	if ( q!= &ptr->vmemlist && top == (unsigned)block )
			q->mlen += size;
	else {
		block->mlen = size;
		block->mnext = p;
		q->mnext = block;
		q = block;
	}
	if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
		q->mlen += p->mlen;
		q->mnext = p->mnext;
	}
	restore(ps);
	return(OK);
}
