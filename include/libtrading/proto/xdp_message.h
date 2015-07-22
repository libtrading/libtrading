#ifndef LIBTRADING_XDP_MESSAGE_H
#define LIBTRADING_XDP_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/types.h"

#include <stddef.h>

struct buffer;

/*
 * Message types:
 */
enum xdp_msg_type {
	XDP_MSG_ORDER_BOOK_ADD_ORDER		= 100,
	XDP_MSG_ORDER_BOOK_MODIFY		= 101,
	XDP_MSG_ORDER_BOOK_DELETE		= 102,
	XDP_MSG_ORDER_BOOK_EXECUTION		= 103,
	XDP_MSG_ORDER_BOOK_ADD_ORDER_REFRESH	= 106,
	XDP_MSG_TRADE				= 220,
	XDP_MSG_TRADE_CANCEL_OR_BUST		= 221,
	XDP_MSG_TRADE_CORRECTION		= 222,
	XDP_MSG_STOCK_SUMMARY			= 223,
	XDP_MSG_PBBO				= 104,
	XDP_MSG_IMBALANCE			= 105,
};

/*
 * A data structure for XDP messages.
 */
struct xdp_message {
	le16			MsgSize;
	le16			MsgType;
};

/* XDP_MSG_ORDER_BOOK_ADD_ORDER */
struct xdp_msg_order_book_add_order {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			OrderID;
	le32			Price;
	le32			Volume;
	char			Side;
	char			OrderIDGTCIndicator;
	char			TradeSession;
} __attribute__((packed));

/* XDP_MSG_ORDER_BOOK_MODIFY */
struct xdp_msg_order_book_modify {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			OrderID;
	le32			Price;
	le32			Volume;
	char			Side;
	char			OrderIDGTCIndicator;
	char			ReasonCode;
} __attribute__((packed));

/* XDP_MSG_ORDER_BOOK_DELETE */
struct xdp_msg_order_book_delete {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			OrderID;
	char			Side;
	char			OrderIDGTCIndicator;
	char			ReasonCode;
} __attribute__((packed));

/* XDP_MSG_ORDER_BOOK_EXECUTION */
struct xdp_msg_order_book_execution {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			OrderID;
	le32			Price;
	le32			Volume;
	char			Side;
	char			OrderIDGTCIndicator;
	char			ReasonCode;
	le32			TradeID;
} __attribute__((packed));

/* XDP_MSG_ORDER_BOOK_ADD_ORDER_REFRESH */
struct xdp_msg_order_book_add_order_refresh {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTime;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			OrderID;
	le32			Price;
	le32			Volume;
	char			Side;
	char			OrderIDGTCIndicator;
	char			TradeSession;
} __attribute__((packed));

/* XDP_MSG_TRADE */
struct xdp_msg_trade {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTime;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			TradeID;
	le32			Price;
	le32			Volume;
	char			TradeCond1;
	char			TradeCond2;
	char			TradeCond3;
	char			TradeCond4;
	char			TradeThroughExempt;
	char			LiquidityIndicatorFlag;
	le32			AskPrice;
	le32			AskVolume;
	le32			BidPrice;
	le32			BidVolume;
} __attribute__((packed));

/* XDP_MSG_TRADE_CANCEL_OR_BUST */
struct xdp_msg_trade_cancel_or_bust {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTime;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			OriginalTradeID;
} __attribute__((packed));

/* XDP_MSG_TRADE_CORRECTION */
struct xdp_msg_trade_correction {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTime;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			OriginalTradeID;
	le32			TradeID;
	le32			Price;
	le32			Volume;
	char			TradeCond1;
	char			TradeCond2;
	char			TradeCond3;
	char			TradeCond4;
	char			TradeThroughExempt;
} __attribute__((packed));

/* XDP_MSG_STOCK_SUMMARY */
struct xdp_msg_stock_summary {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTime;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			HighPrice;
	le32			LowPrice;
	le32			Open;
	le32			Close;
	le32			TotalVolume;
} __attribute__((packed));

/* XDP_MSG_PBBO */
struct xdp_msg_pbbo {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTime;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			BidPrice;
	le32			AskPrice;
} __attribute__((packed));

/* XDP_MSG_IMBALANCE */
struct xdp_msg_imbalance {
	le16			MsgSize;
	le16			MsgType;
	le32			SourceTime;
	le32			SourceTimeNS;
	le32			SymbolIndex;
	le32			SymbolSeqNum;
	le32			ReferencePrice;
	le32			PairedQty;
	le32			TotalImbalanceQty;
	le32			MarketImbalanceQty;
	le16			AuctionTime;
	char			AuctionType;
	char			ImbalanceSide;
	le32			ContinuousBookClearingPrice;
	le32			ClosingOnlyClearingPrice;
	le32			SSRFilingPrice;
} __attribute__((packed));

int xdp_message_decode(struct buffer *buf, struct xdp_message *msg, size_t size);

#ifdef __cplusplus
}
#endif

#endif
