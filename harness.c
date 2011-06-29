#include "fix/test/harness.h"

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

void assert_int_equals(int expected, int actual)
{
	if (expected != actual) {
		printf("Expected: %d, but was %d\n", expected, actual);
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
