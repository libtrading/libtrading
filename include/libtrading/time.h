#ifndef LIBTRADING_TIME_H
#define LIBTRADING_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>

static inline uint64_t
timespec_delta(struct timespec *before, struct timespec *after)
{
	return 1000000000 * (after->tv_sec - before->tv_sec) + (after->tv_nsec - before->tv_nsec);
}

#ifdef __cplusplus
}
#endif

#endif
