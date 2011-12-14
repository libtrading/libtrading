#include "trading/soupbin_session.h"

#include "trading/buffer.h"

#include <stdlib.h>
#include <string.h>

#define SOUPBIN_RX_BUFFER_SIZE		4096

struct soupbin_session *soupbin_session_new(int sockfd)
{
	struct soupbin_session *self = calloc(1, sizeof *self);

	if (!self)
		return NULL;

	self->rx_buffer = buffer_new(SOUPBIN_RX_BUFFER_SIZE);
	if (!self->rx_buffer) {
		free(self->rx_buffer);
		return NULL;
	}

	self->sockfd = sockfd;

	return self;
}

void soupbin_session_delete(struct soupbin_session *session)
{
	buffer_delete(session->rx_buffer);
	free(session);
}

static struct soupbin_packet *soupbin_packet_decode(struct buffer *buf, uint16_t len)
{
	struct soupbin_packet *packet;
	size_t size;
	void *start;

	start = buffer_start(buf);

	size = sizeof(uint16_t) + len;

	packet = malloc(size);
	if (!packet)
		return NULL;

	memcpy(packet, start - sizeof(uint16_t), size);

	buffer_advance(buf, len);

	return packet;
}

struct soupbin_packet *soupbin_session_recv(struct soupbin_session *session)
{
	uint16_t len;
	ssize_t nr;

	if (buffer_size(session->rx_buffer) > 0)
		goto decode;

	buffer_reset(session->rx_buffer);

	nr = buffer_read(session->rx_buffer, session->sockfd);
	if (nr <= 0)
		return NULL;

decode:
	len = buffer_get_be16(session->rx_buffer);

	if (buffer_remaining(session->rx_buffer) < len)
		assert(0);

	return soupbin_packet_decode(session->rx_buffer, len);
}
