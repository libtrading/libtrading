#include "test-suite.h"
#include "harness.h"

#include "trading/fix_message.h"
#include "trading/buffer.h"

#include <string.h>

static const char		*expected;
static struct fix_field		field;
static struct buffer		*buf;

static void setup(void)
{
	buf = buffer_new(1024);
}

static void teardown(void)
{
	buffer_delete(buf);
}

void test_fix_field_unparse_str(void)
{
	setup();

	expected = "99=abcdef\1";

	field = FIX_STRING_FIELD(99, "abcdef");

	fix_field_unparse(&field, buf);

	assert_str_equals(expected, buf->data, strlen(expected));

	teardown();
}

void test_fix_field_unparse_checksum(void)
{
	setup();

	expected = "10=001\1";

	field = FIX_CHECKSUM_FIELD(CheckSum, 1);

	fix_field_unparse(&field, buf);

	assert_str_equals(expected, buf->data, strlen(expected));

	teardown();
}
