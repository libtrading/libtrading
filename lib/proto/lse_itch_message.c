#include "libtrading/proto/lse_itch_message.h"

#include "libtrading/buffer.h"

#include <stdlib.h>
#include <string.h>

int lse_itch_message_decode(struct buffer *buf, struct lse_itch_message *msg)
{
	size_t available;
	size_t size;

	available = buffer_size(buf);

	if (!available)
		return -1;

	size = buffer_peek_8(buf);

	if (available < size)
		return -1;

	memcpy(msg, buffer_start(buf), size);

	buffer_advance(buf, size);

	return 0;
}
