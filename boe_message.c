#include "trading/boe_message.h"

#include "trading/buffer.h"

#include <string.h>

#define BOE_MAGIC_LEN		sizeof(uint16_t)
#define BOE_MSG_LENGTH_LEN	sizeof(uint16_t)

int boe_message_decode(struct buffer *buf, struct boe_message *msg, size_t size)
{
	uint16_t magic, len;
	void *start;
	size_t count;

	start = buffer_start(buf);

	magic = buffer_get_le16(buf);
	if (magic != BOE_MAGIC)
		return -1;

	len = buffer_get_le16(buf);

	count = BOE_MAGIC_LEN + len;

	if (count > size)
		count = size;

	memcpy(msg, start, count);

	buffer_advance(buf, len - BOE_MSG_LENGTH_LEN);

	return 0;
}
