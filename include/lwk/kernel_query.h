#define CTL_ASPACE 1
#define CTL_TASK 2

#define CTL_PMEM 1

int
kernel_query(
  int name,
  size_t nlen,
  void **oldval,
  size_t *oldlenp,
  void *newval,
  size_t newlen
);

int
kernel_set(
	int name,
	void *newval,
	size_t newlen
);
