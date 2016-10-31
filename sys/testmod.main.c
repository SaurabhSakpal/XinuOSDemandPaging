/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

void proc1_test3(char *msg, int lck)
{
	int i = 0;
	get_bs(1,10);
	xmmap(7000,1,10);
	int* b = 7000*4096;
	for(i = 0;i<10;i++)
	{
		b = (7000*4096) + (4*i);
		kprintf("-----------------------------------------> B(%ul): %d \n",b,*b);
	}
	xmunmap(7000);	
	release_bs(1);
	return;
}

int main()
{
	int i = 0;
	int pid1;	
	int pid2;
	int pid3;
	kprintf("\n Testing Memory Mapping \n");
	get_bs(1,10);
	xmmap(6000,1,10);
	int* a = 6000*4096;
	for(i = 0;i<10;i++)
	{
		a = (6000*4096)+ (4*i);
		*a = 2+i;
		//kprintf("-----------------------------------------> A(%ul): %d \n",a,*a);
	}
	xmunmap(6000);	
	release_bs(1);
	
	//pid2 = create(proc1_test3, 2000, 20, "proc1_test3", 0, NULL);
	pid3 = vcreate(proc1_test3, 2000,100, 20, "proc1_test3", 0, NULL);
	
	//resume(pid2);
	resume(pid3);
	
	sleep(6);
	return 0;
}
