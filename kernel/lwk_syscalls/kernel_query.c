#include <lwk/task.h>
#include <lwk/aspace.h>
#include <arch/uaccess.h>

int
sys_kernel_query(
		int *name,
		int nlen,
		void *oldval,
		size_t *oldlenp,
		void *newval,
		size_t newlen
)
{
	int status;
	void *buf;
	int buflen;
	// XXX: do we need to do anything with security here?
	// TODO: copy to and from user

//	if (copy_from_user(&_start_state, start_state, sizeof(_start_state)))
//		return -EFAULT;

//	if ((_start_state.aspace_id < UASPACE_MIN_ID) ||
//	    (_start_state.aspace_id > UASPACE_MAX_ID))
//		return -EINVAL;
	// what are we copying here?
	// that is something that I need to figure out.
	printk(KERN_DEBUG "oldval %p oldlen %p %d newval %p newlen %d\n",  oldval,
			oldlenp, *oldlenp,
			newval,
			newlen);
	if ((status = kernel_query(name, nlen, &buf, &buflen, newval, newlen)) != 0) {
		printk(KERN_DEBUG "kernel_query failed status %d\n", status);
		return status;
	}
	printk(KERN_WARNING "query buf %p\n",buf);
	printk(KERN_WARNING "query user tree count %d\n",
			((struct user_tree*)buf)->count);
	printk(KERN_DEBUG "Copying to user buflen %d\n", buflen);
	if (oldval && copy_to_user(oldval, buf, buflen))
		return -EFAULT;
	printk(KERN_DEBUG "Copied to user\n");
	printk(KERN_DEBUG "finished syscall\n");
	kmem_free(buf);
	// TODO(npe): free the buf
	return 0;
}
