#ifndef	LIBTRADING_ITOA_H
#define	LIBTRADING_ITOA_H

#include <libtrading/types.h>

#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

int uitoa(unsigned int n, char *s);
int checksumtoa(int n, char *s);
int i64toa(int64_t n, char *s);
int itoa(int n, char *s);

#endif
