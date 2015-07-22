#ifndef LIBTRADING_OMX_ITCH186_MESSAGE_H
#define LIBTRADING_OMX_ITCH186_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/types.h"

struct buffer;

/*
 * Message types:
 */
enum omx_itch186_msg_type {
	OMX_ITCH186_MSG_SECONDS				= 'T', /* Section 4.1.1 */
	OMX_ITCH186_MSG_MILLISECONDS			= 'M', /* Section 4.1.2 */
	OMX_ITCH186_MSG_SYSTEM_EVENT			= 'S', /* Section 4.2.1 */
	OMX_ITCH186_MSG_MARKET_SEGMENT_STATE		= 'O', /* Section 4.2.2 */
	OMX_ITCH186_MSG_ORDER_BOOK_DIRECTORY		= 'R', /* Section 4.3.1 */
	OMX_ITCH186_MSG_ORDER_BOOK_TRADING_ACTION	= 'H', /* Section 4.3.2 */
	OMX_ITCH186_MSG_ADD_ORDER			= 'A', /* Section 4.4.1 */
	OMX_ITCH186_MSG_ADD_ORDER_MPID			= 'F', /* Section 4.4.2 */
	OMX_ITCH186_MSG_ORDER_EXECUTED			= 'E', /* Section 4.5.1 */
	OMX_ITCH186_MSG_ORDER_EXECUTED_WITH_PRICE	= 'C', /* Section 4.5.2 */
	OMX_ITCH186_MSG_ORDER_CANCEL			= 'X', /* Section 4.5.3 */
	OMX_ITCH186_MSG_ORDER_DELETE			= 'D', /* Section 4.5.4 */
	OMX_ITCH186_MSG_TRADE				= 'P', /* Section 4.6.1 */
	OMX_ITCH186_MSG_CROSS_TRADE			= 'Q', /* Section 4.6.2 */
	OMX_ITCH186_MSG_BROKEN_TRADE			= 'B', /* Section 4.7 */
	OMX_ITCH186_MSG_NOII				= 'I', /* Section 4.8 */
};

struct omx_itch186_message {
	u8			MessageType;
} __attribute__((packed));

struct omx_itch186_msg_seconds {
	u8			MessageType;
	char			Second[5];
} __attribute__((packed));

struct omx_itch186_msg_milliseconds {
	u8			MessageType;
	char			Millisecond[3];
} __attribute__((packed));

struct omx_itch186_msg_system_event {
	u8			MessageType;
	char			EventCode;
} __attribute__((packed));

struct omx_itch186_msg_market_segment_state {
	u8			MessageType;
	char			MarketSegmentID[3];
	char			EventCode;
} __attribute__((packed));

struct omx_itch186_msg_order_book_directory {
	u8			MessageType;
	char			OrderBook[6];
	char			Symbol[16];
	char			ISIN[12];
	char			FinancialProduct[3];
	char			TradingCurrency[3];
	char			MIC[4];
	char			MarketSegmentID[3];
	char			NoteCodes[8];
	char			RoundLotSize[9];
} __attribute__((packed));

struct omx_itch186_msg_order_book_trading_action {
	u8			MessageType;
	char			OrderBook[6];
	char			TradingState;
	char			Reserved;
	char			Reason[4];
} __attribute__((packed));

struct omx_itch186_msg_add_order {
	u8			MessageType;
	char			OrderReferenceNumber[9];
	char			BuySellIndicator;
	char			Quantity[9];
	char			OrderBook[6];
	char			Price[10];
} __attribute__((packed));

struct omx_itch186_msg_add_order_mpid {
	u8			MessageType;
	char			OrderReferenceNumber[9];
	char			BuySellIndicator;
	char			Quantity[9];
	char			OrderBook[6];
	char			Price[10];
	char			Attribution[4];
} __attribute__((packed));

struct omx_itch186_msg_order_executed {
	u8			MessageType;
	char			OrderReferenceNumber[9];
	char			ExecutedQuantity[9];
	char			MatchNumber[9];
	char			OwnerParticipantID[4];
	char			CounterpartyParticipantID[4];
} __attribute__((packed));

struct omx_itch186_msg_order_executed_with_price {
	u8			MessageType;
	char			OrderReferenceNumber[9];
	char			ExecutedQuantity[9];
	char			MatchNumber[9];
	char			Printable;
	char			TradePrice[10];
	char			OwnerParticipantID[4];
	char			CounterpartyParticipantID[4];
} __attribute__((packed));

struct omx_itch186_msg_order_cancel {
	u8			MessageType;
	char			OrderReferenceNumber[9];
	char			CanceledQuantity[9];
} __attribute__((packed));

struct omx_itch186_msg_order_delete {
	u8			MessageType;
	char			OrderReferenceNumber[9];
} __attribute__((packed));

struct omx_itch186_msg_trade {
	u8			MessageType;
	char			OrderReferenceNumber[9];
	char			TradeType;
	char			Quantity[9];
	char			OrderBook[6];
	char			MatchNumber[9];
	char			TradePrice[10];
	char			BuyerParticipantID[4];
	char			SellerParticipantID[4];
} __attribute__((packed));

struct omx_itch186_msg_cross_trade {
	u8			MessageType;
	char			Quantity[9];
	char			OrderBook[6];
	char			CrossPrice[10];
	char			MatchNumber[9];
	char			CrossType;
	char			NumberOfTrades[10];
} __attribute__((packed));

struct omx_itch186_msg_broken_trade {
	u8			MessageType;
	char			MatchNumber[9];
} __attribute__((packed));

struct omx_itch186_msg_noii {
	u8			MessageType;
	char			PairedQuantity[9];
	char			ImbalanceQuantity[9];
	char			ImbalanceDirection;
	char			OrderBook[6];
	char			EquilibriumPrice[10];
	char			CrossType;
	char			BestBidPrice[10];
	char			BestBidQuantity[9];
	char			BestAskPrice[10];
	char			BestAskQuantity[9];
} __attribute__((packed));

unsigned long omx_itch186_message_size(u8 type);

struct omx_itch186_message *omx_itch186_message_decode(struct buffer *buf);

#ifdef __cplusplus
}
#endif

#endif
