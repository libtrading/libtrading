#ifndef LIBTRADING_NASDAQ_ITCH41_MESSAGE_H
#define LIBTRADING_NASDAQ_ITCH41_MESSAGE_H

#include "libtrading/types.h"

struct buffer;

/*
 * Message types:
 */
enum itch41_msg_type {
	ITCH41_MSG_TIMESTAMP_SECONDS		= 'T',	/* Section 4.1 */
	ITCH41_MSG_SYSTEM_EVENT			= 'S',	/* Section 4.2 */
	ITCH41_MSG_STOCK_DIRECTORY		= 'R',	/* Section 4.3.1. */
	ITCH41_MSG_STOCK_TRADING_ACTION		= 'H',	/* Section 4.3.2. */
	ITCH41_MSG_REG_SHO_RESTRICTION		= 'Y',	/* Section 4.3.3. */
	ITCH41_MSG_MARKET_PARTICIPANT_POS	= 'L',	/* Section 4.3.4. */
	ITCH41_MSG_ADD_ORDER			= 'A',	/* Section 4.4.1. */
	ITCH41_MSG_ADD_ORDER_MPID		= 'F',	/* Section 4.4.2. */
	ITCH41_MSG_ORDER_EXECUTED		= 'E',	/* Section 4.5.1. */
	ITCH41_MSG_ORDER_EXECUTED_WITH_PRICE	= 'C',	/* Section 4.5.2. */
	ITCH41_MSG_ORDER_CANCEL			= 'X',	/* Section 4.5.3. */
	ITCH41_MSG_ORDER_DELETE			= 'D',	/* Section 4.5.4. */
	ITCH41_MSG_ORDER_REPLACE		= 'U',	/* Section 4.5.5. */
	ITCH41_MSG_TRADE			= 'P',	/* Section 4.6.1. */
	ITCH41_MSG_CROSS_TRADE			= 'Q',	/* Section 4.6.2. */
	ITCH41_MSG_BROKEN_TRADE			= 'B',	/* Section 4.6.3. */
	ITCH41_MSG_NOII				= 'I',	/* Section 4.7. */
};

/*
 * System event codes:
 */
enum itch41_event_code {
	ITCH41_EVENT_START_OF_MESSAGES		= 'O',
	ITCH41_EVENT_START_OF_SYSTEM_HOURS	= 'S',
	ITCH41_EVENT_START_OF_MARKET_HOURS	= 'Q',
	ITCH41_EVENT_END_OF_MARKET_HOURS	= 'M',
	ITCH41_EVENT_END_OF_SYSTEM_HOURS	= 'E',
	ITCH41_EVENT_END_OF_MESSAGES		= 'C',
	ITCH41_EVENT_EMERGENCY_HALT		= 'A',
	ITCH41_EVENT_EMERGENCY_QUOTE_ONLY	= 'R',
	ITCH41_EVENT_EMERGENCY_RESUMPTION	= 'B',
};

/*
 * A data structure for ITCH messages.
 */
struct itch41_message {
	u8			MessageType;
};

/* ITCH41_MSG_TIMESTAMP_SECONDS */
struct itch41_msg_timestamp_seconds {
	u8			MessageType;
	be32			Second;	
} packed;

/* ITCH41_MSG_SYSTEM_EVENT */
struct itch41_msg_system_event {
	u8			MessageType;
	be32			Timestamp;
	char			EventCode;	/* ITCH41_EVENT_<code> */
} packed;

/* ITCH41_MSG_STOCK_DIRECTORY */
struct itch41_msg_stock_directory {
	u8			MessageType;
	be32			TimestampNanoseconds;
	char			Stock[8];
	char			MarketCategory;
	char			FinancialStatusIndicator;
	be32			RoundLotSize;
	char			RoundLotsOnly;
} packed;

/* ITCH41_MSG_STOCK_TRADING_ACTION */
struct itch41_msg_stock_trading_action {
	u8			MessageType;
	be32			TimestampNanoseconds;
	char			Stock[8];
	char			TradingState;
	char			Reserved;
	char			Reason[4];
} packed;

/* ITCH41_MSG_REG_SHO_RESTRICTION */
struct itch41_msg_reg_sho_restriction {
	u8			MessageType;
	be32			TimestampNanoseconds;
	char			Stock[8];
	char			RegSHOAction;
} packed;

/* ITCH41_MSG_MARKET_PARTICIPANT_POS */
struct itch41_msg_market_participant_pos {
	u8			MessageType;
	be32			TimestampNanoseconds;
	char			MPID[4];
	char			Stock[8];
	char			PrimaryMarketMaker;
	char			MarketMakerMode;
	char			MarketParticipantState;	
} packed;

/* ITCH41_MSG_ADD_ORDER */
struct itch41_msg_add_order {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
} packed;

/* ITCH41_MSG_ADD_ORDER_MPID */
struct itch41_msg_add_order_mpid {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
	char			Attribution[4];
} packed;

/* ITCH41_MSG_ORDER_EXECUTED */
struct itch41_msg_order_executed {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	be32			ExecutedShares;
	be64			MatchNumber;
} packed;

/* ITCH41_MSG_ORDER_EXECUTED_WITH_PRICE */
struct itch41_msg_order_executed_with_price {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	be32			ExecutedShares;
	be64			MatchNumber;
	char			Printable;
	be32			ExecutionPrice;
} packed;

/* ITCH41_MSG_ORDER_CANCEL */
struct itch41_msg_order_cancel {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	be32			CanceledShares;
} packed;

/* ITCH41_MSG_ORDER_DELETE */
struct itch41_msg_order_delete {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
} packed;

/* ITCH41_MSG_ORDER_REPLACE */
struct itch41_msg_order_replace {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OriginalOrderReferenceNumber;
	be64			NewOrderReferenceNumber;
	be32			Shares;
	be32			Price;
} packed;

/* ITCH41_MSG_TRADE */
struct itch41_msg_trade {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
	be64			MatchNumber;
} packed;

/* ITCH41_MSG_CROSS_TRADE */
struct itch41_msg_cross_trade {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			Shares;
	char			Stock[8];
	be32			CrossPrice;
	be64			MatchNumber;
	char			CrossType;
} packed;

/* ITCH41_MSG_BROKEN_TRADE */
struct itch41_msg_broken_trade {
	u8			MessageType;
	be32			TimestampNanoseconds;
	be64			MatchNumber;
} packed;

/* ITCH41_MSG_NOII */
struct itch41_msg_noii {
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

struct itch41_message *itch41_message_decode(struct buffer *buf);

#endif
