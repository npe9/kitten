/*
 * page_ops.c
 *
 *  Created on: May 12, 2014
 *      Author: Noah Evans
 */
#include <lwk/arena.h>
#include <lwk/aspace.h>
#include <lwk/kernel.h>
#include <lwk/pmem.h>
#include <lwk/spinlock.h>
#include <lwk/list.h>

/* so this is global */


// XXX: do locking.

/*
 * What I really need to do here is to go through this and measure what is going
 * on with the observers. I can see what the different pages that are touched are
 * and then I can use that along with my observations of observer behavior to handle
 * this.
 */

/*
 * so what is the structure here?
 * I need to make a header file that defines my interface.
 * aspace_arena
 *
 *
 */

//static DEFINE_SPINLOCK(arena_list_lock);

struct arena_list_entry {
	struct list_head	link;
	struct pmem_region	rgn;
};

static struct arena_list_entry *
alloc_arena_list_entry(void)
{
	return kmem_alloc(sizeof(struct arena_list_entry));
}

static void
free_arena_list_entry(struct arena_list_entry *entry)
{
	kmem_free(entry);
}

// what are we doing here?
// so a pmem region should be able to have children.

// I need the address of the new paddr too.
// must call with aspace locked.
void
arena_delete(struct pmem_region *backing) {
//	struct pmem_region *rgn;
	struct arena_list_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, backing->list, link) {
		list_del(&entry->link);
		free_arena_list_entry(entry);
	}
}

int arena_new_page(struct aspace *aspace, vmpagesize_t pagesz, paddr_t *paddr) {
	// TODO: error checking?
//	struct arena_list_entry *entry;
//	struct pmem_region *arena;
//	struct list_head *list;
//	arena = list_entry(backing->head, struct arena_list_entry, rgn);
//	if(arena->brk >= arena->end) {
//		printk(KERN_ERR "trying to allocate pmem off of edge of region");
//		return -EINVAL;
//	}
//	arena->brk += pagesz;


	// So now I have to set up the private data.
	// how do I do that? that is not that hard, but I need to do it.
	struct pmem_region *arena;
	arena = aspace->as_private_data;

//	aspace_dump2console(aspace->id);
//	pmem_dump2console();
//	for(;;);
	// what do we have to do to do this?
//	printk("printing without %p\n", arena);
//	printk("mapping pmem brk %p\n", arena->brk);

//	*paddr = kmem_get_pages(0);
	if((arena->brk + 4096) >= arena->end)
		panic("out of memory");
	*paddr = arena->brk;
	// FIXME(npe) pagesizes
	arena->brk += 4096;


	//
	// need a way of bookkeeping here.
	// just add to the paddr
	return 0;
}



int arena_enqueue(struct pmem_region *backing) {
//	struct arena_list_entry *entry;
//	struct pmem_region *arena;
//	arena = list_entry(backing->head, struct arena_list_entry, rgn);

	return 0;

}

// XXX: how do you deal with the start of this?
//int arena_dequeue(struct pmem_region *arena) {
//	// if list is empty or list is
//	arena->head = list_
//
//	// move list entry.
//			// we can have no elements.
//			// if list is empty, or if
//	// really all you do is just push that.
//	// you need bookkeeping.
//	// do you keep a private structure around?
//	// well that is interesting because you need the bookeeping on a per-aspace basis.
//	return 0;
//}
/*
 * so what do you do here when you flush? You are doing an msync of some sort.
 * This msync corresponds to what you're
 */

void
arena_sync(struct pmem_region *backing)
{
//	struct arena_list_entry *entry;
//	struct pmem_region *arena;
//
//	arena = list_entry(backing->head, struct arena_list_entry, rgn);
//
//	arena->->enqueue(backing);
//	arena->tail = arena->tail->next;

}

// how do you deal with the attachments? Who is using them?
// you use the tail.
// so what happens is that you have a list of outstanding values.
// these outstanding values are then given by linux.

// so you get an attach message.
/*
 * how does that attach message work?
 * you have a file or a pointer or something that points to a vm region.
 * You can advertise it as well. that will eventually become a form of pmem.
 *
 * when you create that creates a certain value. that value can then be passed.
 * that value corresponds to a segid, a shmem name or whatever. that is up to the system.
 * we just find the pmem_region and that is how we get it.
 */
int
arena_attach(struct pmem_region *backing, struct list_head *arena_list)
{
	struct pmem_region *arena;

	// you need the second to last value.
	arena_list = backing->tail->prev;
	arena = list_entry(arena_list, struct arena_list_entry, rgn);
	arena->refcnt++;

	return 0;
}

/*
 * the released arena has been given by attach.
 * so is what you really want here a list entry?
 */
void
arena_release(struct pmem_region *backing, struct list_head *arena_list)
{
	struct pmem_region *arena;

	arena = list_entry(arena_list, struct arena_list_entry, rgn);
	arena->refcnt--;
	if(arena->refcnt == 0) {
		list_del(arena_list);
		list_add_tail(arena_list, backing->list);
	}
}

struct aspace_operations_struct arena_pops = {
		.new_page = arena_new_page,
//		.init = arena_init,
//		.enqueue = arena_enqueue,
//		.dequeue = arena_dequeue,
};


// must call with aspace locked
int arena_init(struct aspace *aspace, paddr_t start, size_t extent) {
	int status;
	struct pmem_region	query;
	struct pmem_region	*result;

	result = kmem_alloc(sizeof(struct pmem_region));
        if(result == NULL)
          panic("couldn't alloc arena");


	pmem_region_unset_all(&query);

	query.start		= (uintptr_t) start;
	query.end		= (uintptr_t) start + extent;
	query.allocated		= false;
	query.allocated_is_set	= false;

	printk("mapping pmem looking for 0x%lx 0x%lx\n", start, start+extent);
//	pmem_dump2console();
	status = pmem_query(&query, result);
	if(status != 0) {
		panic("couldn't find memory %d 0x%lx 0x%lx\n", status, result->start, result->end);
	}
	status = pmem_alloc_umem(extent, 0, result);
	printk("mapping pmem want 0x%lx 0x%lx query 0x%lx 0x%lx result 0x%lx 0x%lx\n",
			start, start+extent, query.start, query.end, result->start, result->end);
	result->brk = result->start;
	aspace->as_private_data = result;
	aspace->as_ops = &arena_pops;
	return 0;
	// so what do I need to do with this function?
	// I need to
//	struct pmem_region *rgn;
//	struct list_head *list;
//	struct arena_list_entry *entry, *tmp;
//
//	if((backing->end - backing->start) < extent) {
//		printk(KERN_ERR "attempting to allocate size %ld in region of size %ld",
//				size, backing->end - backing->start);
//		return -EINVAL;
//	}
//
//	INIT_LIST_HEAD(backing->list);
//
//
//	// so that is interesting? You make this hierarchical.
//	// so are you doing multiple levels?
//	for(i = backing->start; i < backing->end; i += size) {
//		if (!(entry = alloc_arena_list_entry()))
//			goto oom;
//		entry->rgn.start = i;
//		entry->rgn.end = i + size;
//		entry->rgn.size = size;
//		// XXX: do I need to do any more setup of the pmem here?
//		list_add_tail(list, backing->list);
//
//	}
//	backing->head = backing->tail = backing->list;
//
//	// what about alignment?
//
//	// XXX: make a new pmem child per region?
//
//	// XXX: should this return the new pmem_region?
//
//	// you need to get this much pmem
//	// you need to attach the pmem to the
//	return 0;
//oom:
//	// XXX: need to remember the proper idiom for this.
//	list_for_each_entry_safe(entry, tmp, backing->list, link) {
//		list_del(&entry->link);
//		free_arena_list_entry(entry);
//	}
	return -ENOMEM;
}

int
register_arena(struct aspace *aspace, struct aspace *backing)
{
	int i;
	struct pmem_region *pmem;
	struct arena_list_entry *entry, *tmp;
	size_t size;
	// XXX: so that is what I need.
	// you need to lock this. but that
	aspace->as_ops = &arena_pops;
	// so that is what sets that up.
	// so what do I need to do to make this work?
	// I need to have my own bit of the pool.
	// so I don't need the pmem region.
	// I need to set up my own data structures.

	// so what is the data structure?
	// so you have these regions.
	// and the regions are what make things work.
//	aspace_virt_to_phys(aspace->id, aspace->)
//	aspace->as_private_data;
// need to use aspace virt to phys or something else to get my pmem.

	if((pmem->end - pmem->start) < size) {
		printk(KERN_ERR "attempting to allocate size %ld in region of size %ld",
				size, pmem->end - pmem->start);
		return -EINVAL;
	}

	INIT_LIST_HEAD(pmem->list);


	// so that is interesting? You make this hierarchical.
	// so are you doing multiple levels?
	for(i = pmem->start; i < pmem->end; i += size) {
		if (!(entry = alloc_arena_list_entry()))
			goto oom;
		entry->rgn.start = i;
		entry->rgn.end = i + size;
		entry->rgn.size = size;
		// XXX: do I need to do any more setup of the pmem here?
		list_add_tail(&entry, pmem->list);

	}
	pmem->head = pmem->tail = pmem->list;

	// what about alignment?

	// XXX: make a new pmem child per region?

	// XXX: should this return the new pmem_region?

	// you need to get this much pmem
	// you need to attach the pmem to the
	return 0;
oom:
	// XXX: need to remember the proper idiom for this.
	list_for_each_entry_safe(entry, tmp, pmem->list, link) {
		list_del(&entry->link);
		free_arena_list_entry(entry);
	}
	return -ENOMEM;
}


