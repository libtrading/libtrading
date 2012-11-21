#include "libtrading/proto/fast_message.h"

#include "libtrading/buffer.h"

#include <string.h>
#include <stdlib.h>

static int parse_uint(struct buffer *buffer, u64 *value)
{
	const int bytes = 9;
	u64 result = 0;
	int i;
	u8 c;

	for (i = 0; i < bytes; i++) {
		if (!buffer_size(buffer))
			goto partial;

		c = buffer_first_char(buffer);
		buffer_advance(buffer, 1);

		if (c & 0x80) {
			result = (result << 7) | (c & 0x7F);
			*value = result;

			return 0;
		}

		result = (result << 7) | c;
	}

	return FAST_MSG_STATE_GARBLED;

partial:
	return FAST_MSG_STATE_PARTIAL;
}

static int parse_int(struct buffer *buffer, i64 *value)
{
	const int bytes = 9;
	i64 result = 0;
	int i;
	u8 c;

	if (!buffer_size(buffer))
		goto partial;

	if (buffer_first_char(buffer) & 0x40)
		result = -1;

	for (i = 0; i < bytes; i++) {
		if (!buffer_size(buffer))
			goto partial;

		c = buffer_first_char(buffer);
		buffer_advance(buffer, 1);

		if (c & 0x80) {
			result = (result << 7) | (c & 0x7F);
			*value = result;

			return 0;
		}

		result = (result << 7) | c;
	}

	return FAST_MSG_STATE_GARBLED;

partial:
	return FAST_MSG_STATE_PARTIAL;
}

/*
 * If ASCII string is nullable, an additional zero-preamble
 * is allowed at the start of the string. Hence the function
 * returns the number of bytes read. Negative return value
 * indicates an error. If the return value is equal to 1 and
 * the string is nullable, it means that the string is NULL.
 */
static int parse_string(struct buffer *buffer, char *value)
{
	int len = 0;
	u8 c;

	while (len < FAST_STRING_MAX_BYTES - 1) {
		if (!buffer_size(buffer))
			goto partial;

		c = buffer_first_char(buffer);
		buffer_advance(buffer, 1);

		if (c & 0x80) {
			value[len++] = c & 0x7F;
			value[len] = '\0';

			return len;
		} else
			value[len++] = c;
	}

	return FAST_MSG_STATE_GARBLED;

partial:
	return FAST_MSG_STATE_PARTIAL;
}

static int parse_pmap(struct buffer *buffer, struct fast_pmap *pmap)
{
	char c;

	pmap->nr_bytes = 0;

	while (pmap->nr_bytes < FAST_PMAP_MAX_BYTES) {
		if (!buffer_size(buffer))
			goto partial;

		c = buffer_first_char(buffer);
		buffer_advance(buffer, 1);

		pmap->bytes[pmap->nr_bytes++] = c;

		if (c & 0x80)
			return 0;
	}

	return FAST_MSG_STATE_GARBLED;

partial:
	return FAST_MSG_STATE_PARTIAL;
}

static int fast_decode_uint(struct buffer *buffer, struct fast_pmap *pmap, struct fast_field *field)
{
	int ret = 0;
	u64 tmp;

	switch (field->op) {
	case FAST_OP_NONE:
		ret = parse_uint(buffer, &field->uint_value);

		if (ret)
			goto fail;

		field->is_none = false;

		if (field_is_mandatory(field))
			break;

		if (!field->uint_value)
			field->is_none = true;
		else
			field->uint_value--;

		break;
	case FAST_OP_COPY:
		if (!pmap_is_set(pmap, field->pmap_bit)) {
			if (field_is_mandatory(field) &&
						field_is_none(field)) {
					ret = FAST_MSG_STATE_GARBLED;
					goto fail;
			}
		} else {
			ret = parse_uint(buffer, &field->uint_value);

			if (ret)
				goto fail;

			field->is_none = false;

			if (field_is_mandatory(field))
				break;

			if (!field->uint_value)
				field->is_none = true;
			else
				field->uint_value--;
		}

		break;
	case FAST_OP_INCR:
		if (!pmap_is_set(pmap, field->pmap_bit)) {
			if (field_is_mandatory(field) &&
						field_is_none(field)) {
					ret = FAST_MSG_STATE_GARBLED;
					goto fail;
			}

			field->uint_value++;
		} else {
			ret = parse_uint(buffer, &field->uint_value);

			if (ret)
				goto fail;

			field->is_none = false;

			if (field_is_mandatory(field))
				break;

			if (!field->uint_value)
				field->is_none = true;
			else
				field->uint_value--;
		}

		break;
	case FAST_OP_DELTA:
		ret = parse_uint(buffer, &tmp);

		if (ret)
			goto fail;

		field->is_none = false;

		if (field_is_mandatory(field)) {
			field->uint_value += tmp;
			break;
		}

		if (!tmp)
			field->is_none = true;
		else {
			tmp--;
			field->uint_value += tmp;
		}

		break;
	case FAST_OP_CONSTANT:
		field->is_none = false;

		if (field_is_mandatory(field))
			break;

		if (!pmap_is_set(pmap, field->pmap_bit))
			field->is_none = true;

		break;
	default:
		return FAST_MSG_STATE_GARBLED;
	};

	return 0;

fail:
	return ret;
}

static int fast_decode_int(struct buffer *buffer, struct fast_pmap *pmap, struct fast_field *field)
{
	int ret = 0;
	i64 tmp;

	switch (field->op) {
	case FAST_OP_NONE:
		ret = parse_int(buffer, &field->int_value);

		if (ret)
			goto fail;

		field->is_none = false;

		if (field_is_mandatory(field))
			break;

		if (!field->int_value)
			field->is_none = true;
		else if (field->int_value > 0)
			field->int_value--;

		break;
	case FAST_OP_COPY:
		if (!pmap_is_set(pmap, field->pmap_bit)) {
			if (field_is_mandatory(field) &&
						field_is_none(field)) {
					ret = FAST_MSG_STATE_GARBLED;
					goto fail;
			}
		} else {
			ret = parse_int(buffer, &field->int_value);

			if (ret)
				goto fail;

			field->is_none = false;

			if (field_is_mandatory(field))
				break;

			if (!field->int_value)
				field->is_none = true;
			else if (field->int_value > 0)
				field->int_value--;
		}

		break;
	case FAST_OP_INCR:
		if (!pmap_is_set(pmap, field->pmap_bit)) {
			if (field_is_mandatory(field) &&
						field_is_none(field)) {
					ret = FAST_MSG_STATE_GARBLED;
					goto fail;
			}

			field->int_value++;
		} else {
			ret = parse_int(buffer, &field->int_value);

			if (ret)
				goto fail;

			field->is_none = false;

			if (field_is_mandatory(field))
				break;

			if (!field->int_value)
				field->is_none = true;
			else if (field->int_value > 0)
				field->int_value--;
		}

		break;
	case FAST_OP_DELTA:
		ret = parse_int(buffer, &tmp);

		if (ret)
			goto fail;

		field->is_none = false;

		if (field_is_mandatory(field)) {
			field->int_value += tmp;
			break;
		}

		if (!tmp)
			field->is_none = true;
		else if (tmp > 0) {
			tmp--;
			field->int_value += tmp;
		} else
			field->int_value += tmp;

		break;
	case FAST_OP_CONSTANT:
		field->is_none = false;

		if (field_is_mandatory(field))
			break;

		if (!pmap_is_set(pmap, field->pmap_bit))
			field->is_none = true;

		break;
	default:
		return FAST_MSG_STATE_GARBLED;
	}

	return 0;

fail:
	return ret;
}

static int fast_decode_string(struct buffer *buffer, struct fast_pmap *pmap, struct fast_field *field)
{
	int ret;

	switch (field->op) {
	case FAST_OP_NONE:
		ret = parse_string(buffer, field->string_value);

		if (ret < 0)
			goto fail;

		field->is_none = false;

		if (field_is_mandatory(field))
			break;

		if (ret == 1)
			field->is_none = true;

		break;
	case FAST_OP_COPY:
		if (!pmap_is_set(pmap, field->pmap_bit)) {
			if (field_is_mandatory(field) &&
						field_is_none(field)) {
					ret = FAST_MSG_STATE_GARBLED;
					goto fail;
			}
		} else {
			ret = parse_string(buffer, field->string_value);

			if (ret < 0)
				goto fail;

			field->is_none = false;

			if (field_is_mandatory(field))
				break;

			if (ret == 1)
				field->is_none = true;
		}

		break;
	case FAST_OP_INCR:
			ret = FAST_MSG_STATE_GARBLED;
			goto fail;
	case FAST_OP_DELTA:
			ret = FAST_MSG_STATE_GARBLED;
			goto fail;
	case FAST_OP_CONSTANT:
		field->is_none = false;

		if (field_is_mandatory(field))
			break;

		if (!pmap_is_set(pmap, field->pmap_bit))
			field->is_none = true;

		break;
	default:
		return FAST_MSG_STATE_GARBLED;
	}

	return 0;

fail:
	return ret;
}

static inline struct fast_message *fast_get_msg(struct fast_message *msgs, int tid)
{
	int i;

	for (i = 0; i < FAST_TEMPLATE_MAX_NUMBER; i++) {
		if (msgs[i].tid == tid)
			return &msgs[i];
	}

	return NULL;
}

struct fast_message *fast_message_decode(struct fast_message *msgs, struct buffer *buffer, u64 last_tid)
{
	int ret = FAST_MSG_STATE_PARTIAL;
	struct fast_message *msg;
	struct fast_field *field;
	struct fast_pmap pmap;

	unsigned long size;
	const char *start;
	unsigned long i;

	u64 tid;

retry:
	start	= buffer_start(buffer);
	size	= buffer_size(buffer);

	if (!size)
		goto fail;

	ret = parse_pmap(buffer, &pmap);
	if (ret)
		goto fail;

	if (pmap_is_set(&pmap, 0)) {
		ret = parse_uint(buffer, &tid);

		if (ret)
			goto fail;
	} else
		tid = last_tid;

	msg = fast_get_msg(msgs, tid);

	if (!msg) {
		ret = FAST_MSG_STATE_GARBLED;
		goto fail;
	}

	msg->pmap = &pmap;

	for (i = 0; i < msg->nr_fields; i++) {
		field = msg->fields + i;

		switch (field->type) {
		case FAST_TYPE_INT:
			ret = fast_decode_int(buffer, msg->pmap, field);
			if (ret)
				goto fail;
			break;
		case FAST_TYPE_UINT:
			ret = fast_decode_uint(buffer, msg->pmap, field);
			if (ret)
				goto fail;
			break;
		case FAST_TYPE_STRING:
			ret = fast_decode_string(buffer, msg->pmap, field);
			if (ret)
				goto fail;
			break;
		default:
			ret = FAST_MSG_STATE_GARBLED;
			goto fail;
		}
	}

	return msg;

fail:
	/* Should we stop FAST decoding? */
	if (ret != FAST_MSG_STATE_PARTIAL)
		goto retry;

	buffer_advance(buffer, start - buffer_start(buffer));

	return NULL;
}

struct fast_message *fast_message_new(int nr_messages)
{
	struct fast_message *self = calloc(nr_messages, sizeof *self);

	if (!self)
		return NULL;

	return self;
}

void fast_message_free(struct fast_message *self, int nr_messages)
{
	int i;

	if (!self)
		return;

	for (i = 0; i < nr_messages; i++)
		free(self[i].fields);

	free(self);

	return;
}

bool fast_message_copy(struct fast_message *dest, struct fast_message *src)
{
	dest->nr_fields = src->nr_fields;
	dest->tid = src->tid;

	dest->fields = calloc(src->nr_fields, sizeof(struct fast_field));
	if (!dest->fields)
		return false;

	/* Copying is safe until there are pointers in the struct fast_field */
	memcpy(dest->fields, src->fields, src->nr_fields * sizeof(struct fast_field));

	return true;
}
