#include "libtrading/proto/nyse_taq_message.h"

#include "libtrading/buffer.h"

void *nyse_taq_msg_decode(struct buffer *buf, size_t size)
{
	void *msg;

	if (buffer_size(buf) < size)
		return NULL;

	msg = buffer_start(buf);

	buffer_advance(buf, size);

	return msg;
}
