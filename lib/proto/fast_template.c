#include "libtrading/proto/fast_message.h"
#include "libtrading/proto/fast_session.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>

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
	else if (!xmlStrcmp(node->name, (const xmlChar *)"string") ||
			!xmlStrcmp(node->name, (const xmlChar *)"String"))
		field->type = FAST_TYPE_STRING;
	else if (!xmlStrcmp(node->name, (const xmlChar *)"decimal") ||
			!xmlStrcmp(node->name, (const xmlChar *)"Decimal"))
		field->type = FAST_TYPE_DECIMAL;
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
	default:
		ret = 1;
		break;
	}

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

	switch (field->type) {
	case FAST_TYPE_INT:
	case FAST_TYPE_UINT:
	case FAST_TYPE_STRING:
	case FAST_TYPE_DECIMAL:
		node = node->xmlChildrenNode;

		while (node && node->type == XML_TEXT_NODE)
			node = node->next;

		ret = fast_op_init(node, field);
		if (ret)
			goto exit;

		ret = fast_reset_init(node, field);
		if (ret)
			goto exit;

		break;
	default:
		ret = 1;
		goto exit;
	}

exit:
	return ret;
}

int fast_suite_template(struct fast_session *self, const char *xml)
{
	struct fast_message *msg;
	struct fast_field *field;
	xmlNodePtr template;
	xmlNodePtr element;
	int nr_fields;
	xmlDocPtr doc;
	int pmap_bit;
	int ret = 1;

	doc = xmlParseFile(xml);
	if (doc == NULL)
		goto exit;

	template = xmlDocGetRootElement(doc);
	if (template == NULL)
		goto exit;

	if (xmlStrcmp(template->name, (const xmlChar *)"templates"))
		goto free;

	if (xmlChildElementCount(template) > FAST_TEMPLATE_MAX_NUMBER)
		goto free;

	template = template->xmlChildrenNode;
	while (template != NULL) {
		if (template->type == XML_TEXT_NODE) {
			template = template->next;
			continue;
		}

		if (xmlStrcmp(template->name, (const xmlChar *)"template"))
			goto free;

		msg = self->rx_messages + self->nr_messages;
		nr_fields = xmlChildElementCount(template);

		msg->fields = calloc(nr_fields, sizeof(struct fast_field));
		if (!msg->fields)
			goto free;

		msg->nr_fields = 0;
		msg->tid = 0;

		pmap_bit = 1;

		element = template->xmlChildrenNode;
		while (element != NULL) {
			if (element->type == XML_TEXT_NODE) {
				element = element->next;
				continue;
			}

			field = msg->fields + msg->nr_fields;

			if (fast_field_init(element, field))
				goto free;

			if (pmap_required(field))
				field->pmap_bit = pmap_bit++;

			msg->nr_fields++;
			element = element->next;
		}

		self->nr_messages++;
		template = template->next;
	}

	ret = 0;

free:
	xmlFreeDoc(doc);

exit:
	return ret;
}
