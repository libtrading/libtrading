#ifndef LIBTRADING_COMPAT_H
#define LIBTRADING_COMPAT_H

#ifdef CONFIG_NEED_CLOCK_GETTIME
#include <time.h>

enum {
	CLOCK_MONOTONIC,
	CLOCK_REALTIME,
};

typedef int clockid_t;

int clock_gettime(clockid_t clock_id, struct timespec *tp);
#endif

#endif /* LIBTRADING_COMPAT_H */
