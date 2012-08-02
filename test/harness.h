#ifndef LIBTRADING_HARNESS_H
#define LIBTRADING_HARNESS_H

#include <stdbool.h>
#include <stddef.h>

#define assert_true(value) do_assert_true(__FILE__, __LINE__, value)
#define assert_false(value) do_assert_false(__FILE__, __LINE__, value)

#define fail_if(condition)			\
	do {					\
		assert_false(condition);	\
	} while (0)

void assert_is_null(const void *p);
void assert_int_equals(long long expected, long long actual);
void assert_mem_equals(const void *s1, const void *s2, size_t len);
void assert_str_equals(const char *s1, const char *s2, size_t len);
void do_assert_true(const char *filename, int line, bool value);
void do_assert_false(const char *filename, int line, bool value);

#endif /* LIBTRADING_HARNESS_H */
