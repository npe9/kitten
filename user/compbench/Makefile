BASE=..
include $(BASE)/Makefile.header


PROGRAMS = compbench
compbench_SOURCES = compbench.c tcasm.c  kdbus.c kdbus-util.c kdbus-enum.c xpmem.c pipebench.c mmap.c filebench.c posixq.c func.c base.c arena.c
compbench_LDADD   = -llwk -lpthread  -lrt -lxpmem

CARGS=-g -lrt
MPICC=mpicc
CC=cc



%.exe: %.c
	$(MPICC) $(CARGS) $< -o $@ -lrt

#compbench: compbench.c tcasm.c mpi.c kdbus.c kdbus-util.c kdbus-enum.c xpmem.c pipebench.c mmap.c filebench.c posixq.c func.c base.c compbench.h benches.h
##	$(MPICC) -g  -o $@ compbench.c kdbus.c kdbus-util.c kdbus-enum.c tcasm.c mpi.c xpmem.c pipebench.c mmap.c filebench.c posixq.c func.c base.c -lrt -lxpmem
#	cc -g  -o $@ compbench.c kdbus.c kdbus-util.c kdbus-enum.c tcasm.c mpi.c xpmem.c pipebench.c mmap.c filebench.c posixq.c func.c base.c -lrt -lxpmem

include $(BASE)/Makefile.footer
	