#include "trading/ouch42_message.h"

#include "trading/buffer.h"

#include <string.h>

static unsigned long ouch42_in_message_size(u8 type)
{
	switch (type) {
	case OUCH42_MSG_ENTER_ORDER:
		return sizeof (struct ouch42_msg_enter_order);
	case OUCH42_MSG_REPLACE_ORDER:
		return sizeof (struct ouch42_msg_replace_order);
	case OUCH42_MSG_CANCEL_ORDER:
		return sizeof (struct ouch42_msg_cancel_order);
	case OUCH42_MSG_MODIFY_ORDER:
		return sizeof (struct ouch42_msg_modify_order);
	default:
		break;
	};

	return 0;
}

static unsigned long ouch42_out_message_size(u8 type)
{
	switch (type) {
	case OUCH42_MSG_SYSTEM_EVENT:
		return sizeof (struct ouch42_msg_system_event);
	case OUCH42_MSG_ACCEPTED:
		return sizeof (struct ouch42_msg_accepted);
	case OUCH42_MSG_REPLACED:
		return sizeof (struct ouch42_msg_replaced);
	case OUCH42_MSG_CANCELED:
		return sizeof (struct ouch42_msg_canceled);
	case OUCH42_MSG_AIQ_CANCELED:
		return sizeof (struct ouch42_msg_aiq_canceled);
	case OUCH42_MSG_EXECUTED:
		return sizeof (struct ouch42_msg_executed);
	case OUCH42_MSG_BROKEN_TRADE:
		return sizeof (struct ouch42_msg_broken_trade);
	case OUCH42_MSG_REJECTED:
		return sizeof (struct ouch42_msg_rejected);
	case OUCH42_MSG_CANCEL_PENDING:
		return sizeof (struct ouch42_msg_cancel_pending);
	case OUCH42_MSG_CANCEL_REJECT:
		return sizeof (struct ouch42_msg_cancel_reject);
	case OUCH42_MSG_ORDER_PRIO_UPDATE:
		return sizeof (struct ouch42_msg_order_prio_update);
	case OUCH42_MSG_ORDER_MODIFIED:
		return sizeof (struct ouch42_msg_order_modified);
	default:
		break;
	};

	return 0;
}

int ouch42_in_message_decode(struct buffer *buf, struct ouch42_message *msg)
{
	void *start;
	size_t size;
	u8 type;

	start = buffer_start(buf);

	type = buffer_get_8(buf);

	size = ouch42_in_message_size(type);
	if (!size)
		return -1;

	memcpy(msg, start, size);

	buffer_advance(buf, size);

	return 0;
}

int ouch42_out_message_decode(struct buffer *buf, struct ouch42_message *msg)
{
	void *start;
	size_t size;
	u8 type;

	start = buffer_start(buf);

	type = buffer_get_8(buf);

	size = ouch42_out_message_size(type);
	if (!size)
		return -1;

	memcpy(msg, start, size);

	buffer_advance(buf, size);

	return 0;
}
