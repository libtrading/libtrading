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

void fcontainer_init(struct fcontainer *self, struct fast_message *init_msg)
{
	int i;
	unsigned long nr_fields;
	struct fast_message *msg;
	struct fast_field *fields;

	nr_fields = init_msg->nr_fields;
	fields = init_msg->fields;

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
		case FAST_TYPE_SEQUENCE:
			goto fail;
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

	line[size - 1] = 0;

	add_elem(self);
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
		case FAST_TYPE_SEQUENCE:
			break;
		default:
			break;
		}
	}

	ret = 0;

exit:
	return ret;
}

int snprintmsg(char *buf, size_t size, struct fast_message *msg)
{
	struct fast_field *field;
	char delim = '|';
	int len = 0;
	int i;

	if (!msg)
		goto exit;

	if (len < size)
		len += snprintf(buf + len, size - len, "%c", delim);

	for (i = 0; i < msg->nr_fields && len < size; i++) {
		field = msg->fields + i;

		if (field_state_empty(field)) {
			len += snprintf(buf + len, size - len, "none%c", delim);
			continue;
		}

		switch (field->type) {
		case FAST_TYPE_INT:
			len += snprintf(buf + len, size - len, "%" PRId64 "%c", field->int_value, delim);
			break;
		case FAST_TYPE_UINT:
			len += snprintf(buf + len, size - len, "%" PRIu64 "%c", field->uint_value, delim);
			break;
		case FAST_TYPE_STRING:
			len += snprintf(buf + len, size - len, "%s%c", field->string_value, delim);
			break;
		case FAST_TYPE_DECIMAL:
			len += snprintf(buf + len, size - len, "%" PRId64 "%c", field->decimal_value.exp, delim);
			len += snprintf(buf + len, size - len, "%" PRId64 "%c", field->decimal_value.mnt, delim);
			break;
		case FAST_TYPE_SEQUENCE:
			len += snprintseq(buf + len, size - len, field);
			break;
		default:
			break;
		}
	}

exit:
	return len;
}

int snprintseq(char *buf, size_t size, struct fast_field *field)
{
	struct fast_sequence *seq;
	struct fast_message *msg;
	int len = 0;
	int i;

	if (!field || field->type != FAST_TYPE_SEQUENCE)
		goto exit;

	if (len < size)
		len += snprintf(buf + len, size - len, "\n<sequence>\n");

	seq = field->ptr_value;

	for (i = 1; i <= seq->length.uint_value && len < size; i++) {
		msg = seq->elements + i;

		len += snprintmsg(buf + len, size - len, msg);
		len += snprintf(buf + len, size - len, "\n");
	}

	if (len < size)
		len += snprintf(buf + len, size - len, "</sequence>");

exit:
	return len;
}

void fprintmsg(FILE *stream, struct fast_message *msg)
{
	char buf[FAST_MAX_LINE_LENGTH];
	int size = sizeof(buf);
	int len = 0;

	len += snprintmsg(buf, size, msg);

	if (len < size)
		buf[len++] = '\0';

	fprintf(stream, "%s\n", buf);
}
