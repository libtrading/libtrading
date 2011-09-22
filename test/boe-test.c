#include "fix/test/harness.h"
#include "test-suite.h"

#include "fix/buffer.h"
#include "fix/boe.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

void test_boe(void)
{
	struct boe_login_request login;
	struct boe_header header;
	struct buffer *buf;
	int fd;

	buf = buffer_new(1024);

	fd = open("examples/login-request-message", O_RDONLY);
	fail_if(fd < 0);

	fail_if(buffer_read(buf, fd) < 0);

	fail_if(boe_decode_header(buf, &header) < 0);

	assert_int_equals(BOE_MAGIC, header.StartOfMessage);
	assert_int_equals(131, header.MessageLength);
	assert_int_equals(LoginRequest, header.MessageType);
	assert_int_equals(0, header.MatchingUnit);
	assert_int_equals(0, header.SequenceNumber);

	fail_if(boe_decode_login_request(buf, &login) < 0);

	assert_mem_equals("0001", login.SessionSubID, BOE_SESSION_SUB_ID_LEN);
	assert_mem_equals("TEST", login.Username, BOE_USERNAME_LEN);
	assert_mem_equals("TESTING\0\0\0", login.Username, BOE_PASSWORD_LEN);
	assert_int_equals(0, login.NoUnspecifiedUnitReplay);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login.OrderAcknowledgementBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login.OrderRejectedBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login.OrderModifiedBitfields);
	assert_int_equals(0, login.OrderRestatedBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login.UserModifyRejectedBitfields);
	assert_int_equals(0, login.OrderCancelledBitfields);
	assert_int_equals(0, login.CancelRejectedBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login.OrderExecutionBitfields);
	assert_int_equals(Symbol, login.TradeCancelOrCorrectBitfields);
	assert_int_equals(0, login.ReservedBitfields1);
	assert_int_equals(0, login.ReservedBitfields2);
	assert_int_equals(3, login.NumberOfUnits);

	buffer_delete(buf);

	fail_if(close(fd) < 0);
}
