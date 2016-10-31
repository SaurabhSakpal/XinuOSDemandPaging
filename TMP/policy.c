/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>


int page_replace_policy;
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
	/* sanity check ! */
	if(policy==4 || policy==3)
	{
		page_replace_policy = policy;
		return OK;
	}
	//kprintf("To be implemented!\n");

  return SYSERR;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}
