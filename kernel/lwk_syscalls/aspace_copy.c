#include <lwk/aspace.h>

int
sys_aspace_copy(
	id_t    src,
	id_t    *dst,
	int flags
)
{
	int status;

	if (current->uid != 0)
		return -EPERM;

	status = aspace_copy(src, dst, flags);

	return status;
}
