#include <lwk/aspace.h>
#include <lwk/arena.h>
#include <arch/uaccess.h>

int
sys_aspace_set_region(
	id_t                   id,
	vaddr_t				  	start,
	size_t					extent,
	bkflags_t                type
)
{


//	if (current->uid != 0)
//		return -EPERM;

	if (id != MY_ID && ((id < UASPACE_MIN_ID) || (id > UASPACE_MAX_ID)))
		return -EINVAL;
	// what else do we need to check?

	return aspace_set_region(id, start, extent, type);
}


int
sys_aspace_sync_region(
	id_t                   id,
	vaddr_t				  	start,
	size_t					extent
)
{

	if (current->uid != 0)
		return -EPERM;

	if ((id < UASPACE_MIN_ID) || (id > UASPACE_MAX_ID))
		return -EINVAL;

	// what else do we need to check?

	return aspace_sync_region(id, start, extent);
}
