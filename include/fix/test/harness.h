#ifndef FIX_HARNESS_H
#define FIX_HARNESS_H

#include <stdbool.h>
#include <stddef.h>

#define assert_true(value) do_assert_true(__FILE__, __LINE__, value)
#define assert_false(value) do_assert_false(__FILE__, __LINE__, value)

void assert_is_null(const void *p);
void assert_int_equals(int expected, int actual);
void assert_str_equals(const char *s1, const char *s2, size_t len);
void do_assert_true(const char *filename, int line, bool value);
void do_assert_false(const char *filename, int line, bool value);

#endif /* FIX_HARNESS_H */
