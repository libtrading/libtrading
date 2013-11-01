#include "libtrading/proto/fast_session.h"

#include <stdlib.h>

struct fast_session *fast_session_new(struct fast_session_cfg *cfg)
{
	struct fast_session *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

	self->rx_buffer		= buffer_new(FAST_RECV_BUFFER_SIZE);
	if (!self->rx_buffer) {
		fast_session_free(self);
		return NULL;
	}

	self->tx_message_buffer		= buffer_new(FAST_TX_BUFFER_SIZE);
	if (!self->tx_message_buffer) {
		fast_session_free(self);
		return NULL;
	}

	self->tx_pmap_buffer		= buffer_new(FAST_TX_BUFFER_SIZE);
	if (!self->tx_pmap_buffer) {
		fast_session_free(self);
		return NULL;
	}

	self->rx_messages	= fast_message_new(FAST_TEMPLATE_MAX_NUMBER);
	if (!self->rx_messages) {
		fast_session_free(self);
		return NULL;
	}

	if (cfg->preamble_bytes > FAST_PREAMBLE_MAX_BYTES) {
		fast_session_free(self);
		return NULL;
	} else
		self->preamble.nr_bytes = cfg->preamble_bytes;

	self->sockfd		= cfg->sockfd;
	self->reset		= cfg->reset;
	self->rx_message	= NULL;
	self->last_tid		= 0;
	self->nr_messages	= 0;

	return self;
}

void fast_session_free(struct fast_session *self)
{
	if (!self)
		return;

	fast_message_free(self->rx_messages, FAST_TEMPLATE_MAX_NUMBER);
	buffer_delete(self->tx_message_buffer);
	buffer_delete(self->tx_pmap_buffer);
	buffer_delete(self->rx_buffer);
	free(self);
}

struct fast_message *fast_session_recv(struct fast_session *self, int flags)
{
	struct buffer *buffer = self->rx_buffer;
	struct fast_message *msg;
	size_t size;
	ssize_t nr;

	msg = fast_message_decode(self);
	if (msg)
		return msg;

	size = buffer_remaining(buffer);
	if (size <= FAST_MESSAGE_MAX_SIZE)
		buffer_compact(buffer);

	/*
	* If buffer's capacity is at least
	* 2 times FAST_MESSAGE_MAX_SIZE then,
	* remaining > FAST_MESSAGE_MAX_SIZE
	*/
	nr = buffer_recv(buffer, self->sockfd, FAST_MESSAGE_MAX_SIZE);
	if (nr <= 0)
		return NULL;

	return fast_message_decode(self);
}

int fast_session_send(struct fast_session *self, struct fast_message *msg, int flags)
{
	msg->pmap_buf = self->tx_pmap_buffer;
	buffer_reset(msg->pmap_buf);
	msg->msg_buf = self->tx_message_buffer;
	buffer_reset(msg->msg_buf);

	return fast_message_send(msg, self->sockfd, flags);
}

void fast_session_reset(struct fast_session *self)
{
	int i;

	for (i = 0; i < self->nr_messages; i++)
		fast_message_reset(self->rx_messages + i);
}
