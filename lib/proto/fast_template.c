#include "libtrading/proto/fast_message.h"
#include "libtrading/proto/fast_session.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <ctype.h>

static int fast_field_init(xmlNodePtr node, struct fast_field *field);

static int fast_presence_init(xmlNodePtr node, struct fast_field *field)
{
	xmlChar *prop;
	int ret = 0;

	if (node == NULL)
		return 1;

	prop = xmlGetProp(node, (const xmlChar *)"presence");

	if (prop == NULL)
		field->presence = FAST_PRESENCE_MANDATORY;
	else if (!xmlStrcmp(prop, (const xmlChar *)"mandatory"))
		field->presence = FAST_PRESENCE_MANDATORY;
	else if (!xmlStrcmp(prop, (const xmlChar *)"optional"))
		field->presence = FAST_PRESENCE_OPTIONAL;
	else
		ret = 1;

	xmlFree(prop);

	return ret;
}

static int fast_misc_init(xmlNodePtr node, struct fast_field *field)
{
	xmlChar *prop;

	if (node == NULL)
		return 1;

	prop = xmlGetProp(node, (const xmlChar *)"charset");

	if (prop != NULL && !xmlStrcmp(prop, (const xmlChar *)"unicode"))
		field_add_flags(field, FAST_FIELD_FLAGS_UNICODE);

	xmlFree(prop);

	prop = xmlGetProp(node, (const xmlChar *)"name");
	if (prop != NULL)
		strncpy(field->name, (const char *)prop, sizeof(field->name));

	xmlFree(prop);

	prop = xmlGetProp(node, (const xmlChar *)"id");
	if (prop != NULL)
		field->id = atoi((const char *)prop);

	xmlFree(prop);

	return 0;
}

static int fast_type_init(xmlNodePtr node, struct fast_field *field)
{
	int ret = 0;

	if (node == NULL)
		ret = 1;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"int32") ||
			!xmlStrcmp(node->name, (const xmlChar *)"int64"))
		field->type = FAST_TYPE_INT;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"uInt32") ||
			!xmlStrcmp(node->name, (const xmlChar *)"uInt64"))
		field->type = FAST_TYPE_UINT;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"length") ||
			!xmlStrcmp(node->name, (const xmlChar *)"Length"))
		field->type = FAST_TYPE_UINT;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"string") ||
			!xmlStrcmp(node->name, (const xmlChar *)"String"))
		field->type = FAST_TYPE_STRING;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"decimal") ||
			!xmlStrcmp(node->name, (const xmlChar *)"Decimal"))
		field->type = FAST_TYPE_DECIMAL;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"sequence") ||
			!xmlStrcmp(node->name, (const xmlChar *)"Sequence"))
		field->type = FAST_TYPE_SEQUENCE;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"exponent") ||
			!xmlStrcmp(node->name, (const xmlChar *)"Exponent"))
		field->type = FAST_TYPE_INT;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"mantissa") ||
			!xmlStrcmp(node->name, (const xmlChar *)"Mantissa"))
		field->type = FAST_TYPE_INT;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"bytevector") ||
			!xmlStrcmp(node->name, (const xmlChar *)"byteVector"))
		field->type = FAST_TYPE_VECTOR;
	else
		ret = 1;

	return ret;
}

static int fast_op_init(xmlNodePtr node, struct fast_field *field)
{
	int ret = 0;

	if (node == NULL)
		field->op = FAST_OP_NONE;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"copy"))
		field->op = FAST_OP_COPY;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"delta"))
		field->op = FAST_OP_DELTA;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"default"))
		field->op = FAST_OP_DEFAULT;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"constant"))
		field->op = FAST_OP_CONSTANT;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"increment"))
		field->op = FAST_OP_INCR;
	else
		ret = 1;

	return ret;
}

static int fast_vector_init(const char *in, char *out)
{
	int hex;

	for ( ; *in; in++) {
		if (isblank(*in))
			continue;

		if (sscanf(in++, "%02X", &hex) != 1)
			goto fail;

		*out++ = (char)hex;
	}

	return 0;

fail:
	return -1;
}

static int fast_reset_init(xmlNodePtr node, struct fast_field *field)
{
	int ret = 0;
	xmlChar *prop;

	field->has_reset = false;

	switch (field->type) {
	case FAST_TYPE_INT:
		field->int_value = 0;
		field->int_previous = 0;

		if (node == NULL)
			break;

		prop = xmlGetProp(node, (const xmlChar *)"value");

		if (prop == NULL)
			break;

		field->has_reset = true;
		field->int_reset = strtol((char *)prop, NULL, 10);
		field->int_value = field->int_reset;
		field->int_previous = field->int_reset;

		xmlFree(prop);
		break;
	case FAST_TYPE_UINT:
		field->uint_value = 0;
		field->uint_previous = 0;

		if (node == NULL)
			break;

		prop = xmlGetProp(node, (const xmlChar *)"value");

		if (prop == NULL)
			break;

		field->has_reset = true;
		field->uint_reset = strtoul((char *)prop, NULL, 10);
		field->uint_value = field->uint_reset;
		field->uint_previous = field->uint_reset;

		xmlFree(prop);
		break;
	case FAST_TYPE_STRING:
		field->string_value[0] = 0;
		field->string_previous[0] = 0;

		if (node == NULL)
			break;

		prop = xmlGetProp(node, (const xmlChar *)"value");

		if (prop == NULL)
			break;

		field->has_reset = true;
		strcpy(field->string_reset, (char *)prop);
		strcpy(field->string_value, (char *)prop);
		strcpy(field->string_previous, (char *)prop);

		xmlFree(prop);
		break;
	case FAST_TYPE_VECTOR:
		memset(field->vector_value, 0, sizeof(field->vector_value));
		memset(field->vector_previous, 0, sizeof(field->vector_previous));

		if (node == NULL)
			break;

		prop = xmlGetProp(node, (const xmlChar *)"value");

		if (prop == NULL)
			break;

		field->has_reset = true;

		ret = fast_vector_init((const char *)prop, field->vector_reset);
		if (ret)
			break;

		memcpy(field->vector_value, field->vector_reset,
						sizeof(field->vector_reset));
		memcpy(field->vector_previous, field->vector_reset,
						sizeof(field->vector_reset));

		break;
	case FAST_TYPE_DECIMAL:
		field->decimal_value.exp = 0;
		field->decimal_value.mnt = 0;
		field->decimal_previous.exp = 0;
		field->decimal_previous.mnt = 0;

		break;
	case FAST_TYPE_SEQUENCE:
		ret = 1;
		break;
	default:
		ret = 1;
		break;
	}

	return ret;
}

static int fast_sequence_init(xmlNodePtr node, struct fast_field *field)
{
	struct fast_sequence *seq;
	struct fast_message *msg;
	struct fast_field *orig;
	xmlNodePtr tmp;
	int i, ret = 1;
	int nr_fields;

	field->ptr_value = calloc(1, sizeof(struct fast_sequence));
	if (!field->ptr_value)
		goto exit;

	seq = field->ptr_value;

	nr_fields = xmlChildElementCount(node);

	node = node->xmlChildrenNode;
	while (node && node->type != XML_ELEMENT_NODE)
		node = node->next;

	if (node == NULL)
		goto exit;

	if (xmlStrcmp(node->name, (const xmlChar *)"length"))
		goto exit;

	if (fast_field_init(node, &seq->length))
		goto exit;

	if (!field_is_mandatory(field))
		seq->length.presence = FAST_PRESENCE_OPTIONAL;

	node = node->next;
	orig = field;

	for (i = 0; i < FAST_SEQUENCE_ELEMENTS; i++) {
		msg = seq->elements + i;
		tmp = node;

		msg->fields = calloc(nr_fields, sizeof(struct fast_field));
		if (!msg->fields)
			goto exit;

		msg->ghtab = g_hash_table_new(g_str_hash, g_str_equal);
		if (!msg->ghtab)
			goto exit;

		msg->nr_fields = 0;

		while (tmp != NULL) {
			if (tmp->type != XML_ELEMENT_NODE) {
				tmp = tmp->next;
				continue;
			}

			field = msg->fields + msg->nr_fields;

			if (fast_field_init(tmp, field))
				goto exit;

			if (strlen(field->name))
				g_hash_table_insert(msg->ghtab, field->name, field);

			if (pmap_required(field))
				field_add_flags(orig, FAST_FIELD_FLAGS_PMAPREQ);

			msg->nr_fields++;
			tmp = tmp->next;
		}
	}

	ret = 0;

exit:
	return ret;
}

static int fast_decimal_init_atomic(xmlNodePtr node, struct fast_field *field)
{
	int ret = 0;

	node = node->xmlChildrenNode;

	while (node && node->type != XML_ELEMENT_NODE)
		node = node->next;

	ret = fast_op_init(node, field);
	if (ret)
		goto exit;

	ret = fast_reset_init(node, field);
	if (ret)
		goto exit;

exit:
	return ret;
}

static int fast_decimal_init_individ(xmlNodePtr node, struct fast_field *field)
{
	struct fast_decimal *decimal;
	int ret = -1;

	decimal = &field->decimal_value;

	decimal->fields = calloc(2, sizeof(struct fast_field));
	if (!decimal->fields)
		goto exit;

	node = node->xmlChildrenNode;

	while (node) {
		if (node->type != XML_ELEMENT_NODE) {
			node = node->next;
			continue;
		}

		if (!xmlStrcmp(node->name, (const xmlChar *)"exponent"))
			ret = fast_field_init(node, &decimal->fields[0]);
		else if (!xmlStrcmp(node->name, (const xmlChar *)"mantissa"))
			ret = fast_field_init(node, &decimal->fields[1]);
		else
			ret = -1;

		if (ret)
			goto exit;

		node = node->next;
	}

	field_add_flags(field, FAST_FIELD_FLAGS_DECIMAL_INDIVID);
	decimal->fields[0].presence = field->presence;

exit:
	return ret;
}

static int fast_field_init(xmlNodePtr node, struct fast_field *field)
{
	int ret;

	field->state = FAST_STATE_UNDEFINED;

	ret = fast_type_init(node, field);
	if (ret)
		goto exit;

	ret = fast_presence_init(node, field);
	if (ret)
		goto exit;

	ret = fast_misc_init(node, field);
	if (ret)
		goto exit;

	switch (field->type) {
	case FAST_TYPE_INT:
	case FAST_TYPE_UINT:
	case FAST_TYPE_STRING:
	case FAST_TYPE_VECTOR:
		node = node->xmlChildrenNode;

		while (node && node->type != XML_ELEMENT_NODE)
			node = node->next;

		ret = fast_op_init(node, field);
		if (ret)
			goto exit;

		ret = fast_reset_init(node, field);
		if (ret)
			goto exit;

		break;
	case FAST_TYPE_DECIMAL:
		field->decimal_value.fields = NULL;

		ret = fast_decimal_init_atomic(node, field);
		if (!ret)
			goto exit;

		ret = fast_decimal_init_individ(node, field);

		break;
	case FAST_TYPE_SEQUENCE:
		ret = fast_sequence_init(node, field);
		break;
	default:
		ret = 1;
		goto exit;
	}

exit:
	return ret;
}

static int fast_message_init(xmlNodePtr node, struct fast_message *msg)
{
	struct fast_field *field;
	int nr_fields;
	xmlChar *prop;
	int ret = 1;

	if (xmlStrcmp(node->name, (const xmlChar *)"template"))
		goto exit;

	prop = xmlGetProp(node, (const xmlChar *)"id");
	if (prop != NULL)
		msg->tid = strtol((char *)prop, NULL, 10);
	else
		msg->tid = 0;
	xmlFree(prop);

	prop = xmlGetProp(node, (const xmlChar *)"name");
	if (prop != NULL)
		strncpy(msg->name, (const char *)prop, sizeof(msg->name));
	else
		strncpy(msg->name, (const char *)"", sizeof(msg->name));
	xmlFree(prop);

	prop = xmlGetProp(node, (const xmlChar *)"reset");
	if (prop != NULL && !xmlStrcmp(prop, (const xmlChar *)"T"))
		fast_msg_add_flags(msg, FAST_MSG_FLAGS_RESET);
	xmlFree(prop);

	nr_fields = xmlChildElementCount(node);
	msg->fields = calloc(nr_fields, sizeof(struct fast_field));
	if (!msg->fields)
		goto exit;

	msg->nr_fields = 0;

	msg->ghtab = g_hash_table_new(g_str_hash, g_str_equal);
	if (!msg->ghtab)
		goto exit;

	node = node->xmlChildrenNode;
	while (node != NULL) {
		if (node->type != XML_ELEMENT_NODE) {
			node = node->next;
			continue;
		}

		field = msg->fields + msg->nr_fields;

		if (fast_field_init(node, field))
			goto exit;

		if (strlen(field->name))
			g_hash_table_insert(msg->ghtab, field->name, field);

		msg->nr_fields++;
		node = node->next;
	}

	ret = 0;

exit:
	return ret;
}

int fast_parse_template(struct fast_session *self, const char *xml)
{
	struct fast_message *msg;
	xmlNodePtr node;
	xmlDocPtr doc;
	int ret = 1;

	doc = xmlParseFile(xml);
	if (doc == NULL)
		goto exit;

	node = xmlDocGetRootElement(doc);
	if (node == NULL)
		goto exit;

	if (xmlStrcmp(node->name, (const xmlChar *)"templates"))
		goto free;

	if (xmlChildElementCount(node) > FAST_TEMPLATE_MAX_NUMBER)
		goto free;

	node = node->xmlChildrenNode;
	while (node != NULL) {
		if (node->type != XML_ELEMENT_NODE) {
			node = node->next;
			continue;
		}

		msg = self->rx_messages + self->nr_messages;
		if (fast_message_init(node, msg))
			goto free;

		self->nr_messages++;
		node = node->next;
	}

	ret = 0;

free:
	xmlFreeDoc(doc);

exit:
	return ret;
}
