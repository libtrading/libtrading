#include "fix/message.h"

#include "fix/buffer.h"

#include <assert.h>
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
	cksum		= buffer_checksum(self->head_buf) + buffer_checksum(self->body_buf);
	check_sum	= FIX_CHECKSUM_FIELD(CheckSum, cksum);
	fix_field_unparse(&check_sum, self->body_buf);
}

int fix_message_send(struct fix_message *self, int sockfd, int flags)
{
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

	/* TODO: use writev() */
	buffer_write(self->head_buf, sockfd);
	buffer_write(self->body_buf, sockfd);

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
