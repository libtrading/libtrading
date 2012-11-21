#include "libtrading/proto/fast_session.h"

#include <stdlib.h>

struct fast_session *fast_session_new(int sockfd)
{
	struct fast_session *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

	self->rx_buffer		= buffer_new(FAST_RECV_BUFFER_SIZE);
	if (!self->rx_buffer) {
		fast_session_free(self);
		return NULL;
	}

	self->rx_messages	= fast_message_new(FAST_TEMPLATE_MAX_NUMBER);
	if (!self->rx_messages) {
		fast_session_free(self);
		return NULL;
	}

	self->sockfd		= sockfd;
	self->last_tid		= 0;
	self->nr_messages	= 0;

	return self;
}

void fast_session_free(struct fast_session *self)
{
	if (!self)
		return;

	fast_message_free(self->rx_messages, FAST_TEMPLATE_MAX_NUMBER);
	buffer_delete(self->rx_buffer);
	free(self);
}

bool fast_session_message_add(struct fast_session *self, struct fast_message *msg)
{
	if (self->nr_messages >= FAST_TEMPLATE_MAX_NUMBER)
		return false;

	if (!fast_message_copy(self->rx_messages + self->nr_messages, msg))
		return false;

	self->nr_messages++;

	return true;
}

static inline bool fast_session_buffer_full(struct fast_session *session)
{
	return buffer_remaining(session->rx_buffer) <= FAST_MESSAGE_MAX_SIZE;
}

struct fast_message *fast_session_recv(struct fast_session *self, int flags)
{
	struct fast_message *msgs = self->rx_messages;
	struct buffer *buffer = self->rx_buffer;
	u64 last_tid = self->last_tid;
	struct fast_message *msg;
	const char *start_prev;
	size_t size;
	ssize_t nr;
	long shift;

	start_prev = buffer_start(buffer);

	msg = fast_message_decode(msgs, buffer, last_tid);
	if (msg)
		return msg;

	shift = start_prev - buffer_start(buffer);

	buffer_advance(buffer, shift);

	if (fast_session_buffer_full(self))
		buffer_compact(buffer);

	size = buffer_remaining(buffer);
	if (size > FAST_MESSAGE_MAX_SIZE) {
		size -= FAST_MESSAGE_MAX_SIZE;

		nr = buffer_nread(buffer, self->sockfd, size);
		if (nr < 0)
			return NULL;
	}

	if (!buffer_size(buffer))
		return NULL;

	return fast_message_decode(msgs, buffer, last_tid);
}
