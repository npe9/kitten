#include <lwk/task.h>
#include <lwk/aspace.h>
#include <lwk/kernel_query.h>

int
kernel_query(int name,
		int nlen,
		void **oldval,
		size_t *oldlenp,
		void *newval,
		size_t newlen)
{
	/*
	 * so what do we do here?
	 * I want to be able to write things.
	 *
	 */
	printk(KERN_WARNING "buf %p oldlenp %p %d\n", oldval, oldlenp, *oldlenp);

	printk(KERN_WARNING "entering kernel query %d\n", name);
	switch(name) {
	case CTL_ASPACE:
		printk(KERN_WARNING "aspacing\n");
		// so where do I find the aspaces?
		aspace_enum(oldval, oldlenp);
		printk(KERN_WARNING "aspace_enumed got oldval %p *oldval %p oldlenp %d\n", oldval, *oldval, *oldlenp);
		printk(KERN_WARNING "user tree count %d\n",
				((struct user_tree*)oldval)->count);
		break;
	case CTL_TASK:
		printk(KERN_WARNING "tasking\n");

		// so where do I find the tasks?
		task_enum(oldval, oldlenp);
		break;
	default:
		printk(KERN_WARNING "not found\n");
		return -EINVAL;
	}
	printk(KERN_WARNING "finished\n");

	return 0;
}
