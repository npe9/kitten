/*
 * arena.c - template file for doing application specific producer observer app composition benchmarks
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>
#include "compbench.h"
#include <string.h>
#include <lwk/liblwk.h>
#include <lwk/task.h>


typedef struct Shared Shared;
struct Shared {
	int spin;
	struct timeval obstime;
};
typedef struct Arena Arena;
struct Arena {
	Shared *shared;
} arena;

#define STACK_SIZE 8192

void arenainit() {
}

void
observerhelper(void *v)
{
	Work *w;
	Arena *t;


	w = v;
	t = (Arena*) w->private;

	observer(w);
	/* fork doesn't share but we need the time back from the observer
	 * clone() or a pthread might be better.
	 */
	memcpy(&t->shared->obstime, &w->b->obsresult,
			sizeof(struct timeval));
}

void arenaspawn(Work *w) {
	Arena *t;
	__pid_t pid;
	int status;
	cpu_set_t mask;
	int result;
	int i, j, k, nflop;
	id_t id, new_id, tid;
	struct pmem_region rgn;
	vaddr_t stack;
	char *buf;
	int pagesz = VM_PAGE_4KB;
	int npages = 64;
	int nbacking = 64;
	int my_aspace;
	vaddr_t region;
	int my_id;


	printf("spawning\n");
	w->private = (void*) &arena;
	t = (Arena*) w->private;

	printf("beginning\n");

	// TODO(npe): allow the user to state how much should be benchmarked.
	nflop = 1;
	id = 1;
	printf("\n");
	printf("TEST BEGIN: TCASM API\n");
	// need to make an observer
	// need to test the producer.
	// need this to be apples to apples.

	start_state_t s;
	if(shouldobserve){
		if((status = aspace_copy(id, &new_id, 0))) {
			printf("couldn't copy aspace %d", status);
			// I forget how I handle this.
		}
		printf("aspace copied new_id %d\n", new_id);
		s.aspace_id = new_id;
		//	s.aspace_id = id;
		// how to select a cpu?
		// that is a good question?
		// this is where I want to know what is running on every cpu.
		s.cpu_id = 1;
		s.entry_point = (long int)observer;
		// this is interesting, I need to dump and figure out what the kernel is up to.
		s.group_id = 0;
		if((status = pmem_alloc_umem(8*VM_PAGE_2MB, VM_PAGE_2MB, &rgn))){
			// I forget how I handle this.
			// error handling.
		}
		printf("before stack\n");
		//	aspace_dump2console(id);
		printf("rgn size %d %p %p %d\n", rgn.size, rgn.start, rgn.end, rgn.end - rgn.start);
		if((status = aspace_map_region_anywhere(id, &stack, rgn.end - rgn.start, VM_WRITE|VM_READ|VM_USER, VM_PAGE_2MB, "new-stack", rgn.start))) {
			printf("couldn't map region %d\n", status);
			// I forget how I handle this.
		}
		//	aspace_dump2console(id);
		s.fs = stack+VM_PAGE_2MB;
		s.stack_ptr = stack+2*VM_PAGE_2MB;
		// TODO: Get this working?
		s.task_id = ANY_ID;
		snprintf(s.task_name,32,"tcasm observer");
		s.entry_point = observerhelper;
		s.use_args = 1;
		s.arg[0] = w;
		aspace_get_myid(&my_aspace);
		status = task_create(&s, &my_id);
		if(status != 0) {
			// do some sort of error handling here.
		}
	}
//	pmem_dump2console();
	status = arena_map_backed_region_anywhere(my_aspace, &region, pagesz*npages, pagesz*nbacking,
			VM_USER, BK_ARENA, pagesz, "cow-region", 0x00000007200000);
//	pmem_dump2console();

	if(status != 0) {
		printf("couldn't create backed region %d\n", status);
		// I forget how I handle this.

	}

	w->buf = (char*)region;

	// where do I put the shared bits?
	// I don't necessarily have to do I?
	t->shared = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_SHARED, -1 /* t->spinfd */, 0);


//		CPU_ZERO(&mask);
//		CPU_SET(w->prodcpu, &mask);
//		result = sched_setaffinity(0, CPU_ALLOC_SIZE(16), &mask);

		producer(w);


}

id_t shared_id;

int arenasharework(Work *w) {
	Arena *t;
	id_t my_id;
	int status;

	aspace_get_myid(&my_id);

	t = (Arena*) w->private;
	t->shared->spin++;
	// need to do the copy.
	// also
	status = aspace_copy(my_id, &shared_id, 0);
	if(status != 0) {
		printf("couldn't copy aspace %d %d\n", my_id, shared_id);
		return -1;
	}
	status = aspace_set_region(my_id, w->buf, w->datalen, BK_ARENA);
	if(status != 0) {
		printf("couldn't set region %d %p\n", my_id, w->buf);
		return -1;
	}

	return 0;
}

Work*
arenagetwork(Work *w) {
	Arena *t;
	static int lastspin;
	static int first = 1;

	t = (Arena*) w->private;
	id_t my_id, cur_id;
	int i;
	int status;
	char *buf;
	int offset;


	aspace_get_myid(&my_id);
	cur_id = shared_id;

	status = aspace_unsmartmap(cur_id, my_id);
	if(status != 0) {
		printf("couldn't unsmartmap into tcasm observer status %d\n", status);
		exit(status);
	}

	while (lastspin == t->shared->spin)
		;
	status = aspace_smartmap(cur_id, my_id, SMARTMAP_ALIGN, SMARTMAP_ALIGN);
	if(status != 0) {
		printf("couldn't smartmap into tcasm observer status %d\n", status);
		exit(status);
	}
//	printf("obs w->buf[0] %d\n", w->buf[0]);
	lastspin = t->shared->spin;
	first = 0;
	return w;
}

void arenafinalize() {
}

Bench arenabench = { .name = "arena", .init = arenainit, .sharework =
		arenasharework, .getwork = arenagetwork, .spawn = arenaspawn,
		.finalize = arenafinalize, };
