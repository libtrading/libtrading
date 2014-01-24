#include "libtrading/proto/omx_itch186_message.h"

#include "libtrading/buffer.h"

#include <stdlib.h>
#include <string.h>

unsigned long omx_itch186_message_size(u8 type)
{
	switch (type) {
        case OMX_ITCH186_MSG_SECONDS:
		return sizeof(struct omx_itch186_msg_seconds);
        case OMX_ITCH186_MSG_MILLISECONDS:
		return sizeof(struct omx_itch186_msg_milliseconds);
        case OMX_ITCH186_MSG_SYSTEM_EVENT:
		return sizeof(struct omx_itch186_msg_system_event);
	case OMX_ITCH186_MSG_MARKET_SEGMENT_STATE:
		return sizeof(struct omx_itch186_msg_market_segment_state);
        case OMX_ITCH186_MSG_ORDER_BOOK_DIRECTORY:
		return sizeof(struct omx_itch186_msg_order_book_directory);
        case OMX_ITCH186_MSG_ORDER_BOOK_TRADING_ACTION:
		return sizeof(struct omx_itch186_msg_order_book_trading_action);
        case OMX_ITCH186_MSG_ADD_ORDER:
		return sizeof(struct omx_itch186_msg_add_order);
        case OMX_ITCH186_MSG_ADD_ORDER_MPID:
		return sizeof(struct omx_itch186_msg_add_order_mpid);
        case OMX_ITCH186_MSG_ORDER_EXECUTED:
		return sizeof(struct omx_itch186_msg_order_executed);
        case OMX_ITCH186_MSG_ORDER_EXECUTED_WITH_PRICE:
		return sizeof(struct omx_itch186_msg_order_executed_with_price);
        case OMX_ITCH186_MSG_ORDER_CANCEL:
		return sizeof(struct omx_itch186_msg_order_cancel);
        case OMX_ITCH186_MSG_ORDER_DELETE:
		return sizeof(struct omx_itch186_msg_order_delete);
        case OMX_ITCH186_MSG_TRADE:
		return sizeof(struct omx_itch186_msg_trade);
        case OMX_ITCH186_MSG_CROSS_TRADE:
		return sizeof(struct omx_itch186_msg_cross_trade);
        case OMX_ITCH186_MSG_BROKEN_TRADE:
		return sizeof(struct omx_itch186_msg_broken_trade);
        case OMX_ITCH186_MSG_NOII:
		return sizeof(struct omx_itch186_msg_noii);
	default:
		break;
	};

	return 0;
}

struct omx_itch186_message *omx_itch186_message_decode(struct buffer *buf)
{
	size_t available;
	size_t size;
	void *start;
	u8 type;

	available = buffer_size(buf);

	if (!available)
		return NULL;

	type = buffer_peek_8(buf);

	size = omx_itch186_message_size(type);
	if (!size)
		return NULL;

	if (available < size)
		return NULL;

	start = buffer_start(buf);

	buffer_advance(buf, size);

	return start;
}
