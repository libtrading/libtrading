#include "test-suite.h"
#include "harness.h"

#include "trading/mbt_quote_message.h"
#include "trading/buffer.h"

#include <stdlib.h>

static struct mbt_quote_message *mbt_msg;
static struct buffer *buf;

static void setup(void)
{
	buf = buffer_new(1024);
}

static void teardown(void)
{
	buffer_delete(buf);

	mbt_quote_message_delete(mbt_msg);

	mbt_msg = NULL;
}

void test_mbt_quote_decode_logging_on(void)
{
	struct mbt_quote_logging_on *logon;

	setup();

	buffer_printf(buf, "L|100=USERNAME;101=PASSWORD\n");

	mbt_msg = mbt_quote_message_decode(buf);

	assert_int_equals(MBT_QUOTE_LOGGING_ON, mbt_msg->Type);

	logon = mbt_quote_message_payload(mbt_msg);

	assert_str_equals("USERNAME", logon->UserName, 8);
	assert_str_equals("PASSWORD", logon->Password, 8);

	teardown();
}
