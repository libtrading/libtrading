#include "libtrading/proto/itch41_message.h"

#include "libtrading/buffer.h"

#include <stdlib.h>
#include <string.h>

static unsigned long itch41_message_size(u8 type)
{
	switch (type) {
        case ITCH41_MSG_TIMESTAMP_SECONDS:
		return sizeof(struct itch41_msg_timestamp_seconds);
        case ITCH41_MSG_SYSTEM_EVENT:
		return sizeof(struct itch41_msg_system_event);
        case ITCH41_MSG_STOCK_DIRECTORY:
		return sizeof(struct itch41_msg_stock_directory);
        case ITCH41_MSG_STOCK_TRADING_ACTION:
		return sizeof(struct itch41_msg_stock_trading_action);
        case ITCH41_MSG_REG_SHO_RESTRICTION:
		return sizeof(struct itch41_msg_reg_sho_restriction);
        case ITCH41_MSG_MARKET_PARTICIPANT_POS:
		return sizeof(struct itch41_msg_market_participant_pos);
        case ITCH41_MSG_ADD_ORDER:
		return sizeof(struct itch41_msg_add_order);
        case ITCH41_MSG_ADD_ORDER_MPID:
		return sizeof(struct itch41_msg_add_order_mpid);
        case ITCH41_MSG_ORDER_EXECUTED:
		return sizeof(struct itch41_msg_order_executed);
        case ITCH41_MSG_ORDER_EXECUTED_WITH_PRICE:
		return sizeof(struct itch41_msg_order_executed_with_price);
        case ITCH41_MSG_ORDER_CANCEL:
		return sizeof(struct itch41_msg_order_cancel);
        case ITCH41_MSG_ORDER_DELETE:
		return sizeof(struct itch41_msg_order_delete);
        case ITCH41_MSG_ORDER_REPLACE:
		return sizeof(struct itch41_msg_order_replace);
        case ITCH41_MSG_TRADE:
		return sizeof(struct itch41_msg_trade);
        case ITCH41_MSG_CROSS_TRADE:
		return sizeof(struct itch41_msg_cross_trade);
        case ITCH41_MSG_BROKEN_TRADE:
		return sizeof(struct itch41_msg_broken_trade);
        case ITCH41_MSG_NOII:
		return sizeof(struct itch41_msg_noii);
	default:
		break;
	};

	return 0;
}

struct itch41_message *itch41_message_decode(struct buffer *buf)
{
	size_t available;
	size_t size;
	void *start;
	u8 type;

	available = buffer_size(buf);

	if (!available)
		return NULL;

	type = buffer_peek_8(buf);

	size = itch41_message_size(type);
	if (!size)
		return NULL;

	if (available < size)
		return NULL;

	start = buffer_start(buf);

	buffer_advance(buf, size);

	return start;
}
