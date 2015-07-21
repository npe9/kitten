#include <lwk/pmem.h>
#include <arch/uaccess.h>
#include <arch/pisces/pisces_boot_params.h>

int
sys_pmem_alloc(
	size_t                               size,
	size_t                               alignment,
	const struct pmem_region __user *    constraint,
	struct pmem_region __user *          result
)
{
	struct pmem_region _constraint, _result;
	int status;
	extern struct pisces_boot_params *pisces_boot_params;
	uint64_t *i = (uint64_t*)pisces_boot_params->init_dbg_buf;
	extern int boomboom;

	*i = 0xba5e000;
	//if (current->uid != 0)
	//	return -EPERM;
	(*i)++;
	if (copy_from_user(&_constraint, constraint, sizeof(_constraint)))
		return -EINVAL;
	(*i)++;
	if ((status = pmem_alloc(size, alignment, &_constraint, &_result)) != 0)
		return status;
	(*i)++;
	if (result && copy_to_user(result, &_result, sizeof(_result)))
		return -EINVAL;
	(*i)++;
	return 0;
}
