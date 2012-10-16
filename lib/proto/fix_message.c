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

static int parse_tag(struct buffer *self)
{
	const char *delim;
	const char *start;
	char *end;
	int ret;

	start = buffer_start(self);
	delim = buffer_find(self, '=');

	if (!delim)
		return 0;

	if (*delim != '=')
		return 0;

	ret = strtol(start, &end, 10);
	if (end != delim)
		return 0;

	buffer_advance(self, 1);

	return ret;
}

static const char *parse_value(struct buffer *self)
{
	char *start, *end;

	start = buffer_start(self);

	end = buffer_find(self, 0x01);

	if (!end)
		return NULL;

	if (*end != 0x01)
		return NULL;

	buffer_advance(self, 1);

	return start;
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

static const char *parse_field(struct buffer *self, int tag)
{
	if (parse_tag(self) != tag) {
		next_tag(self);
		return NULL;
	}

	return parse_value(self);
}

static const char *parse_field_promisc(struct buffer *self, int *tag)
{
	*tag = parse_tag(self);

	if (!(*tag)) {
		next_tag(self);
		return NULL;
	}

	return parse_value(self);
}

static void rest_of_message(struct fix_message *self, struct buffer *buffer)
{
	int tag = 0;
	const char *tag_ptr = NULL;
	unsigned long nr_fields = 0;

	self->fields = calloc(2, sizeof(struct fix_field));
	if (!self->fields)
		return;

retry:
	if (!(tag_ptr = parse_field_promisc(buffer, &tag)))
		return;

	switch (tag) {
	case CheckSum:
		break;
	case BeginSeqNo:
	case EndSeqNo:
		self->fields[nr_fields++] = FIX_INT_FIELD(tag, strtol(tag_ptr, NULL, 10));
	default:
		goto retry;
	};

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
static bool checksum(struct fix_message *self, struct buffer *buffer)
{
	const char *start;
	int offset;

	start = buffer_start(buffer);

	/* The number of bytes between tag MsgType and buffer's start */
	offset = start - (self->msg_type - 3);

	/*
	 * Checksum tag and its trailing delimiter increase
	 * the message's length by seven bytes - "10=***\x01"
	 */
	if (buffer_size(buffer) + offset < self->body_length + 7)
		return false;

	/* Buffer's start will point to the CheckSum tag */
	buffer_advance(buffer, self->body_length - offset);

	self->check_sum = parse_field(buffer, CheckSum);
	if (!self->check_sum)
		return false;

	if (!verify_checksum(self, buffer))
		return false;

	/* Go back to analyze other fields */
	buffer_advance(buffer, start - buffer_start(buffer));

	return true;
}

static bool parse_msg_type(struct fix_message *self)
{
	self->msg_type = parse_field(self->head_buf, MsgType);

	// if third field is not MsgType -> garbled

	return self->msg_type != NULL;
}

static bool parse_body_length(struct fix_message *self)
{
	int len;
	const char *ptr = parse_field(self->head_buf, BodyLength);

	if (!ptr)
		return false;

	len = strtol(ptr, NULL, 10);
	self->body_length = len;

	if (len <= 0 || len > FIX_MAX_MESSAGE_SIZE)
		return false;

	return true;
}

static bool parse_begin_string(struct fix_message *self)
{
	self->begin_string = parse_field(self->head_buf, BeginString);

	// if first field is not BeginString -> garbled
	// if BeginString is invalid or empty -> garbled

	return self->begin_string != NULL;
}

static bool first_three_fields(struct fix_message *self)
{
	if (!parse_begin_string(self))
		return false;

	if (!parse_body_length(self))
		return false;

	return parse_msg_type(self);
}

struct fix_message *fix_message_parse(struct buffer *buffer)
{
	struct fix_message *self;
	unsigned long size;
	const char *start;

	self	= fix_message_new();
	if (!self)
		return NULL;

	self->head_buf = buffer;

retry:
	start	= buffer_start(buffer);
	size	= buffer_size(buffer);

	if (!size)
		goto fail;

	if (!first_three_fields(self))
		goto fail;

	if (!checksum(self, buffer))
		goto fail;

	rest_of_message(self, buffer);

	return self;

fail:
	if (size > FIX_MAX_MESSAGE_SIZE)
		goto retry;

	buffer_advance(buffer, start - buffer_start(buffer));
	fix_message_free(self);

	return NULL;
}

struct fix_field *fix_message_has_tag(struct fix_message *self, int tag)
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

struct fix_message *fix_message_new(void)
{
	struct fix_message *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

	return self;
}

void fix_message_free(struct fix_message *self)
{
	if (!self)
		return;

	free(self->fields);
	free(self);
}

bool fix_message_type_is(struct fix_message *self, const char *expected_type)
{
	if (!self->msg_type)
		return false;

	return strncmp(self->msg_type, expected_type, strlen(expected_type)) == 0;
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
	char fmt[64], buf[64];
	struct timeval tv;
	struct tm *tm;
	int i;

	gettimeofday(&tv, NULL);
	tm = gmtime(&tv.tv_sec);
	assert(tm != NULL);

	/* YYYYMMDD-HH:MM:SS.sss */
	strftime(fmt, sizeof fmt, "%Y%m%d-%H:%M:%S", tm);
	snprintf(buf, sizeof buf, "%s.%03ld", fmt, (long)tv.tv_usec / 1000);

	/* standard header */
	msg_type	= FIX_STRING_FIELD(MsgType, self->msg_type);
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
