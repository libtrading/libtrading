#include "libtrading/proto/fast_message.h"
#include "libtrading/proto/fast_session.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>

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
	else if (!xmlStrcmp(node->name, (const xmlChar *)"constant"))
		field->op = FAST_OP_CONSTANT;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"increment"))
		field->op = FAST_OP_INCR;
	else
		ret = 1;

	return ret;
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
	int pmap_bit;

	field->ptr_value = calloc(1, sizeof(struct fast_sequence));
	if (!field->ptr_value)
		goto exit;

	seq = field->ptr_value;

	nr_fields = xmlChildElementCount(node);

	node = node->xmlChildrenNode;
	while (node && node->type != XML_ELEMENT_NODE)
		node = node->next;

	if (xmlStrcmp(node->name, (const xmlChar *)"length"))
		goto exit;

	if (fast_field_init(node, &seq->length))
		goto exit;

	node = node->next;
	orig = field;

	for (i = 0; i < FAST_SEQUENCE_ELEMENTS; i++) {
		msg = seq->elements + i;
		tmp = node;

		msg->fields = calloc(nr_fields, sizeof(struct fast_field));
		if (!msg->fields)
			goto exit;

		msg->nr_fields = 0;
		pmap_bit = 0;

		while (tmp != NULL) {
			if (tmp->type != XML_ELEMENT_NODE) {
				tmp = tmp->next;
				continue;
			}

			field = msg->fields + msg->nr_fields;

			if (fast_field_init(tmp, field))
				goto exit;

			if (pmap_required(field)) {
				field_add_flags(orig, FAST_FIELD_FLAGS_PMAPREQ);
				field->pmap_bit = pmap_bit++;
			}

			msg->nr_fields++;
			tmp = tmp->next;
		}
	}

	ret = 0;

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
	case FAST_TYPE_DECIMAL:
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
	int pmap_bit;
	int ret = 1;

	if (xmlStrcmp(node->name, (const xmlChar *)"template"))
		goto exit;

	nr_fields = xmlChildElementCount(node);

	prop = xmlGetProp(node, (const xmlChar *)"id");

	msg->fields = calloc(nr_fields, sizeof(struct fast_field));
	if (!msg->fields)
		goto exit;

	if (prop != NULL)
		msg->tid = strtol((char *)prop, NULL, 10);
	else
		msg->tid = 0;

	msg->nr_fields = 0;
	pmap_bit = 1;

	node = node->xmlChildrenNode;
	while (node != NULL) {
		if (node->type != XML_ELEMENT_NODE) {
			node = node->next;
			continue;
		}

		field = msg->fields + msg->nr_fields;

		if (fast_field_init(node, field))
			goto exit;

		if (pmap_required(field))
			field->pmap_bit = pmap_bit++;

		msg->nr_fields++;
		node = node->next;
	}

	ret = 0;

exit:
	return ret;
}

int fast_suite_template(struct fast_session *self, const char *xml)
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

int fast_micex_template(struct fast_session *self, const char *xml)
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
