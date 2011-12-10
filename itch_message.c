#include "trading/itch_message.h"

#include "trading/buffer.h"

#include <stdlib.h>
#include <string.h>

static unsigned long itch_message_size(u8 type)
{
	switch (type) {
        case ITCH_MSG_TIMESTAMP_SECONDS:
		return sizeof(struct itch_msg_timestamp_seconds);
        case ITCH_MSG_SYSTEM_EVENT:
		return sizeof(struct itch_msg_system_event);
        case ITCH_MSG_STOCK_DIRECTORY:
		return sizeof(struct itch_msg_stock_directory);
        case ITCH_MSG_STOCK_TRADING_ACTION:
		return sizeof(struct itch_msg_stock_trading_action);
        case ITCH_MSG_REG_SHO_RESTRICTION:
		return sizeof(struct itch_msg_reg_sho_restriction);
        case ITCH_MSG_MARKET_PARTICIPANT_POS:
		return sizeof(struct itch_msg_market_participant_pos);
        case ITCH_MSG_ADD_ORDER:
		return sizeof(struct itch_msg_add_order);
        case ITCH_MSG_ADD_ORDER_MPID:
		return sizeof(struct itch_msg_add_order_mpid);
        case ITCH_MSG_ORDER_EXECUTED:
		return sizeof(struct itch_msg_order_executed);
        case ITCH_MSG_ORDER_EXECUTED_WITH_PRICE:
		return sizeof(struct itch_msg_order_executed_with_price);
        case ITCH_MSG_ORDER_CANCEL:
		return sizeof(struct itch_msg_order_cancel);
        case ITCH_MSG_ORDER_DELETE:
		return sizeof(struct itch_msg_order_delete);
        case ITCH_MSG_ORDER_REPLACE:
		return sizeof(struct itch_msg_order_replace);
        case ITCH_MSG_TRADE:
		return sizeof(struct itch_msg_trade);
        case ITCH_MSG_CROSS_TRADE:
		return sizeof(struct itch_msg_cross_trade);
        case ITCH_MSG_BROKEN_TRADE:
		return sizeof(struct itch_msg_broken_trade);
        case ITCH_MSG_NOII:
		return sizeof(struct itch_msg_noii);
	default:
		break;
	};

	return 0;
}

struct itch_message *itch_message_decode(struct buffer *buf)
{
	struct itch_message *msg;
	void *start;
	size_t size;
	u8 type;

	start = buffer_start(buf);

	type = buffer_get_8(buf);

	size = itch_message_size(type);
	if (!size)
		return NULL;

	msg = malloc(size);
	if (!msg)
		return NULL;

	memcpy(msg, start, size);

	buffer_advance(buf, size);

	return msg;
}
