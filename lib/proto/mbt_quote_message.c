#include "libtrading/proto/mbt_quote_message.h"

#include "libtrading/buffer.h"

#include <stdlib.h>
#include <string.h>

#define MBT_QUOTE_FIELD_DELIM		';'
#define MBT_QUOTE_HEADER_DELIM		'|'
#define MBT_QUOTE_MESSAGE_END		'\n'

void mbt_quote_message_delete(struct mbt_quote_message *msg)
{
	switch (msg->Type) {
	case MBT_QUOTE_LOGGING_ON: {
		struct mbt_quote_logging_on *logon = mbt_quote_message_payload(msg);

		free(logon->UserName);
		free(logon->Password);

		break;
	}
	default:
		break;
	}

	free(msg);
}

static char *mbt_quote_decode_field(struct buffer *buf)
{
	char *start, *end;

	if (!buffer_find(buf, '='))
		return NULL;

	buffer_advance(buf, 1);

	start = buffer_start(buf);

	for (;;) {
		char ch;

		ch = buffer_first_char(buf);
		if (ch == MBT_QUOTE_FIELD_DELIM || ch == MBT_QUOTE_MESSAGE_END)
			break;

		if (!buffer_remaining(buf))
			return NULL;

		buffer_advance(buf, 1);
	}

	end = buffer_start(buf);
	if (!end)
		return NULL;

	return strndup(start, end-start);
}

struct mbt_quote_message *mbt_quote_message_decode(struct buffer *buf)
{
	struct mbt_quote_message *msg;
	char type;

	type = buffer_get_char(buf);

	if (buffer_get_char(buf) != MBT_QUOTE_HEADER_DELIM)
		return NULL;

	switch (type) {
	case MBT_QUOTE_LOGGING_ON: {
		struct mbt_quote_logging_on *logon;

		msg = calloc(1, sizeof(*msg) + sizeof(*logon));
		if (!msg)
			return NULL;

		msg->Type = type;

		logon = mbt_quote_message_payload(msg);

		/* FIXME: Fields are in no particular order. */

		logon->UserName = mbt_quote_decode_field(buf);

		logon->Password = mbt_quote_decode_field(buf);

		break;
	}
	default:
		msg = NULL;
		break;
	}

	if (buffer_get_char(buf) != MBT_QUOTE_MESSAGE_END)
		goto error_free;

	return msg;

error_free:
	mbt_quote_message_delete(msg);

	return NULL;
}
