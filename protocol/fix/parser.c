#include "trading/fix_message.h"

#include "fix/buffer.h"

#include <stdlib.h>
#include <stddef.h>

static int parse_tag(struct buffer *self)
{
	const char *delim;
	const char *start;
	char *end;
	int ret;

	start = buffer_start(self);
	delim = buffer_find(self, '=');

	if (!delim)
		return 0;

	if (*delim != '=')
		return 0;

	ret = strtol(start, &end, 10);
	if (end != delim)
		return 0;

	buffer_advance(self, 1);

	return ret;
}

static const char *parse_value(struct buffer *self)
{
	char *start, *end;

	start = buffer_start(self);

	end = buffer_find(self, 0x01);

	if (!end)
		return NULL;

	if (*end != 0x01)
		return NULL;

	*end = '\0';

	buffer_advance(self, 1);

	return start;
}

static const char *parse_field(struct buffer *self, int tag)
{
	if (parse_tag(self) != tag)
		return NULL;

	return parse_value(self);
}

static void rest_of_message(struct fix_message *self, struct buffer *buffer)
{
}

static bool checksum(struct fix_message *self, struct buffer *buffer)
{
	// if BodyLength is invalid (e.g. does not point to CheckSum) -> garbled
	// if CheckSum does not match or invalid or empty -> garbled

	return true;
}

static bool msg_type(struct fix_message *self)
{
	self->msg_type = parse_field(self->head_buf, MsgType);

	// if third field is not MsgType -> garbled

	return self->msg_type != NULL;
}

static bool body_length(struct fix_message *self)
{
	parse_field(self->head_buf, BodyLength);

	// if second field is not BodyLength -> garbled

	return true;
}

static bool begin_string(struct fix_message *self)
{
	self->begin_string = parse_field(self->head_buf, BeginString);

	// if first field is not BeginString -> garbled
	// if BeginString is invalid or empty -> garbled

	return self->begin_string != NULL;
}

static bool first_three_fields(struct fix_message *self)
{
	if (!begin_string(self))
		return false;

	if (!body_length(self))
		return false;

	return msg_type(self);
}

struct fix_message *fix_message_parse(struct buffer *buffer)
{
	struct fix_message *self;

	self		= fix_message_new();
	if (!self) {
		buffer_delete(buffer);
		return NULL;
	}

	self->head_buf = buffer;

	if (!first_three_fields(self))
		goto garbled;

	if (!checksum(self, buffer))
		goto garbled;

	rest_of_message(self, buffer);

	return self;

garbled:
	/* TODO: Make sure buffer is either empty or start of it points to a new BeginString */
	fix_message_free(self);

	return NULL;
}

void fix_message_validate(struct fix_message *self)
{
	// if MsgSeqNum is missing -> logout, terminate
}
