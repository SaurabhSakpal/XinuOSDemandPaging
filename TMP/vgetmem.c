/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD *vgetmem(unsigned nbytes)
{
	STATWORD ps;
	struct	mblock	*p, *q, *leftover;
	struct pentry *ptr;
	ptr = &proctab[currpid];
	unsigned long virtual_address = 4096*4096;

	if(nbytes>((proctab[currpid].vhpnpages)*4096))
	{
		return SYSERR;
	}

	
	disable(ps);
	if (nbytes==0 || ptr->vmemlist.mnext== (struct mblock *) NULL)
	{
		//kprintf("------------------------ 1. ENTERED\n");
		restore(ps);
		return SYSERR;
	}
	nbytes = (unsigned int) roundmb(nbytes);
	for (q= &ptr->vmemlist,p=ptr->vmemlist.mnext ;p != (struct mblock *) NULL ;q=p,p=p->mnext)
	if ( p->mlen == nbytes)
        {
		//kprintf("------------------------ 2. ENTERED\n");
		q->mnext = p->mnext;
		
		restore(ps);
            	p = (WORD *)(virtual_address + ((unsigned)p - ((int)BACKING_STORE_BASE + (proctab[currpid].store * 128))));
		return( (WORD *)p );
	}
	else if ( p->mlen > nbytes )
	{
		//kprintf("------------------------ 3. ENTERED\n");
		leftover = (struct mblock *)( (unsigned)p + nbytes );
		q->mnext = leftover;
		leftover->mnext = p->mnext;
		leftover->mlen = p->mlen - nbytes;
		
		restore(ps);
            	p =(WORD *)(virtual_address + ((unsigned)p - ((int)BACKING_STORE_BASE + (proctab[currpid].store * 128))));
		
		return( (WORD *)p );
	}
	//kprintf("------------------------ 4. ENTERED\n");
	restore(ps);
	return((WORD *)0);
}


