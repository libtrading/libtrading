#include "libtrading/compat.h"

#include <sys/time.h>

int clock_gettime(clockid_t clock_id, struct timespec *tsp)
{
	struct timeval tvp;

	gettimeofday(&tvp, NULL);

	tsp->tv_sec	= tvp.tv_sec;
	tsp->tv_nsec	= tvp.tv_usec * 1000;

	return 0;
}
