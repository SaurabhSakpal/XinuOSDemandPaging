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
	get_bs(4,100);
	get_bs(8,200);
	get_bs(6,123);
	
	sleep(3);

	release_bs(8);
	release_bs(6);
	release_bs(4);	
	return;
}

int main()
{
	int pid1;	
	kprintf("\nTesting Backing Store \n");
	get_bs(8,80);
	xmmap(6000,8,80);
	int *a = 6000*4096; 
	int *b = 6001*4096;
	int *c = 6002*4096;
	*a = 100;
	*b =100;
	*c = 100; 
	xmunmap(6000);
	release_bs(8);
	return 0;
}
