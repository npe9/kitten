/*
 * mmapault.h - template file for doing application specific producer observer app composition benchmarks
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <semaphore.h>
#include <sched.h>
#include "compbench.h"
#include <pthread.h>

typedef struct Mmap Mmap;
struct Mmap {
	sem_t full;
	sem_t empty;
	sem_t mutex;
	int i;
	int n;
	char** q;

} mmappriv;

#define STACK_SIZE 8096

void mmapinit(Work *w) {
}


void mmapspawn(Work *w) {
	int i;
	Mmap *x;
	__pid_t pid;
	void *status;
	void* stack;
	cpu_set_t mask;
	int result;

	CPU_ZERO(&mask);
	CPU_SET(w->prodcpu, &mask);
	result = sched_setaffinity(getpid(), sizeof(cpu_set_t), &mask);
	if (result < 0) {
		perror("mmap: couldn't set producer affinity");
		exit(1);
	}

//	w->private = &mmappriv;
	w->private = mmap(NULL, sizeof(struct Mmap), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
	x = (Mmap*) w->private;

	x->q = mmap(NULL, sizeof(char**) * qsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
	x->i = 0;
	x->n = 0;
	sem_init(&x->full, 1, 0);
	sem_init(&x->empty, 1, 0);
	sem_init(&x->mutex, 1, 0);

	sem_post(&x->mutex);
//	sem_post(&x->full);
	for (i = 0; i < qsize; i++) {
		sem_post(&x->empty);
		x->q[i] = mmap(NULL, w->quanta, PROT_READ | PROT_WRITE,
				MAP_ANONYMOUS | MAP_SHARED, 0, 0);
		if (x->q[i] == MAP_FAILED) {
			perror("mmap: couldn't allocate queue buffer");
			exit(1);
		}
	}
	/* prime the queue, no need to mutex, observer doesn't exist yet */
	sem_wait(&x->empty);
	w->buf = x->q[x->n++];
//	printf("shared work first w->buf %d %d %p\n", x->n-1, (x->n-1) % QUEUE_SIZE, w->buf);
//	stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
//			MAP_ANONYMOUS | MAP_PRIVATE | MAP_GROWSDOWN, -1, 0);
//	if (stack == MAP_FAILED) {
//		perror("couldn't make stack for observer");
//		exit(1);
//	}
	/* clone file system so we retain the shm_opened fd */

//	pthread_t *obsthread;
//	obsthread = malloc(sizeof(pthread_t));
	if (shouldobserve) {
//		pthread_create(obsthread, NULL, observerwrapper, w);
//		pid = clone(observerwrapper, stack + STACK_SIZE, CLONE_VM, w);
//		if (pid < 0) {
//			perror("mmap: couldn't clone observer");
//			exit(1);
//		}
		switch (pid = fork()) {
		default:
//			printf("producer\n");
			producer(w);
			sem_wait(&x->empty);
			sem_wait(&x->mutex);
			munmap(x->q[x->n % qsize], w->quanta);
			x->q[x->n++ % qsize] = NULL;
//				printf("shared work last w->buf %d %d %p\n", x->n-1, (x->n-1) % qsize, w->buf);
			sem_post(&x->mutex);
			sem_post(&x->full);
			sem_post(&x->full);

			// FIXME: wait for all observers
			if (shouldobserve) {
				wait(&status);
				//		waitpid(pid, &status, __WCLONE);
				//		munmap(stack, STACK_SIZE);
			}
			for (i = 0; i < qsize; i++) {
				if (x->q[i] == NULL)
					continue;
				munmap(x->q[i], w->quanta);
				x->q[i] = NULL;
			}
			munmap(x->q, sizeof(char**) * qsize);
			sem_destroy(&x->mutex);
			sem_destroy(&x->full);
			sem_destroy(&x->empty);
			break;
		case 0:
			CPU_ZERO(&mask);
			CPU_SET(w->obscpu, &mask);
			result = sched_setaffinity(0, sizeof(cpu_set_t), &mask);
			if (result < 0) {
				perror("mmap: couldn't set observer affinity");
				//		exit(1);
				pthread_exit(NULL);
			}
			observer(w);
			exit(0);
			break;
		}
	} else {
		/* pretend the observer consumed */
		for(i = 0; i < w->datalen/w->quanta; i++) {
			sem_post(&x->full);
			sem_post(&x->empty);
		}

		producer(w);
		sem_wait(&x->empty);
		sem_wait(&x->mutex);
		munmap(x->q[x->n % qsize], w->quanta);
		x->q[x->n++ % qsize] = NULL;
//	printf("shared work last w->buf %d %d %p\n", x->n-1, (x->n-1) % qsize, w->buf);
		sem_post(&x->mutex);
		sem_post(&x->full);
		sem_post(&x->full);

		// FIXME: wait for all observers
		if (shouldobserve) {
			wait(&status);
//		waitpid(pid, &status, __WCLONE);
//		munmap(stack, STACK_SIZE);
		}
		for (i = 0; i < qsize; i++) {
			if (x->q[i] == NULL)
				continue;
			munmap(x->q[i], w->quanta);
			x->q[i] = NULL;
		}
		munmap(x->q, sizeof(char**) * qsize);
		sem_destroy(&x->mutex);
		sem_destroy(&x->full);
		sem_destroy(&x->empty);
	}
}

int mmapsharework(Work *w) {
	Mmap *x;

	x = (Mmap*) w->private;

	sem_wait(&x->empty);
	sem_wait(&x->mutex);
	w->buf = x->q[x->n++ % qsize];
//	printf("shared work w->buf %d %d %p\n", x->n-1, (x->n-1) % qsize, w->buf);
	sem_post(&x->mutex);
	sem_post(&x->full);

	return 0;
}

Work*
mmapgetwork(Work *w) {
	Mmap *x;

	x = (Mmap*) w->private;

//	printf("getting work\n");
	sem_wait(&x->full);
	sem_wait(&x->mutex);
	w->rbuf = x->q[x->i++ % qsize];
//	printf("got work w->rbuf %d %d %p\n", x->i-1, (x->i-1) % qsize, w->rbuf);
	sem_post(&x->mutex);
	sem_post(&x->empty);
	if (w->rbuf == NULL)
		return NULL;
	return w;
}

void mmapfinalize() {

}

Bench mmapbench = { .name = "mmap", .init = mmapinit, .spawn = mmapspawn,
		.sharework = mmapsharework, .getwork = mmapgetwork, .finalize =
				mmapfinalize, };
