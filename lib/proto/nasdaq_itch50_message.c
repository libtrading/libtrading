#include "libtrading/proto/nasdaq_itch50_message.h"

#include "libtrading/buffer.h"

#include <stdlib.h>
#include <string.h>

static unsigned long itch50_message_size(u8 type)
{
	switch (type) {
        case ITCH50_MSG_SYSTEM_EVENT:
		return sizeof(struct itch50_msg_system_event);
        case ITCH50_MSG_STOCK_DIRECTORY:
		return sizeof(struct itch50_msg_stock_directory);
        case ITCH50_MSG_STOCK_TRADING_ACTION:
		return sizeof(struct itch50_msg_stock_trading_action);
        case ITCH50_MSG_REG_SHO_RESTRICTION:
		return sizeof(struct itch50_msg_reg_sho_restriction);
        case ITCH50_MSG_MARKET_PARTICIPANT_POS:
		return sizeof(struct itch50_msg_market_participant_pos);
        case ITCH50_MSG_MWCB_DECLINE_LEVEL:
		return sizeof(struct itch50_msg_mwcb_decline_level);
        case ITCH50_MSG_MWCB_STATUS:
		return sizeof(struct itch50_msg_mwcb_status);
        case ITCH50_MSG_IPO_QUOTING_PERIOD_UPDATE:
		return sizeof(struct itch50_msg_ipo_quoting_period_update);
        case ITCH50_MSG_ADD_ORDER:
		return sizeof(struct itch50_msg_add_order);
        case ITCH50_MSG_ADD_ORDER_MPID:
		return sizeof(struct itch50_msg_add_order_mpid);
        case ITCH50_MSG_ORDER_EXECUTED:
		return sizeof(struct itch50_msg_order_executed);
        case ITCH50_MSG_ORDER_EXECUTED_WITH_PRICE:
		return sizeof(struct itch50_msg_order_executed_with_price);
        case ITCH50_MSG_ORDER_CANCEL:
		return sizeof(struct itch50_msg_order_cancel);
        case ITCH50_MSG_ORDER_DELETE:
		return sizeof(struct itch50_msg_order_delete);
        case ITCH50_MSG_ORDER_REPLACE:
		return sizeof(struct itch50_msg_order_replace);
        case ITCH50_MSG_TRADE:
		return sizeof(struct itch50_msg_trade);
        case ITCH50_MSG_CROSS_TRADE:
		return sizeof(struct itch50_msg_cross_trade);
        case ITCH50_MSG_BROKEN_TRADE:
		return sizeof(struct itch50_msg_broken_trade);
        case ITCH50_MSG_NOII:
		return sizeof(struct itch50_msg_noii);
        case ITCH50_MSG_RPII:
		return sizeof(struct itch50_msg_rpii);
	default:
		break;
	};

	return 0;
}

struct itch50_message *itch50_message_decode(struct buffer *buf)
{
	size_t available;
	size_t size;
	void *start;
	u8 type;

	available = buffer_size(buf);

	if (!available)
		return NULL;

	type = buffer_peek_8(buf);

	size = itch50_message_size(type);
	if (!size)
		return NULL;

	if (available < size)
		return NULL;

	start = buffer_start(buf);

	buffer_advance(buf, size);

	return start;
}
