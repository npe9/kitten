#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <liblwk.h>

int cur_id, shared_id;

#define MS_UPDATE 8
#define num_entries 4

void analyze_();
int fd = 0;
int rank = 0;
sem_t *lock;
void * ptr;
size_t total_pages = 0;
unsigned long long offset = 0;
unsigned long long total_time=0;

struct usda {
	int outer;
	int time_loop;
	int finalized;
	unsigned long long filesize;
	size_t meta_offset;
	int n[4];
	double d[3];
};

struct usda *cte;

int tcasm_observer() {
	printf("OBSERVING!\n");
}

int newproc_() {
	       int i, j, k, l, nflop, status;
        id_t id, new_id, tid;
        struct pmem_region rgn;
        vaddr_t stack;
        char *buf;
        double start, end;
        double cp_start, cp_end;
        double set_start, set_end;
        double iter_start, iter_end;
        vaddr_t region;
        int my_aspace;
        double cp_avg, set_avg;

        int pagesz = VM_PAGE_4KB;
        int npages = 2048*2;
        int nbacking = 2048*2;

        // TODO(npe): allow the user to state how much should be benchmarked.
        nflop = 1;
        id = 1;
        printf("\n");
        // need to make an observer
        // need to test the producer.
        // need this to be apples to apples.

        start_state_t s;
        if((status = aspace_copy(id, &new_id, 0))) {
                printf("couldn't copy aspace %d", status);
                return -1;
        }
        printf("aspace copied new_id %d\n", new_id);
        s.aspace_id = new_id;
//      s.aspace_id = id;
        // how to select a cpu?
        // that is a good question?
        // this is where I want to know what is running on every cpu.
        s.cpu_id = 1;
	s.entry_point = (long int)analyze_;
        // this is interesting, I need to dump and figure out what the kernel is up to.
        s.group_id = 0;
        if((status = pmem_alloc_umem(8*VM_PAGE_2MB, VM_PAGE_2MB, &rgn))){
                return -1;
                // error handling.
        }
        printf("before stack\n");
//      aspace_dump2console(id);
        printf("rgn size %d %p %p %d\n", rgn.size, rgn.start, rgn.end, rgn.end - rgn.start);
        if((status = aspace_map_region_anywhere(id, &stack, rgn.end - rgn.start, VM_WRITE|VM_READ|VM_USER, VM_PAGE_2MB, "new-stack", rgn.start))) {
                printf("couldn't map region %d\n", status);
                return -1;
        }
        printf("after stack\n");
//      aspace_dump2console(id);
        printf("stack %x stack+VM_PAGE_2MB %x\n", stack, stack+VM_PAGE_2MB);
        s.fs = stack+VM_PAGE_2MB;
        s.stack_ptr = stack+2*VM_PAGE_2MB;
        // TODO: Get this working?
        s.task_id = ANY_ID;
        snprintf(s.task_name,32,"tcasm observer");
        s.use_args = 1;
        aspace_get_myid(&my_aspace);
        if((status = pmem_alloc_umem(1000*VM_PAGE_4KB, VM_PAGE_4KB, &rgn))){
                return -1;
                // error handling.
        }
        printf("backed rgn size %d %p %p %d\n", rgn.size, rgn.start, rgn.end, rgn.end - rgn.start);
//      pmem_dump2console();
        status = arena_map_backed_region_anywhere(my_aspace, &region, pagesz*npages, pagesz*nbacking,
                        VM_USER, BK_ARENA, pagesz, "cow-region", rgn.start);
//      pmem_dump2console();
	if(status != 0) {
                printf("couldn't create backed region %d\n", status);
                return -1;
        }
        printf("region %p\n", region);
	buf = (char*)region;
//      buf = (char*)0x000000000b800000;
	printf("buf (offset) %d\n", buf);
        s.arg[0] = buf;
        s.user_id = 1;
        if((status = task_create(&s, &tid))) {
                printf("couldn't create %d\n", status);
                return -1;
        }
}

int setup1_(int *nx, int *ny, int *nz, int *ng) {
	int i;
	cte->filesize = 8*(*ng);

	cte->n[0] = *nx;
	cte->n[1] = *ny;
	cte->n[2] = *nz;
	cte->n[3] = *ng;

	for(i=0; i < 4; i++)
		cte->filesize *= cte->n[i];
// so he is truncating what do we do that corresponds to that?
// so I need to make a new aspace copy here?
	// XXX: set up observer process and shared memory?
	aspace_dump2console(1);
	ptr = (struct usda *) mmap(NULL, cte->filesize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, cte->meta_offset);
	sem_post(lock);
}

int setup2_(double *dx, double *dy, double *dz) {
	cte->d[0] = *dx;
	cte->d[1] = *dy;
	cte->d[2] = *dz;
}	

int shm_init_(int*iproc) {
	char filename[10];
	char lockname[10];
	int i;
	
	/* if (*iproc ==0){ */
	/* 	printf("PID %d ready for attach\n", getpid()); */
	/* 	fflush(stdout); */
	/* 	while (0 == i) */
	/* 		sleep(5); */
	/* } */

	//sprintf(lockname, "/sem_lock%d", *iproc);

	///lock = sem_open(lockname, O_CREAT, 0666, 0);
	//if (lock == SEM_FAILED)
	//	printf("SEM FAILED\n");

	sprintf(filename, "/%di.shm", *iproc); 
	//fd = shm_open(filename, O_CREAT|O_RDWR, 0666);

	if ( fd < 0) {
		printf("failed to open file\n");
		return 1;
	}
	rank = *iproc;

	//if(ftruncate(fd, sizeof(struct usda)) < 0) {
	//	printf("failed to truncate\n");
	//k	return -1;
	//}
	
	// This is where we create the values.
	// and then they fill up the struct
	cte = (struct usda *) mmap(NULL, sizeof(struct usda), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

	cte->outer = 0;
	cte->time_loop = 0;
	cte->finalized = 0;
	cte->meta_offset = 4096*(sizeof(struct usda)/4096)+(sizeof(struct usda)%4096 ? 4096 : 0);
	cte->filesize = 0;

	return 1;
}

int shm_allocate_(unsigned long *size, void **data)
{
	*data = (void*)(((unsigned long)ptr) + offset);
	offset += *size*8;

	return 1;
}

int msync_ (int *tint) {
	unsigned long pages, ret;

	cte->time_loop  = *tint;

	pages = msync(cte, sizeof(struct usda), MS_SYNC | MS_UPDATE);
	if (pages < 0)
		printf("MSYNC FAILED\n");

	//gettimeofday(&start, NULL);
	ret = msync(ptr, cte->filesize, MS_SYNC | MS_UPDATE);
	//gettimeofday(&stop, NULL);

	if (ret < 0) {
		printf("MSYNC FAILED\n");
		return ret;
	}

	/* else */
	/* 	pages += ret; */

	sem_post(lock);
	//total_time += pages;
	/* if(rank == 0) */
	/* 	printf("%d, %d, %lu\n", *tint, *oint, pages); */
	return 1;
}

/* int remap_ (int *size, int *fd, void **data, int *offset) { */
/* 	munmap(*data, *size); */
/* 	*data = NULL; */
/* 	*data = mmap(NULL, *size, PROT_READ | PROT_WRITE, MAP_PRIVATE, *fd, *offset); */
/* } */

/* int unmap_(int *size, void **data) { */
/* 	munmap(*data, *size); */
/* } */

int unlink_shm_()
{
	char filename[10];
	
	cte->finalized = 1;
	if(msync(cte, sizeof(struct usda), MS_SYNC | MS_UPDATE) < 0)
		printf("MSYNC FAILED\n");

	sem_post(lock);
	sprintf(filename, "/%di.shm", rank);  
	munmap(ptr, cte->filesize);
	munmap(cte, sizeof(cte));
	//shm_unlink(filename); 
	sem_close(lock);
	//printf("%d, %llu\n", rank, total_time);	
}
