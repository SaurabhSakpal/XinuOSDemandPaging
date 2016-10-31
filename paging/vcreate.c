/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
void update_bsmtab(int id,int pages,int pid)
{
	STATWORD ps;
	disable(ps);
        bsm_tab[id].bs_status = BSM_MAPPED;
       	bsm_tab[id].bs_npages = pages;
        struct bs_node* node = (struct bs_node*)getmem(sizeof(struct bs_node));
	node->pid = pid;
        node->bs_vpno = 4096;
        node->bs_npages = pages;
        bsm_tab[id].first = node;
        bsm_tab[id].last = node;
        bsm_tab[id].is_private = 1;
       	bsm_tab[id].bs_mapping = 1;
        //kprintf("BS: %d is private heap of Process: %d\n",id,pid);
        restore(ps);
}

SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	int vheap_id = 0;
        int *bsavail = &vheap_id;
        int ret  = get_bsm(bsavail);
        if(ret==SYSERR)
        {
                //kprintf(" XXXXXXXXXXX  SYSERR: PROCESS WITH VHEAP NOT CREATED \n");
                return SYSERR;
        }

	unsigned long   savsp, *pushsp;
        STATWORD        ps;
        int             pid;            /* stores new process id        */
        struct  pentry  *pptr;          /* pointer to proc. table entry */
        int             i;
        unsigned long   *a;             /* points to list of args       */
        unsigned long   *saddr;         /* stack address                */
        int             INITRET();
	
        disable(ps);
        if (ssize < MINSTK)
                ssize = MINSTK;
        ssize = (int) roundew(ssize);
        if (((saddr = (unsigned long *)getstk(ssize)) ==
            (unsigned long *)SYSERR ) ||
            (pid=newpid()) == SYSERR || priority < 1 ) {
                restore(ps);
                return(SYSERR);
        }

        //Initialize page directory
        int avail = 0;
        int *ptr_avail = &avail;
        get_frm(ptr_avail);
        init_pageDirectory(avail,pid);
        //initalize pdbr
        proctab[pid].pdbr = (avail+1024)*4096;

        numproc++;
        pptr = &proctab[pid];
	pptr->fildes[0] = 0;    /* stdin set to console */
        pptr->fildes[1] = 0;    /* stdout set to console */
        pptr->fildes[2] = 0;    /* stderr set to console */

        for (i=3; i < _NFILE; i++)      /* others set to unused */
                pptr->fildes[i] = FDFREE;

        pptr->pstate = PRSUSP;
        for (i=0 ; i<PNMLEN && (int)(pptr->pname[i]=name[i])!=0 ; i++)
                ;
        pptr->pprio = priority;
        pptr->pbase = (long) saddr;
        pptr->pstklen = ssize;
        pptr->psem = 0;
        pptr->phasmsg = FALSE;
        pptr->plimit = pptr->pbase - ssize + sizeof (long);
        pptr->pirmask[0] = 0;
        pptr->pnxtkin = BADPID;
        pptr->pdevs[0] = pptr->pdevs[1] = pptr->ppagedev = BADDEV;
	
                /* Bottom of stack */
        *saddr = MAGIC;
        savsp = (unsigned long)saddr;

        /* push arguments */
        pptr->pargs = nargs;
        a = (unsigned long *)(&args) + (nargs-1); /* last argument      */
        for ( ; nargs > 0 ; nargs--)    /* machine dependent; copy args */
                *--saddr = *a--;        /* onto created process' stack  */
        *--saddr = (long)INITRET;       /* push on return address       */
	*--saddr = pptr->paddr = (long)procaddr; /* where we "ret" to   */
        *--saddr = savsp;               /* fake frame ptr for procaddr  */
        savsp = (unsigned long) saddr;

/* this must match what ctxsw expects: flags, regs, old SP */
/* emulate 386 "pushal" instruction */
        *--saddr = 0;
        *--saddr = 0;   /* %eax */
        *--saddr = 0;   /* %ecx */
        *--saddr = 0;   /* %edx */
        *--saddr = 0;   /* %ebx */
        *--saddr = 0;   /* %esp; fill in below */
        pushsp = saddr;
        *--saddr = savsp;       /* %ebp */
        *--saddr = 0;           /* %esi */
        *--saddr = 0;           /* %edi */
        *pushsp = pptr->pesp = (unsigned long)saddr;
	

	
        if(ret!=SYSERR)
        {
                proctab[pid].store = vheap_id;
                update_bsmtab(vheap_id,hsize,pid);
                pptr->store = vheap_id;
                pptr->vhpno = 4096;
                pptr->vhpnpages = hsize;
        }

	struct mblock* heap_ptr;
	pptr->vmemlist.mnext = heap_ptr = (struct mblock *) roundmb(((vheap_id*128)*4096) + (2048*4096));        
	heap_ptr->mlen = hsize*4096;
		
	heap_ptr->mnext = NULL; 
	
	
	//kprintf("Starting Address of private Heap: %ul BS: %d \n ",pptr->vmemlist.mnext,vheap_id);
	restore(ps);

        return(pid);
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
