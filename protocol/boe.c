#include "trading/boe_message.h"

#include "fix/buffer.h"

#include <stdlib.h>
#include <string.h>

#define BOE_MAGIC_LEN		sizeof(uint16_t)
#define BOE_MSG_LENGTH_LEN	sizeof(uint16_t)

struct boe_message *boe_decode_message(struct buffer *buf)
{
	struct boe_message *msg;
	uint16_t magic, len;
	void *start;
	size_t size;

	start = buffer_start(buf);

	magic = buffer_get_le16(buf);
	if (magic != BOE_MAGIC)
		return NULL;

	len = buffer_get_le16(buf);

	size = BOE_MAGIC_LEN + len;

	msg = malloc(size);
	if (!msg)
		return NULL;

	memcpy(msg, start, size);

	buffer_advance(buf, len - BOE_MSG_LENGTH_LEN);

	return msg;
}
