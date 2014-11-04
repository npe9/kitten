#include <lwk/liblwk.h>

int
aspace_map_backed_region(
	id_t			id,
	vaddr_t			start,
	size_t			extent,
	size_t			backing_extent,
	vmflags_t		flags,
	bkflags_t		type,
	vmpagesize_t		pagesz,
	const char *		name,
	paddr_t			pmem
) {
	int status;


	// I need to tell it which type of thing to make.
	// what is the type of thing?
	// I need to make sure that this is an arena.
	if ((status = aspace_add_region(id, start, extent, flags | VM_COW, pagesz, name)))
		return status;
	aspace_set_region(id, type);
	// XXX: so this gives me what I need plus slack.
	if ((status = aspace_add_region(id, start + extent, backing_extent, flags, pagesz, name)))
		return status;

	// XXX: this doesn't work because you're not doing all of this right.
	if ((status = aspace_map_pmem(id, pmem, start, extent + backing_extent))) {
		aspace_del_region(id, start, extent);
		return status;
	}

	return 0;
}

int
arena_map_backed_region_anywhere(
	id_t			id,
	vaddr_t *		start,
	size_t			extent,
	size_t			backing_extent,
	vmflags_t		flags,
	bkflags_t		type,
	vmpagesize_t		pagesz,
	const char *		name,
	paddr_t			pmem
) {
	int status;

retry:
//printf("finding holes\n");

	if ((status = aspace_find_hole(id, 0, extent + backing_extent, pagesz, start)))
		goto fail;
	printf("second hole *start %llx extent %llx backing %llx\n", *start, extent, backing_extent);
	if ((status = aspace_add_region(id, *start, extent, flags | VM_COW, pagesz, name))) {
		if (status == -ENOTUNIQ)
			goto retry; /* we lost a race with someone */
		goto fail;
	}
//	printf("after first region:\n");
//	aspace_dump2console(id);
//	printf("adding region\n");

	// XXX: change the backing region to have a name too.
	if ((status = aspace_add_region(id, *start+extent, backing_extent, (VM_USER |VM_WRITE|VM_READ), pagesz, name))) {
		if (status == -ENOTUNIQ)
			goto retry; /* we lost a race with someone */
		goto first_region_cleanup;
	}
//	printf("after second region:\n");
//	aspace_dump2console(id);

	printf("mapping pmem pmem %llx start %llx xtent extent %llx backing %llx\n", pmem, *start, extent, backing_extent);
	if ((status = aspace_map_pmem(id, pmem, *start, extent)))
		goto all_region_cleanup;
	printf("mapping pmem 2 pmem+extent %llx *start+extent %llx\n", pmem+extent, *start+extent);
	if ((status = aspace_map_pmem(id, pmem+extent, *start+extent, backing_extent)))
		goto first_pmem_cleanup;
	printf("mapping pmem 3 *start+extent %llx backing_extent llx\n", *start+extent, backing_extent);
	if((status = aspace_set_region(id, *start+extent, backing_extent, BK_ARENA)))
		goto all_pmem_cleanup;

//	printf("after region set:\n");
//	aspace_dump2console(id);
	return 0;

all_pmem_cleanup:
	aspace_unmap_pmem(id, pmem+extent, backing_extent);
first_pmem_cleanup:
	aspace_unmap_pmem(id, pmem, extent);
all_region_cleanup:
	aspace_del_region(id, *start+extent, backing_extent);
first_region_cleanup:
	aspace_del_region(id, *start, extent);
fail:
//	printf("failed %d did cleanup succeed?\n", status);
//	aspace_dump2console(id);

	return status;
}

