#ifndef LIBTRADING_ITCH4_MESSAGE_H
#define LIBTRADING_ITCH4_MESSAGE_H

struct buffer;

#include "trading/types.h"

/*
 * Message types:
 */
enum itch4_msg_type {
	ITCH4_MSG_TIMESTAMP_SECONDS		= 'T',	/* Section 4.1 */
	ITCH4_MSG_SYSTEM_EVENT			= 'S',	/* Section 4.2 */
	ITCH4_MSG_STOCK_DIRECTORY		= 'R',	/* Section 4.3.1. */
	ITCH4_MSG_STOCK_TRADING_ACTION		= 'H',	/* Section 4.3.2. */
	ITCH4_MSG_REG_SHO_RESTRICTION		= 'Y',	/* Section 4.3.3. */
	ITCH4_MSG_MARKET_PARTICIPANT_POS	= 'L',	/* Section 4.3.4. */
	ITCH4_MSG_ADD_ORDER			= 'A',	/* Section 4.4.1. */
	ITCH4_MSG_ADD_ORDER_MPID		= 'F',	/* Section 4.4.2. */
	ITCH4_MSG_ORDER_EXECUTED		= 'E',	/* Section 4.5.1. */
	ITCH4_MSG_ORDER_EXECUTED_WITH_PRICE	= 'C',	/* Section 4.5.2. */
	ITCH4_MSG_ORDER_CANCEL			= 'X',	/* Section 4.5.3. */
	ITCH4_MSG_ORDER_DELETE			= 'D',	/* Section 4.5.4. */
	ITCH4_MSG_ORDER_REPLACE			= 'U',	/* Section 4.5.5. */
	ITCH4_MSG_TRADE				= 'P',	/* Section 4.6.1. */
	ITCH4_MSG_CROSS_TRADE			= 'Q',	/* Section 4.6.2. */
	ITCH4_MSG_BROKEN_TRADE			= 'B',	/* Section 4.6.3. */
	ITCH4_MSG_NOII				= 'I',	/* Section 4.7. */
};

/*
 * System event codes:
 */
enum itch4_event_code {
	ITCH4_EVENT_START_OF_MESSAGES		= 'O',
	ITCH4_EVENT_START_OF_SYSTEM_HOURS	= 'S',
	ITCH4_EVENT_START_OF_MARKET_HOURS	= 'Q',
	ITCH4_EVENT_END_OF_MARKET_HOURS		= 'M',
	ITCH4_EVENT_END_OF_SYSTEM_HOURS		= 'E',
	ITCH4_EVENT_END_OF_MESSAGES		= 'C',
	ITCH4_EVENT_EMERGENCY_HALT		= 'A',
	ITCH4_EVENT_EMERGENCY_QUOTE_ONLY	= 'R',
	ITCH4_EVENT_EMERGENCY_RESUMPTION	= 'B',
};

/*
 * A data structure for ITCH messages.
 */
struct itch4_message {
	u8			MessageType;
};

/* ITCH4_MSG_TIMESTAMP_SECONDS */
struct itch4_msg_timestamp_seconds {
	u8			MessageType;
	be32			Second;	
} packed;

/* ITCH4_MSG_SYSTEM_EVENT */
struct itch4_msg_system_event {
	u8			MessageType;
	be32			Timestamp;
	char			EventCode;	/* ITCH4_EVENT_<code> */
} packed;

/* ITCH4_MSG_STOCK_DIRECTORY */
struct itch4_msg_stock_directory {
	u8			MessageType;
	be32			TimestampNanoseconds;
	char			Stock[8];
	char			MarketCategory;
	char			FinancialStatusIndicator;
	be32			RoundLotSize;
	char			RoundLotsOnly;
} packed;

/* ITCH4_MSG_STOCK_TRADING_ACTION */
struct itch4_msg_stock_trading_action {
	u8			MessageType;
	be32			TimestampNanoseconds;
	char			Stock[8];
	char			TradingState;
	char			Reserved;
	char			Reason[4];
} packed;

/* ITCH4_MSG_REG_SHO_RESTRICTION */
struct itch4_msg_reg_sho_restriction {
	u8			MessageType;
	be32			TimestampNanoseconds;
	char			Stock[8];
	char			RegSHOAction;
} packed;

/* ITCH4_MSG_MARKET_PARTICIPANT_POS */
struct itch4_msg_market_participant_pos {
	u8			MessageType;
	be32			TimestampNanoseconds;
	char			MPID[4];
	char			Stock[8];
	char			PrimaryMarketMaker;
	char			MarketMakerMode;
	char			MarketParticipantState;	
} packed;

/* ITCH4_MSG_ADD_ORDER */
struct itch4_msg_add_order {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
} packed;

/* ITCH4_MSG_ADD_ORDER_MPID */
struct itch4_msg_add_order_mpid {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
	char			Attribution[4];
} packed;

/* ITCH4_MSG_ORDER_EXECUTED */
struct itch4_msg_order_executed {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	be32			ExecutedShares;
	be64			MatchNumber;
} packed;

/* ITCH4_MSG_ORDER_EXECUTED_WITH_PRICE */
struct itch4_msg_order_executed_with_price {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	be32			ExecutedShares;
	be64			MatchNumber;
	char			Printable;
	be32			ExecutionPrice;
} packed;

/* ITCH4_MSG_ORDER_CANCEL */
struct itch4_msg_order_cancel {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	be32			CanceledShares;
} packed;

/* ITCH4_MSG_ORDER_DELETE */
struct itch4_msg_order_delete {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
} packed;

/* ITCH4_MSG_ORDER_REPLACE */
struct itch4_msg_order_replace {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OriginalOrderReferenceNumber;
	be64			NewOrderReferenceNumber;
	be32			Shares;
	be32			Price;
} packed;

/* ITCH4_MSG_TRADE */
struct itch4_msg_trade {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
	be64			MatchNumber;
} packed;

/* ITCH4_MSG_CROSS_TRADE */
struct itch4_msg_cross_trade {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			Shares;
	char			Stock[8];
	be32			CrossPrice;
	be64			MatchNumber;
	char			CrossType;
} packed;

/* ITCH4_MSG_BROKEN_TRADE */
struct itch4_msg_broken_trade {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			MatchNumber;
} packed;

/* ITCH4_MSG_NOII */
struct itch4_msg_noii {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			PairedShares;
	be64			ImbalanceShares;
	char			ImbalanceDirection;
	char			Stock[8];
	be32			FarPrice;
	be32			NearPrice;
	be32			CurrentReferencePrice;
	char			CrossType;
	char			PriceVariationIndicator;
} packed;

int itch4_message_decode(struct buffer *buf, struct itch4_message *msg);

#endif
