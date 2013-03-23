#include "libtrading/proto/fix_message.h"

#include "libtrading/read-write.h"
#include "libtrading/buffer.h"
#include "libtrading/array.h"

#include <inttypes.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

static const char *fix_msg_types[FIX_MSG_TYPE_MAX] = {
	[FIX_MSG_TYPE_HEARTBEAT]		= "0",
	[FIX_MSG_TYPE_TEST_REQUEST]		= "1",
	[FIX_MSG_TYPE_RESEND_REQUEST]		= "2",
	[FIX_MSG_TYPE_REJECT]			= "3",
	[FIX_MSG_TYPE_SEQUENCE_RESET]		= "4",
	[FIX_MSG_TYPE_LOGOUT]			= "5",
	[FIX_MSG_TYPE_EXECUTION_REPORT]		= "8",
	[FIX_MSG_TYPE_LOGON]			= "A",
	[FIX_MSG_TYPE_NEW_ORDER_SINGLE]		= "D",
};

enum fix_msg_type fix_msg_type_parse(const char *s)
{
	switch (s[1]) {
	case 0x01: {
		/*
		 * Single-character message type:
		 */
		switch (s[0]) {
		case '0': return FIX_MSG_TYPE_HEARTBEAT;
		case '1': return FIX_MSG_TYPE_TEST_REQUEST;
		case '2': return FIX_MSG_TYPE_RESEND_REQUEST;
		case '3': return FIX_MSG_TYPE_REJECT;
		case '4': return FIX_MSG_TYPE_SEQUENCE_RESET;
		case '5': return FIX_MSG_TYPE_LOGOUT;
		case '8': return FIX_MSG_TYPE_EXECUTION_REPORT;
		case 'A': return FIX_MSG_TYPE_LOGON;
		case 'D': return FIX_MSG_TYPE_NEW_ORDER_SINGLE;
		default : return FIX_MSG_TYPE_UNKNOWN;
		}
	}
	default:
		return FIX_MSG_TYPE_UNKNOWN;
	}
}

static int parse_tag(struct buffer *self, int *tag)
{
	const char *delim;
	const char *start;
	char *end;
	int ret;

	start = buffer_start(self);
	delim = buffer_find(self, '=');

	if (!delim || *delim != '=')
		return FIX_MSG_STATE_PARTIAL;

	ret = strtol(start, &end, 10);
	if (end != delim)
		return FIX_MSG_STATE_GARBLED;

	buffer_advance(self, 1);

	*tag = ret;

	return 0;
}

static int parse_value(struct buffer *self, const char **value)
{
	char *start, *end;

	start = buffer_start(self);

	end = buffer_find(self, 0x01);

	if (!end || *end != 0x01)
		return FIX_MSG_STATE_PARTIAL;

	buffer_advance(self, 1);

	*value = start;

	return 0;
}

static void next_tag(struct buffer *self)
{
	char *delim;

	delim = buffer_find(self, 0x01);

	if (!delim)
		return;

	if (*delim != 0x01)
		return;

	buffer_advance(self, 1);

	return;
}

static int parse_field(struct buffer *self, int tag, const char **value)
{
	int ptag, ret;

	ret = parse_tag(self, &ptag);

	if (ret)
		goto fail;
	else if (ptag != tag) {
		ret = FIX_MSG_STATE_GARBLED;
		goto fail;
	}

	return parse_value(self, value);

fail:
	next_tag(self);
	return ret;
}

static int parse_field_promisc(struct buffer *self, int *tag, const char **value)
{
	int ret;

	ret = parse_tag(self, tag);

	if (ret)
		goto fail;

	return parse_value(self, value);

fail:
	next_tag(self);
	return ret;
}

static enum fix_type fix_tag_type(int tag)
{
	switch (tag) {
	case CheckSum:		return FIX_TYPE_CHECKSUM;
	case BeginSeqNo:	return FIX_TYPE_INT;
	case EndSeqNo:		return FIX_TYPE_INT;
	case NewSeqNo:		return FIX_TYPE_INT;
	case GapFillFlag:	return FIX_TYPE_STRING;
	case PossDupFlag:	return FIX_TYPE_STRING;
	case TestReqID:		return FIX_TYPE_STRING;
	case MsgSeqNum:		return FIX_TYPE_MSGSEQNUM;
	case LeavesQty:		return FIX_TYPE_FLOAT;
	case OrderQty:		return FIX_TYPE_FLOAT;
	case CumQty:		return FIX_TYPE_FLOAT;
	case AvgPx:		return FIX_TYPE_FLOAT;
	case Price:		return FIX_TYPE_FLOAT;
	case TransactTime:	return FIX_TYPE_STRING;
	case OrdStatus:		return FIX_TYPE_STRING;
	case ExecType:		return FIX_TYPE_STRING;
	case Account:		return FIX_TYPE_STRING;
	case ClOrdID:		return FIX_TYPE_STRING;
	case OrderID:		return FIX_TYPE_STRING;
	case OrdType:		return FIX_TYPE_STRING;
	case ExecID:		return FIX_TYPE_STRING;
	case Symbol:		return FIX_TYPE_STRING;
	case Side:		return FIX_TYPE_STRING;

	default:		return FIX_TYPE_STRING;	/* unrecognized tag */
	}
}

static void rest_of_message(struct fix_message *self, struct buffer *buffer)
{
	int tag = 0;
	const char *tag_ptr = NULL;
	unsigned long nr_fields = 0;
	enum fix_type type;

	self->nr_fields = 0;

retry:
	if (parse_field_promisc(buffer, &tag, &tag_ptr))
		return;

	type = fix_tag_type(tag);

	switch (type) {
	case FIX_TYPE_INT:
		self->fields[nr_fields++] = FIX_INT_FIELD(tag, strtol(tag_ptr, NULL, 10));
		goto retry;
	case FIX_TYPE_FLOAT:
		self->fields[nr_fields++] = FIX_FLOAT_FIELD(tag, strtod(tag_ptr, NULL));
		goto retry;
	case FIX_TYPE_CHAR:
		goto retry;
	case FIX_TYPE_STRING:
		self->fields[nr_fields++] = FIX_STRING_FIELD(tag, tag_ptr);
		goto retry;
	case FIX_TYPE_CHECKSUM:
		break;
	case FIX_TYPE_MSGSEQNUM:
		self->msg_seq_num = strtol(tag_ptr, NULL, 10);
		goto retry;
	default:
		goto retry;
	}

	self->nr_fields = nr_fields;
}

static bool verify_checksum(struct fix_message *self, struct buffer *buffer)
{
	uint8_t cksum, actual;

	cksum	= strtol(self->check_sum, NULL, 10);

	actual	= buffer_sum_range(buffer, self->begin_string - 2, self->check_sum - 3);

	return actual == cksum;
}

/*
 * The function assumes that the following patterns have fixed sizes:
 * - "BeginString=" ("8=") is 2 bytes long
 * - "CheckSum=" ("10=") is 3 bytes long
 * - "MsgType=" ("35=") is 3 bytes long
 */
static int checksum(struct fix_message *self, struct buffer *buffer)
{
	const char *start;
	int offset;
	int ret;

	start = buffer_start(buffer);

	/* The number of bytes between tag MsgType and buffer's start */
	offset = start - (self->msg_type - 3);

	/*
	 * Checksum tag and its trailing delimiter increase
	 * the message's length by seven bytes - "10=***\x01"
	 */
	if (buffer_size(buffer) + offset < self->body_length + 7) {
		ret = FIX_MSG_STATE_PARTIAL;
		goto exit;
	}

	/* Buffer's start will point to the CheckSum tag */
	buffer_advance(buffer, self->body_length - offset);

	ret = parse_field(buffer, CheckSum, &self->check_sum);
	if (ret)
		goto exit;

	if (!verify_checksum(self, buffer)) {
		ret = FIX_MSG_STATE_GARBLED;
		goto exit;
	}

	/* Go back to analyze other fields */
	buffer_advance(buffer, start - buffer_start(buffer));

exit:
	return ret;
}

static int parse_msg_type(struct fix_message *self)
{
	int ret;

	ret = parse_field(self->head_buf, MsgType, &self->msg_type);

	if (ret)
		goto exit;

	self->type = fix_msg_type_parse(self->msg_type);

	// if third field is not MsgType -> garbled

	if (fix_message_type_is(self, FIX_MSG_TYPE_UNKNOWN))
		ret = FIX_MSG_STATE_GARBLED;

exit:
	return ret;
}

static int parse_body_length(struct fix_message *self)
{
	int len, ret;
	const char *ptr;

	ret = parse_field(self->head_buf, BodyLength, &ptr);

	if (ret)
		goto exit;

	len = strtol(ptr, NULL, 10);
	self->body_length = len;

	if (len <= 0 || len > FIX_MAX_MESSAGE_SIZE)
		ret = FIX_MSG_STATE_GARBLED;

exit:
	return ret;
}

static int parse_begin_string(struct fix_message *self)
{
	// if first field is not BeginString -> garbled
	// if BeginString is invalid or empty -> garbled

	return parse_field(self->head_buf, BeginString, &self->begin_string);
}

static int first_three_fields(struct fix_message *self)
{
	int ret;

	ret = parse_begin_string(self);
	if (ret)
		goto exit;

	ret = parse_body_length(self);
	if (ret)
		goto exit;

	return parse_msg_type(self);

exit:
	return ret;
}

int fix_message_parse(struct fix_message *self, struct buffer *buffer)
{
	int ret = FIX_MSG_STATE_PARTIAL;
	unsigned long size;
	const char *start;

	self->head_buf = buffer;

retry:
	start	= buffer_start(buffer);
	size	= buffer_size(buffer);

	if (!size)
		goto fail;

	ret = first_three_fields(self);
	if (ret)
		goto fail;

	ret = checksum(self, buffer);
	if (ret)
		goto fail;

	rest_of_message(self, buffer);

	return 0;

fail:
	if (ret != FIX_MSG_STATE_PARTIAL)
		goto retry;

	buffer_advance(buffer, start - buffer_start(buffer));

	return -1;
}

struct fix_field *fix_get_field(struct fix_message *self, int tag)
{
	unsigned long i;

	for (i = 0; i < self->nr_fields; i++) {
		if (self->fields[i].tag == tag)
			return &self->fields[i];
	}

	return NULL;
}

void fix_message_validate(struct fix_message *self)
{
	// if MsgSeqNum is missing -> logout, terminate
}

const char *fix_get_string(struct fix_field *field, char *buffer, unsigned long len)
{
	unsigned long count;
	const char *start, *end;

	start	= field->string_value;

	end	= memchr(start, 0x01, len);

	if (!end)
		return NULL;

	count	= end - start;

	if (len < count)
		return NULL;

	strncpy(buffer, start, count);

	buffer[count] = '\0';

	return buffer;
}

struct fix_message *fix_message_new(void)
{
	struct fix_message *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

	self->fields = calloc(FIX_MAX_FIELD_NUMBER, sizeof(struct fix_field));
	if (!self->fields) {
		fix_message_free(self);
		return NULL;
	}

	return self;
}

void fix_message_free(struct fix_message *self)
{
	if (!self)
		return;

	free(self->fields);
	free(self);
}

bool fix_message_type_is(struct fix_message *self, enum fix_msg_type type)
{
	return self->type == type;
}

bool fix_field_unparse(struct fix_field *self, struct buffer *buffer)
{
	switch (self->type) {
	case FIX_TYPE_STRING:
		return buffer_printf(buffer, "%d=%s\x01", self->tag, self->string_value);
	case FIX_TYPE_CHAR:
		return buffer_printf(buffer, "%d=%c\x01", self->tag, self->char_value);
	case FIX_TYPE_FLOAT:
		return buffer_printf(buffer, "%d=%f\x01", self->tag, self->float_value);
	case FIX_TYPE_INT:
		return buffer_printf(buffer, "%d=%" PRId64 "\x01", self->tag, self->int_value);
	case FIX_TYPE_CHECKSUM:
		return buffer_printf(buffer, "%d=%03" PRId64 "\x01", self->tag, self->int_value);
	default:
		/* unknown type */
		break;
	};

	return false;
}

char *fix_timestamp_now(char *buf, size_t len)
{
	struct timeval tv;
	struct tm *tm;
	char fmt[64];

	gettimeofday(&tv, NULL);

	tm = gmtime(&tv.tv_sec);
	if (!tm)
		return NULL;

	/* YYYYMMDD-HH:MM:SS.sss */
	strftime(fmt, sizeof fmt, "%Y%m%d-%H:%M:%S", tm);

	snprintf(buf, len, "%s.%03ld", fmt, (long)tv.tv_usec / 1000);

	return buf;
}

static void fix_message_unparse(struct fix_message *self)
{
	struct fix_field sender_comp_id;
	struct fix_field target_comp_id;
	struct fix_field begin_string;
	struct fix_field sending_time;
	struct fix_field body_length;
	struct fix_field msg_seq_num;
	struct fix_field check_sum;
	struct fix_field msg_type;
	unsigned long cksum;
	char buf[64];
	int i;

	fix_timestamp_now(buf, sizeof buf);

	/* standard header */
	msg_type	= FIX_STRING_FIELD(MsgType, fix_msg_types[self->type]);
	sender_comp_id	= FIX_STRING_FIELD(SenderCompID, self->sender_comp_id);
	target_comp_id	= FIX_STRING_FIELD(TargetCompID, self->target_comp_id);
	msg_seq_num	= FIX_INT_FIELD   (MsgSeqNum, self->msg_seq_num);
	sending_time	= FIX_STRING_FIELD(SendingTime, buf);

	/* body */
	fix_field_unparse(&msg_type, self->body_buf);
	fix_field_unparse(&sender_comp_id, self->body_buf);
	fix_field_unparse(&target_comp_id, self->body_buf);
	fix_field_unparse(&msg_seq_num, self->body_buf);
	fix_field_unparse(&sending_time, self->body_buf);

	for (i = 0; i < self->nr_fields; i++)
		fix_field_unparse(&self->fields[i], self->body_buf);

	/* head */
	begin_string	= FIX_STRING_FIELD(BeginString, self->begin_string);
	body_length	= FIX_INT_FIELD(BodyLength, buffer_size(self->body_buf));

	fix_field_unparse(&begin_string, self->head_buf);
	fix_field_unparse(&body_length, self->head_buf);

	/* trailer */
	cksum		= buffer_sum(self->head_buf) + buffer_sum(self->body_buf);
	check_sum	= FIX_CHECKSUM_FIELD(CheckSum, cksum % 256);
	fix_field_unparse(&check_sum, self->body_buf);
}

int fix_message_send(struct fix_message *self, int sockfd, int flags)
{
	struct iovec iov[2];
	int ret = 0;

	fix_message_unparse(self);

	buffer_to_iovec(self->head_buf, &iov[0]);
	buffer_to_iovec(self->body_buf, &iov[1]);

	if (xwritev(sockfd, iov, ARRAY_SIZE(iov)) < 0) {
		ret = -1;
		goto error_out;
	}

error_out:
	self->head_buf = self->body_buf = NULL;

	return ret;
}
