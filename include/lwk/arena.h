/* Copyright (c) 2007-2010 Sandia National Laboratories */

#ifndef _LWK_ARENA_H
#define _LWK_ARENA_H

#include <lwk/types.h>
#include <lwk/arena.h>
#include <lwk/idspace.h>
#include <lwk/futex.h>
#include <arch/aspace.h>
#include <lwk/aspace.h>

#define BK_ARENA			(1 << 0)
typedef unsigned long bkflags_t;

extern int
arena_map_backed_region(
	id_t			id,
	vaddr_t			start,
	size_t			extent,
	size_t			backing_extent,
	vmflags_t		flags,
	bkflags_t		type,
	vmpagesize_t		pagesz,
	const char *		name,
	paddr_t			pmem
);

extern int
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
);

extern int
sys_aspace_set_region(
	id_t			id,
	vaddr_t			start,
	size_t			extent,
	bkflags_t			type
);

extern int
sys_aspace_sync_region(
	id_t			id,
	vaddr_t			start,
	size_t			extent
);

#endif 
