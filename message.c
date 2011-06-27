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
#define MAX_MESSAGE_SIZE	128

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

	if (self->buffer)
		buffer_delete(self->buffer);

	free(self);
}

bool fix_message_type_is(struct fix_message *self, const char *expected_type)
{
	if (!self->msg_type)
		return false;

	return strncmp(self->msg_type, expected_type, strlen(expected_type)) == 0;
}

static void fix_message_unparse(struct fix_message *self, struct buffer *buffer)
{
	struct fix_field sender_comp_id;
	struct fix_field target_comp_id;
	struct fix_field begin_string;
	struct fix_field msg_type;

	begin_string	= FIX_STRING_FIELD(BeginString, self->begin_string);
	msg_type	= FIX_STRING_FIELD(MsgType, self->msg_type);
	sender_comp_id	= FIX_STRING_FIELD(SenderCompID, self->sender_comp_id);
	target_comp_id	= FIX_STRING_FIELD(TargetCompID, self->target_comp_id);

	fix_field_unparse(&begin_string, buffer);
	/* XXX: BodyLength */
	fix_field_unparse(&msg_type, buffer);
	fix_field_unparse(&sender_comp_id, buffer);
	fix_field_unparse(&target_comp_id, buffer);
}

int fix_message_send(struct fix_message *self, int sockfd, int flags)
{
	struct buffer *buffer;

	buffer = buffer_new(MAX_MESSAGE_SIZE);
	if (!buffer) {
		errno = ENOMEM;
		return -1;
	}

	fix_message_unparse(self, buffer);
	buffer_write(buffer, sockfd);

	buffer_delete(buffer);

	return 0;
}

struct fix_message *fix_message_recv(int sockfd, int flags)
{
	struct fix_message *msg;
	struct buffer *buf;
	ssize_t nr;

	buf = buffer_new(MAX_MESSAGE_SIZE);
	if (!buf)
		return NULL;

	nr = buffer_read(buf, sockfd);
	if (nr <= 0)
		goto out_error;

	msg = fix_message_parse(buf);
	if (!msg)
		goto out_error;

	return msg;

out_error:
	buffer_delete(buf);

	return NULL;
}
