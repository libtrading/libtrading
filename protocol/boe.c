#include "fix/boe.h"

#include "fix/buffer.h"

#include <string.h>

int boe_decode_header(struct buffer *buf, struct boe_header *header)
{
	memcpy(header, buffer_start(buf), sizeof *header);

	buffer_advance(buf, sizeof *header);

	return 0;
}

struct boe_login_request *boe_decode_login_request(struct boe_header *header, struct buffer *buf)
{
	struct boe_login_request *login;
	size_t size;

	size = header->MessageLength - sizeof(header);

	login = malloc(size);
	if (!login)
		return NULL;

	memcpy(login, buffer_start(buf), size);

	buffer_advance(buf, size);

	return login;
}
