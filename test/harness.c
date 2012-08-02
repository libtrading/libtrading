#include "harness.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void assert_is_null(const void *p)
{
	if (p) {
		printf("Expected: NULL, but was %p\n", p);
		exit(EXIT_FAILURE);
	}
}

void do_assert_true(const char *filename, int line, bool value)
{
	if (value == false) {
		printf("%s:%d: Expected: true, but was false\n", filename, line);
		exit(EXIT_FAILURE);
	}
}

void do_assert_false(const char *filename, int line, bool value)
{
	if (value == true) {
		printf("%s:%d: Expected: false, but was true\n", filename, line);
		exit(EXIT_FAILURE);
	}
}

void assert_int_equals(long long expected, long long actual)
{
	if (expected != actual) {
		printf("Expected: %lld, but was %lld\n", expected, actual);
		exit(EXIT_FAILURE);
	}
}

void assert_mem_equals(const void *s1, const void *s2, size_t len)
{
	if (!s2) {
		printf("Expected: '%s', but was: NULL\n", (const char *) s1);
		exit(EXIT_FAILURE);
	}

	if (memcmp(s1, s2, len)) {
		printf("Expected: '%s', but was: '%s'\n", (const char *) s1, (const char *) s2);
		exit(EXIT_FAILURE);
	}
}

void assert_str_equals(const char *s1, const char *s2, size_t len)
{
	if (!s2) {
		printf("Expected: '%s', but was: NULL\n", s1);
		exit(EXIT_FAILURE);
	}

	if (strncmp(s1, s2, len)) {
		printf("Expected: '%s', but was: '%s'\n", s1, s2);
		exit(EXIT_FAILURE);
	}
}
