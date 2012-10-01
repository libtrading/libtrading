#include "libtrading/proto/xdp_message.h"

#include "libtrading/buffer.h"

#include <stdlib.h>
#include <string.h>

int xdp_message_decode(struct buffer *buf, struct xdp_message *msg, size_t size)
{
	size_t available;
	u16 msg_size;

	available = buffer_size(buf);
	if (!available)
		return -1;

	msg_size = buffer_peek_le16(buf);
	if (msg_size > size)
		return -1;

	memcpy(msg, buffer_start(buf), msg_size);

	buffer_advance(buf, msg_size);

	return 0;
}
