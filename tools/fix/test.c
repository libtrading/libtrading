#include <libtrading/proto/fix_message.h>

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "test.h"

struct fcontainer *fcontainer_new(void)
{
	int i;
	struct fcontainer *container;

	container = calloc(1, sizeof *container);
	if (!container)
		goto free;

	for (i = 0; i < FIX_MAX_ELEMENTS_NUMBER; i++) {
		container->felems[i].msg.fields = calloc(FIX_MAX_FIELD_NUMBER, sizeof(struct fix_field));
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

	for (i = 0; i < FIX_MAX_ELEMENTS_NUMBER; i++)
		free(container->felems[i].msg.fields);

	free(container);
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
	if (self->nr < FIX_MAX_ELEMENTS_NUMBER)
		return &self->felems[self->nr++];
	else
		return NULL;
}

int init_elem(struct felem *elem, char *line)
{
	char *start;
	char *end;
	int tag;

	if (!elem)
		return 1;

	start = strcpy(elem->buf, line);

	while (1) {
		if (*start == 0x00 || *start == 0x0A)
			break;

		tag = strtol(start, &end, 10);
		end++;

		switch (tag) {
			case EncryptMethod:
			case HeartBtInt:
			case BeginSeqNo:
			case NewSeqNo:
			case EndSeqNo:
				elem->msg.fields[elem->msg.nr_fields++] = FIX_INT_FIELD(tag, strtol(end, NULL, 10));
				break;
			case ResetSeqNumFlag:
			case GapFillFlag:
			case SendingTime:
			case TestReqID:
			case Text:
				elem->msg.fields[elem->msg.nr_fields++] = FIX_STRING_FIELD(tag, end);
				break;
			case MsgSeqNum:
				elem->msg.msg_seq_num = strtol(end, NULL, 10);
				break;
			case BodyLength:
				elem->msg.body_length = strtol(end, NULL, 10);
				break;
			case SenderCompID:
				elem->msg.sender_comp_id = end;
				break;
			case TargetCompID:
				elem->msg.target_comp_id = end;
				break;
			case BeginString:
				elem->msg.begin_string = end;
				break;
			case CheckSum:
				elem->msg.check_sum = end;
				break;
			case MsgType:
				elem->msg.type = fix_msg_type_parse(end, 0x01);
				elem->msg.msg_type = end;
				break;
			default: /* Unsupported Tag */
				return 1;
		}

		while (*end != 0x00 && *end != 0x01 && *end != 0x0A)
			end++;

		if (*end == 0x00 || *end == 0x0A) {
			*end = 0x00;
			break;
		}

		*end = 0x00;

		start = end + 1;
	}

	return 0;
}

int script_read(FILE *stream, struct fcontainer *server, struct fcontainer *client)
{
	char line[FIX_MAX_LINE_LENGTH];
	int size = FIX_MAX_LINE_LENGTH;

	line[size - 1] = 0;

	while (fgets(line, size, stream)) {
		/* Line is too long */
		if (line[size - 1])
			return 1;

		switch (line[0]) {
			case 'c':
			case 'C':
				if (init_elem(add_elem(client), line + 1))
					return 1;
				break;
			case 's':
			case 'S':
				if (init_elem(add_elem(server), line + 1))
					return 1;
				break;
			default:
				break;
		}
	}

	return 0;
}

static int fstrcmp(const char *expected, const char *actual)
{
	const char *delim;

	if (!expected)
		return 0;

	delim = actual + strlen(expected);
	if (*delim != 0x01)
		return 1;

	return strncmp(expected, actual, strlen(expected)) != 0;
}

static int fstrcasecmp(const char *expected, const char *actual)
{
	const char *delim;

	if (!expected)
		return 0;

	delim = actual + strlen(expected);
	if (*delim != 0x01)
		return 1;

	return strncasecmp(expected, actual, strlen(expected)) != 0;
}

int fmsgcmp(struct fix_message *expected, struct fix_message *actual)
{
	int i;
	int ret = 1;
	struct fix_field *actual_field;
	struct fix_field *expected_field;

	if (expected->body_length && expected->body_length != actual->body_length)
		goto exit;

	if (expected->msg_seq_num && expected->msg_seq_num != actual->msg_seq_num)
		goto exit;

	if (fstrcmp(expected->sender_comp_id, actual->sender_comp_id))
		goto exit;

	if (fstrcmp(expected->target_comp_id, actual->target_comp_id))
		goto exit;

	if (fstrcmp(expected->begin_string, actual->begin_string))
		goto exit;

	if (fstrcmp(expected->msg_type, actual->msg_type))
		goto exit;

	for (i = 0; i < expected->nr_fields; i++) {
		expected_field = &expected->fields[i];
		actual_field = fix_get_field(actual, expected_field->tag);

		if (!actual_field)
			goto exit;

		switch (expected_field->type) {
			case FIX_TYPE_STRING:
				if (fstrcmp(expected_field->string_value, actual_field->string_value))
					goto exit;
				break;
			case FIX_TYPE_FLOAT:
				if (expected_field->float_value != actual_field->float_value)
					goto exit;
				break;
			case FIX_TYPE_CHAR:
				if (fstrcasecmp(&expected_field->char_value, &actual_field->char_value))
					goto exit;
				break;
			case FIX_TYPE_CHECKSUM:
			case FIX_TYPE_INT:
				if (expected_field->int_value != actual_field->int_value)
					goto exit;
				break;
			default:
				break;
		}
	}

	return 0;

exit:
	return ret;
}

static int fstrncpy(char *dest, const char *src, int n)
{
	int i;

	/* If n is negative nothing is copied */
	for (i = 0; i < n && src[i] != 0x00 && src[i] != 0x01; i++)
		dest[i] = src[i];

	return i;
}

void fprintmsg(FILE *stream, struct fix_message *msg)
{
	char buf[FIX_MAX_LINE_LENGTH];
	struct fix_field *field;
	int size = sizeof buf;
	char delim = '|';
	int len = 0;
	int i;

	if (!msg)
		return;

	if (msg->begin_string && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=", delim, BeginString);
		len += fstrncpy(buf + len, msg->begin_string, size - len);
	}

	if (msg->body_length && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=%lu", delim, BodyLength, msg->body_length);
	}

	if (msg->msg_type && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=", delim, MsgType);
		len += fstrncpy(buf + len, msg->msg_type, size - len);
	}

	if (msg->sender_comp_id && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=", delim, SenderCompID);
		len += fstrncpy(buf + len, msg->sender_comp_id, size - len);
	}

	if (msg->target_comp_id && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=", delim, TargetCompID);
		len += fstrncpy(buf + len, msg->target_comp_id, size - len);
	}

	if (msg->msg_seq_num && len < size) {
		len += snprintf(buf + len, size - len, "%c%d=%lu", delim, MsgSeqNum, msg->msg_seq_num);
	}

	for (i = 0; i < msg->nr_fields && len < size; i++) {
		field = msg->fields + i;

		switch (field->type) {
			case FIX_TYPE_STRING:
				len += snprintf(buf + len, size - len, "%c%d=", delim, field->tag);
				len += fstrncpy(buf + len, field->string_value, size - len);
				break;
			case FIX_TYPE_FLOAT:
				len += snprintf(buf + len, size - len, "%c%d=%f", delim, field->tag, field->float_value);
				break;
			case FIX_TYPE_CHAR:
				len += snprintf(buf + len, size - len, "%c%d=%c", delim, field->tag, field->char_value);
				break;
			case FIX_TYPE_CHECKSUM:
			case FIX_TYPE_INT:
				len += snprintf(buf + len, size - len, "%c%d=%" PRId64, delim, field->tag, field->int_value);
				break;
			default:
				break;
		}
	}

	if (len < size)
		buf[len++] = '\0';

	fprintf(stream, "%s%c\n", buf, delim);
}

void fprintmsg_iov(FILE *stream, struct fix_message *msg)
{
	char delim = '|';
	int i;

	if (!msg)
		return;

	for (i = 0; i < 2; ++i) {
		const char *start = msg->iov[i].iov_base;
		unsigned int len = msg->iov[i].iov_len;
		const char *end;

		while ((end = memchr(start, 0x01, len))) {
			fprintf(stdout, "%c%.*s", delim, (int)(end - start), start);
			len -= (end - start + 1);
			start = end + 1;
		}
	}

	fprintf(stdout, "%c\n", delim);

	return;
}
