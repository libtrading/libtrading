#include "fix/test/harness.h"
#include "test-suite.h"

#include "fix/buffer.h"
#include "fix/boe.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

void test_boe_login_request(void)
{
	struct boe_login_request *login;
	struct boe_header header;
	struct buffer *buf;
	int fd;

	buf = buffer_new(1024);

	fd = open("test/protocol/boe/login-request-message.bin", O_RDONLY);
	fail_if(fd < 0);

	fail_if(buffer_read(buf, fd) < 0);

	fail_if(boe_decode_header(buf, &header) < 0);

	assert_int_equals(BOE_MAGIC, header.StartOfMessage);
	assert_int_equals(131, header.MessageLength);
	assert_int_equals(LoginRequest, header.MessageType);
	assert_int_equals(0, header.MatchingUnit);
	assert_int_equals(0, header.SequenceNumber);

	login = boe_decode_login_request(&header, buf);
	assert_true(login != NULL);

	assert_mem_equals("0001", login->SessionSubID, BOE_SESSION_SUB_ID_LEN);
	assert_mem_equals("TEST", login->Username, BOE_USERNAME_LEN);
	assert_mem_equals("TESTING\0\0\0", login->Password, BOE_PASSWORD_LEN);
	assert_int_equals(0, login->NoUnspecifiedUnitReplay);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->OrderAcknowledgementBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->OrderRejectedBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->OrderModifiedBitfields);
	assert_int_equals(0, login->OrderRestatedBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->UserModifyRejectedBitfields);
	assert_int_equals(0, login->OrderCancelledBitfields);
	assert_int_equals(0, login->CancelRejectedBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->OrderExecutionBitfields);
	assert_int_equals(Symbol, login->TradeCancelOrCorrectBitfields);
	assert_int_equals(0, login->ReservedBitfields1);
	assert_int_equals(0, login->ReservedBitfields2);
	assert_int_equals(3, login->NumberOfUnits);

	assert_int_equals(1, login->Units[0].UnitNumber);
	assert_int_equals(113482, login->Units[0].UnitSequence);

	assert_int_equals(2, login->Units[1].UnitNumber);
	assert_int_equals(0, login->Units[1].UnitSequence);

	assert_int_equals(4, login->Units[2].UnitNumber);
	assert_int_equals(41337, login->Units[2].UnitSequence);

	buffer_delete(buf);

	fail_if(close(fd) < 0);

	free(login);
}

void test_boe_login_response(void)
{
	struct boe_login_response *login;
	struct boe_header header;
	struct buffer *buf;
	int fd;

	buf = buffer_new(1024);

	fd = open("test/protocol/boe/login-response-message.bin", O_RDONLY);
	fail_if(fd < 0);

	fail_if(buffer_read(buf, fd) < 0);

	fail_if(boe_decode_header(buf, &header) < 0);

	assert_int_equals(BOE_MAGIC, header.StartOfMessage);
	assert_int_equals(183, header.MessageLength);
	assert_int_equals(LoginResponse, header.MessageType);
	assert_int_equals(0, header.MatchingUnit);
	assert_int_equals(0, header.SequenceNumber);

	login = boe_decode_login_response(&header, buf);
	assert_true(login != NULL);

	assert_int_equals('A', login->LoginResponseStatus);
	assert_mem_equals("Accepted", login->LoginResponseText, strlen("Accepted"));
	assert_int_equals(0, login->NoUnspecifiedUnitReplay);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->OrderAcknowledgementBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->OrderRejectedBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->OrderModifiedBitfields);
	assert_int_equals(0, login->OrderRestatedBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->UserModifyRejectedBitfields);
	assert_int_equals(0, login->OrderCancelledBitfields);
	assert_int_equals(0, login->CancelRejectedBitfields);
	assert_int_equals(Symbol|ClearingFirm|ClearingAccount, login->OrderExecutionBitfields);
	assert_int_equals(Symbol, login->TradeCancelOrCorrectBitfields);
	assert_int_equals(0, login->ReservedBitfields1);
	assert_int_equals(0, login->ReservedBitfields2);
	assert_int_equals(4, login->NumberOfUnits);

	assert_int_equals(1, login->Units[0].UnitNumber);
	assert_int_equals(113482, login->Units[0].UnitSequence);

	assert_int_equals(2, login->Units[1].UnitNumber);
	assert_int_equals(0, login->Units[1].UnitSequence);

	assert_int_equals(3, login->Units[2].UnitNumber);
	assert_int_equals(0, login->Units[2].UnitSequence);

	assert_int_equals(4, login->Units[3].UnitNumber);
	assert_int_equals(41337, login->Units[3].UnitSequence);

	buffer_delete(buf);

	fail_if(close(fd) < 0);

	free(login);
}

void test_boe_logout_request(void)
{
	struct boe_header header;
	struct buffer *buf;
	int fd;

	buf = buffer_new(1024);

	fd = open("test/protocol/boe/logout-request-message.bin", O_RDONLY);
	fail_if(fd < 0);

	fail_if(buffer_read(buf, fd) < 0);

	fail_if(boe_decode_header(buf, &header) < 0);

	assert_int_equals(BOE_MAGIC, header.StartOfMessage);
	assert_int_equals(8, header.MessageLength);
	assert_int_equals(LogoutRequest, header.MessageType);
	assert_int_equals(0, header.MatchingUnit);
	assert_int_equals(0, header.SequenceNumber);

	buffer_delete(buf);

	fail_if(close(fd) < 0);
}

void test_boe_client_heartbeat(void)
{
	struct boe_header header;
	struct buffer *buf;
	int fd;

	buf = buffer_new(1024);

	fd = open("test/protocol/boe/client-heartbeat-message.bin", O_RDONLY);
	fail_if(fd < 0);

	fail_if(buffer_read(buf, fd) < 0);

	fail_if(boe_decode_header(buf, &header) < 0);

	assert_int_equals(BOE_MAGIC, header.StartOfMessage);
	assert_int_equals(8, header.MessageLength);
	assert_int_equals(ClientHeartbeat, header.MessageType);
	assert_int_equals(0, header.MatchingUnit);
	assert_int_equals(0, header.SequenceNumber);

	buffer_delete(buf);

	fail_if(close(fd) < 0);
}

void test_boe_logout(void)
{
	struct boe_logout *logout;
	struct boe_header header;
	struct buffer *buf;
	int fd;

	buf = buffer_new(1024);

	fd = open("test/protocol/boe/logout-message.bin", O_RDONLY);
	fail_if(fd < 0);

	fail_if(buffer_read(buf, fd) < 0);

	fail_if(boe_decode_header(buf, &header) < 0);

	assert_int_equals(BOE_MAGIC, header.StartOfMessage);
	assert_int_equals(89, header.MessageLength);
	assert_int_equals(Logout, header.MessageType);
	assert_int_equals(0, header.MatchingUnit);
	assert_int_equals(0, header.SequenceNumber);

	logout = boe_decode_logout(&header, buf);
	assert_true(logout != NULL);

	assert_int_equals('U', logout->LogoutReason);
	assert_mem_equals("User", logout->LogoutReasonText, 4);
	assert_int_equals(103231, logout->LastReceivedSequenceNumber);

	assert_int_equals(3, logout->NumberOfUnits);

	assert_int_equals(1, logout->Units[0].UnitNumber);
	assert_int_equals(113482, logout->Units[0].UnitSequence);

	assert_int_equals(2, logout->Units[1].UnitNumber);
	assert_int_equals(0, logout->Units[1].UnitSequence);

	assert_int_equals(4, logout->Units[2].UnitNumber);
	assert_int_equals(41337, logout->Units[2].UnitSequence);

	buffer_delete(buf);

	fail_if(close(fd) < 0);

	free(logout);
}
