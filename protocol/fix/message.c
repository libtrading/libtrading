#include "trading/fix_message.h"

#include "fix/read-write.h"
#include "fix/buffer.h"
#include "fix/core.h"

#include <inttypes.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

/*
 * Maximum FIX message size
 */
#define MAX_HEAD_LEN		32UL
#define MAX_BODY_LEN		128UL
#define MAX_MESSAGE_SIZE	(MAX_HEAD_LEN + MAX_BODY_LEN)

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

	*end = '\0';

	buffer_advance(self, 1);

	return start;
}

static const char *parse_field(struct buffer *self, int tag)
{
	if (parse_tag(self) != tag)
		return NULL;

	return parse_value(self);
}

static void rest_of_message(struct fix_message *self, struct buffer *buffer)
{
}

static bool checksum(struct fix_message *self, struct buffer *buffer)
{
	// if BodyLength is invalid (e.g. does not point to CheckSum) -> garbled
	// if CheckSum does not match or invalid or empty -> garbled

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
	parse_field(self->head_buf, BodyLength);

	// if second field is not BodyLength -> garbled

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

	self		= fix_message_new();
	if (!self) {
		buffer_delete(buffer);
		return NULL;
	}

	self->head_buf = buffer;

	if (!first_three_fields(self))
		goto garbled;

	if (!checksum(self, buffer))
		goto garbled;

	rest_of_message(self, buffer);

	return self;

garbled:
	/* TODO: Make sure buffer is either empty or start of it points to a new BeginString */
	fix_message_free(self);

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

	if (self->head_buf)
		buffer_delete(self->head_buf);

	if (self->body_buf)
		buffer_delete(self->body_buf);

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
	struct fix_field body_length;
	struct fix_field check_sum;
	struct fix_field msg_type;
	unsigned long cksum;

	/* body */
	sender_comp_id	= FIX_STRING_FIELD(SenderCompID, self->sender_comp_id);
	target_comp_id	= FIX_STRING_FIELD(TargetCompID, self->target_comp_id);

	fix_field_unparse(&sender_comp_id, self->body_buf);
	fix_field_unparse(&target_comp_id, self->body_buf);

	/* head */
	begin_string	= FIX_STRING_FIELD(BeginString, self->begin_string);
	body_length	= FIX_INT_FIELD(BodyLength, buffer_size(self->body_buf));
	msg_type	= FIX_STRING_FIELD(MsgType, self->msg_type);

	fix_field_unparse(&begin_string, self->head_buf);
	fix_field_unparse(&body_length, self->head_buf);
	fix_field_unparse(&msg_type, self->head_buf);

	/* trailer */
	cksum		= buffer_sum(self->head_buf) + buffer_sum(self->body_buf);
	check_sum	= FIX_CHECKSUM_FIELD(CheckSum, cksum % 256);
	fix_field_unparse(&check_sum, self->body_buf);
}

int fix_message_send(struct fix_message *self, int sockfd, int flags)
{
	struct iovec iov[2];
	int ret = 0;

	self->head_buf = buffer_new(MAX_HEAD_LEN);
	if (!self->head_buf) {
		errno = ENOMEM;
		ret = -1;
		goto error_out;
	}

	self->body_buf = buffer_new(MAX_HEAD_LEN);
	if (!self->body_buf) {
		errno = ENOMEM;
		ret = -1;
		goto error_out;
	}

	fix_message_unparse(self);

	buffer_to_iovec(self->head_buf, &iov[0]);
	buffer_to_iovec(self->body_buf, &iov[1]);

	if (xwritev(sockfd, iov, ARRAY_SIZE(iov)) < 0) {
		ret = -1;
		goto error_out;
	}

error_out:
	buffer_delete(self->head_buf);
	buffer_delete(self->body_buf);
	self->head_buf = self->body_buf = NULL;

	return ret;
}

struct fix_message *fix_message_recv(int sockfd, int flags)
{
	struct buffer *buf;
	ssize_t nr;

	buf = buffer_new(MAX_MESSAGE_SIZE);
	if (!buf)
		return NULL;

	nr = buffer_read(buf, sockfd);
	if (nr <= 0)
		goto out_error;

	return fix_message_parse(buf);

out_error:
	buffer_delete(buf);

	return NULL;
}
