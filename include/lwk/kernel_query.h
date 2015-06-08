#define CTL_ASPACE 1
#define CTL_TASK 2

int
kernel_query(
  int name,
  size_t nlen,
  void **oldval,
  size_t *oldlenp,
  void *newval,
  size_t newlen
);
