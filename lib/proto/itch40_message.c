#include "libtrading/proto/itch40_message.h"

#include "libtrading/buffer.h"

#include <stdlib.h>
#include <string.h>

static unsigned long itch40_message_size(u8 type)
{
	switch (type) {
        case ITCH40_MSG_TIMESTAMP_SECONDS:
		return sizeof(struct itch40_msg_timestamp_seconds);
        case ITCH40_MSG_SYSTEM_EVENT:
		return sizeof(struct itch40_msg_system_event);
        case ITCH40_MSG_STOCK_DIRECTORY:
		return sizeof(struct itch40_msg_stock_directory);
        case ITCH40_MSG_STOCK_TRADING_ACTION:
		return sizeof(struct itch40_msg_stock_trading_action);
        case ITCH40_MSG_MARKET_PARTICIPANT_POS:
		return sizeof(struct itch40_msg_market_participant_pos);
        case ITCH40_MSG_ADD_ORDER:
		return sizeof(struct itch40_msg_add_order);
        case ITCH40_MSG_ADD_ORDER_MPID:
		return sizeof(struct itch40_msg_add_order_mpid);
        case ITCH40_MSG_ORDER_EXECUTED:
		return sizeof(struct itch40_msg_order_executed);
        case ITCH40_MSG_ORDER_EXECUTED_WITH_PRICE:
		return sizeof(struct itch40_msg_order_executed_with_price);
        case ITCH40_MSG_ORDER_CANCEL:
		return sizeof(struct itch40_msg_order_cancel);
        case ITCH40_MSG_ORDER_DELETE:
		return sizeof(struct itch40_msg_order_delete);
        case ITCH40_MSG_ORDER_REPLACE:
		return sizeof(struct itch40_msg_order_replace);
        case ITCH40_MSG_TRADE:
		return sizeof(struct itch40_msg_trade);
        case ITCH40_MSG_CROSS_TRADE:
		return sizeof(struct itch40_msg_cross_trade);
        case ITCH40_MSG_BROKEN_TRADE:
		return sizeof(struct itch40_msg_broken_trade);
        case ITCH40_MSG_NOII:
		return sizeof(struct itch40_msg_noii);
	default:
		break;
	};

	return 0;
}

int itch40_message_decode(struct buffer *buf, struct itch40_message *msg)
{
	size_t available;
	size_t size;
	u8 type;

	available = buffer_size(buf);

	if (!available)
		return -1;

	type = buffer_peek_8(buf);

	size = itch40_message_size(type);
	if (!size)
		return -1;

	if (available < size)
		return -1;

	memcpy(msg, buffer_start(buf), size);

	buffer_advance(buf, size);

	return 0;
}
