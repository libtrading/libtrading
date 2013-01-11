#include <libtrading/proto/fast_message.h>

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "test.h"

#define	DELIMS	".,;|\n"

struct fcontainer *fcontainer_new(void)
{
	int i;
	struct fcontainer *container;

	container = calloc(1, sizeof(struct fcontainer));
	if (!container)
		goto free;

	for (i = 0; i < FAST_MAX_ELEMENTS_NUMBER; i++) {
		container->felems[i].msg.fields = calloc(FAST_FIELD_MAX_NUMBER, sizeof(struct fast_field));
		if (!container->felems[i].msg.fields)
			goto free;
	}

	return container;

free:
	fcontainer_free(container);
	return NULL;
}

void fcontainer_free(struct fcontainer *container)
{
	int i;

	if (!container)
		return;

	for (i = 0; i < FAST_MAX_ELEMENTS_NUMBER; i++)
		free(container->felems[i].msg.fields);

	free(container);
}

void fcontainer_init(struct fcontainer *self, struct fast_field *fields, unsigned long nr_fields)
{
	int i;
	struct fast_message *msg;

	for (i = 0; i < FAST_MAX_ELEMENTS_NUMBER; i++) {
		msg = &self->felems[i].msg;
		msg->nr_fields = nr_fields;
		msg->tid = 0;

		memcpy(msg->fields, fields, nr_fields * sizeof(struct fast_field));
	}

	return;
}

struct felem *cur_elem(struct fcontainer *self)
{
	if (self->cur < self->nr)
		return &self->felems[self->cur];
	else
		return NULL;
}

struct felem *next_elem(struct fcontainer *self)
{
	self->cur++;
	return cur_elem(self);
}

struct felem *add_elem(struct fcontainer *self)
{
	if (self->nr < FAST_MAX_ELEMENTS_NUMBER)
		return &self->felems[self->nr++];
	else
		return NULL;
}

int init_elem(struct felem *elem, char *line)
{
	struct fast_field *field;
	unsigned long i;
	char *start;
	char *end;

	if (!elem)
		goto fail;

	start = strncpy(elem->buf, line, FAST_MAX_LINE_LENGTH);

	for (i = 0; i < elem->msg.nr_fields; i++) {
		field = elem->msg.fields + i;

		if (!strncmp(start, "none", 4)) {
			if (!field_is_mandatory(field)) {
				field_set_empty(field);
				start += 5;
				continue;
			} else
				goto fail;
		}

		switch (field->type) {
		case FAST_TYPE_INT:
			field->int_value = strtol(start, &end, 10);
			start = end + 1;
			break;
		case FAST_TYPE_UINT:
			field->uint_value = strtoul(start, &end, 10);
			start = end + 1;
			break;
		case FAST_TYPE_STRING:
			if (sscanf(start, "%[^" DELIMS  "]s", field->string_value) != 1)
				goto fail;

			start = start + strlen(field->string_value) + 1;
			break;
		case FAST_TYPE_DECIMAL:
			field->decimal_value.exp = strtol(start, &end, 10);
			start = end + 1;
			field->decimal_value.mnt = strtol(start, &end, 10);
			start = end + 1;
			break;
		default:
			goto fail;
		}
	}

	return 0;

fail:
	return 1;
}

int script_read(FILE *stream, struct fcontainer *self)
{
	char line[FAST_MAX_LINE_LENGTH];
	int size = FAST_MAX_LINE_LENGTH;
	char buf[FAST_MAX_LINE_LENGTH];
	struct fast_field *fields;
	struct fast_field *field;
	unsigned long nr_fields;
	unsigned long i;
	char *start;
	char *end;

	line[size - 1] = 0;

	if (!fgets(line, size, stream) || line[size - 1])
		goto fail;

	if (sscanf(line, "%ld", &nr_fields) != 1)
		goto fail;

	fields = calloc(nr_fields, sizeof(struct fast_field));
	if (!fields)
		goto fail;

	for (i = 0; i < nr_fields; i++) {
		field = fields + i;

		if (!fgets(line, size, stream) || line[size - 1])
			goto fail;

		start = line;

		if (sscanf(start, "%[^" DELIMS "]s", buf) != 1)
			goto fail;

		start += (strlen(buf) + 1);

		if (!strncmp(buf, "int", 3))
			field->type = FAST_TYPE_INT;
		else if (!strncmp(buf, "uint", 4))
			field->type = FAST_TYPE_UINT;
		else if (!strncmp(buf, "string", 6))
			field->type = FAST_TYPE_STRING;
		else if (!strncmp(buf, "decimal", 7))
			field->type = FAST_TYPE_DECIMAL;
		else
			goto fail;

		if (sscanf(start, "%[^" DELIMS "]s", buf) != 1)
			goto fail;

		start += (strlen(buf) + 1);

		if (!strncmp(buf, "optional", 8))
			field->presence = FAST_PRESENCE_OPTIONAL;
		else if (!strncmp(buf, "mandatory", 9))
			field->presence = FAST_PRESENCE_MANDATORY;
		else
			goto fail;

		if (sscanf(start, "%[^" DELIMS "]s", buf) != 1)
			goto fail;

		start += (strlen(buf) + 1);

		if (!strncmp(buf, "none", 4))
			field->op = FAST_OP_NONE;
		else if (!strncmp(buf, "copy", 4))
			field->op = FAST_OP_COPY;
		else if (!strncmp(buf, "incr", 4))
			field->op = FAST_OP_INCR;
		else if (!strncmp(buf, "delta", 5))
			field->op = FAST_OP_DELTA;
		else if (!strncmp(buf, "const", 5))
			field->op = FAST_OP_CONSTANT;
		else
			goto fail;

		field->pmap_bit = strtoul(start, &end, 10);
		start = end + 1;

		if (!strncmp(start, "none", 4)) {
			field->has_reset = false;
			continue;
		} else
			field->has_reset = true;

		switch (field->type) {
		case FAST_TYPE_INT:
			field->int_reset = strtol(start, NULL, 10);
			break;
		case FAST_TYPE_UINT:
			field->uint_reset = strtoul(start, NULL, 10);
			break;
		case FAST_TYPE_STRING:
			if (sscanf(start, "%[^" DELIMS "]s", field->string_reset) != 1)
				goto fail;
			break;
		case FAST_TYPE_DECIMAL:
			field->decimal_reset.exp = strtol(start, &end, 10);
			start = end + 1;
			field->decimal_reset.mnt = strtol(start, &end, 10);
			break;
		default:
			goto fail;
		}
	}

	fcontainer_init(self, fields, nr_fields);

	fast_message_init(&add_elem(self)->msg);

	free(fields);

	while (fgets(line, size, stream)) {
		if (line[size - 1])
			goto fail;

		if (init_elem(add_elem(self), line))
			goto fail;
	}

	return 0;

fail:
	return 1;
}

int fmsgcmp(struct fast_message *expected, struct fast_message *actual)
{
	struct fast_field *expected_field;
	struct fast_field *actual_field;
	int ret = 1;
	int i;

	if (expected->nr_fields != actual->nr_fields)
		goto exit;

	for (i = 0; i < expected->nr_fields; i++) {
		actual_field = actual->fields + i;
		expected_field = expected->fields + i;

		if (actual_field->presence != expected_field->presence)
			goto exit;

		if (actual_field->type != expected_field->type)
			goto exit;

		if (actual_field->op != expected_field->op)
			goto exit;

		if (field_state_empty(actual_field)) {
			if (field_state_empty(expected_field))
				continue;
			else
				goto exit;
		}

		switch (expected_field->type) {
		case FAST_TYPE_INT:
			if (actual_field->int_value != expected_field->int_value)
				goto exit;
			break;
		case FAST_TYPE_UINT:
			if (actual_field->uint_value != expected_field->uint_value)
				goto exit;
			break;
		case FAST_TYPE_STRING:
			if (strcmp(actual_field->string_value, expected_field->string_value))
				goto exit;
			break;
		case FAST_TYPE_DECIMAL:
			if (actual_field->decimal_value.exp != expected_field->decimal_value.exp)
				goto exit;

			if (actual_field->decimal_value.mnt != expected_field->decimal_value.mnt)
				goto exit;

			break;
		default:
			break;
		}
	}

	ret = 0;

exit:
	return ret;
}

void fprintmsg(FILE *stream, struct fast_message *msg)
{
	char buf[FAST_MAX_LINE_LENGTH];
	struct fast_field *field;
	int size = sizeof(buf);
	char delim = '|';
	int len = 0;
	int i;

	if (!msg)
		return;

	for (i = 0; i < msg->nr_fields && len < size; i++) {
		field = msg->fields + i;

		if (field_state_empty(field)) {
			len += snprintf(buf + len, size - len, "%cnone", delim);
			continue;
		}

		switch (field->type) {
		case FAST_TYPE_INT:
			len += snprintf(buf + len, size - len, "%c%" PRId64, delim, field->int_value);
			break;
		case FAST_TYPE_UINT:
			len += snprintf(buf + len, size - len, "%c%" PRIu64, delim, field->uint_value);
			break;
		case FAST_TYPE_STRING:
			len += snprintf(buf + len, size - len, "%c%s", delim, field->string_value);
			break;
		case FAST_TYPE_DECIMAL:
			len += snprintf(buf + len, size - len, "%c%" PRId64, delim, field->decimal_value.exp);
			len += snprintf(buf + len, size - len, "%c%" PRId64, delim, field->decimal_value.mnt);
			break;
		default:
			break;
		}
	}

	if (len < size)
		buf[len++] = '\0';

	fprintf(stream, "%s%c\n", buf, delim);
}
