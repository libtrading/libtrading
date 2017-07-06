#ifndef LIBTRADING_NASDAQ_ITCH50_MESSAGE_H
#define LIBTRADING_NASDAQ_ITCH50_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/types.h"

struct buffer;

/*
 * Message types:
 */
enum itch50_msg_type {
	ITCH50_MSG_SYSTEM_EVENT			= 'S',	/* Section 4.1. */
	ITCH50_MSG_STOCK_DIRECTORY		= 'R',	/* Section 4.2.1. */
	ITCH50_MSG_STOCK_TRADING_ACTION		= 'H',	/* Section 4.2.2. */
	ITCH50_MSG_REG_SHO_RESTRICTION		= 'Y',	/* Section 4.2.3. */
	ITCH50_MSG_MARKET_PARTICIPANT_POS	= 'L',	/* Section 4.2.4. */
	ITCH50_MSG_MWCB_DECLINE_LEVEL		= 'V',	/* Section 4.2.5.1. */
	ITCH50_MSG_MWCB_STATUS			= 'W',	/* Section 4.2.5.2. */
	ITCH50_MSG_IPO_QUOTING_PERIOD_UPDATE	= 'K',	/* Section 4.2.6. */
	ITCH50_MSG_ADD_ORDER			= 'A',	/* Section 4.3.1. */
	ITCH50_MSG_ADD_ORDER_MPID		= 'F',	/* Section 4.3.2. */
	ITCH50_MSG_ORDER_EXECUTED		= 'E',	/* Section 4.4.1. */
	ITCH50_MSG_ORDER_EXECUTED_WITH_PRICE	= 'C',	/* Section 4.4.2. */
	ITCH50_MSG_ORDER_CANCEL			= 'X',	/* Section 4.4.3. */
	ITCH50_MSG_ORDER_DELETE			= 'D',	/* Section 4.4.4. */
	ITCH50_MSG_ORDER_REPLACE		= 'U',	/* Section 4.4.5. */
	ITCH50_MSG_TRADE			= 'P',	/* Section 4.5.1. */
	ITCH50_MSG_CROSS_TRADE			= 'Q',	/* Section 4.5.2. */
	ITCH50_MSG_BROKEN_TRADE			= 'B',	/* Section 4.5.3. */
	ITCH50_MSG_NOII				= 'I',	/* Section 4.6. */
	ITCH50_MSG_RPII				= 'N',	/* Section 4.7. */
};

/*
 * System event codes:
 */
enum itch50_event_code {
	ITCH50_EVENT_START_OF_MESSAGES		= 'O',
	ITCH50_EVENT_START_OF_SYSTEM_HOURS	= 'S',
	ITCH50_EVENT_START_OF_MARKET_HOURS	= 'Q',
	ITCH50_EVENT_END_OF_MARKET_HOURS	= 'M',
	ITCH50_EVENT_END_OF_SYSTEM_HOURS	= 'E',
	ITCH50_EVENT_END_OF_MESSAGES		= 'C',
	ITCH50_EVENT_EMERGENCY_HALT		= 'A',
	ITCH50_EVENT_EMERGENCY_QUOTE_ONLY	= 'R',
	ITCH50_EVENT_EMERGENCY_RESUMPTION	= 'B',
};

/*
 * A data structure for ITCH messages.
 */
struct itch50_message {
	u8			MessageType;
};

/* ITCH50_MSG_SYSTEM_EVENT */
struct itch50_msg_system_event {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	char			EventCode;	/* ITCH50_EVENT_<code> */
} __attribute__((packed));

/* ITCH50_MSG_STOCK_DIRECTORY */
struct itch50_msg_stock_directory {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	char			Stock[8];
	char			MarketCategory;
	char			FinancialStatusIndicator;
	be32			RoundLotSize;
	char			RoundLotsOnly;
	char			IssueClassification;
	char			IssueSubType[2];
	char			Authenticity;
	char			ShortSaleThresholdIndicator;
	char			IPOFlag;
	char			LULDReferencePriceTier;
	char			ETPFlag;
	be32			ETPLeverageFactor;
	char			InverseIndicator;
} __attribute__((packed));

/* ITCH50_MSG_STOCK_TRADING_ACTION */
struct itch50_msg_stock_trading_action {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	char			Stock[8];
	char			TradingState;
	char			Reserved;
	char			Reason[4];
} __attribute__((packed));

/* ITCH50_MSG_REG_SHO_RESTRICTION */
struct itch50_msg_reg_sho_restriction {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	char			Stock[8];
	char			RegSHOAction;
} __attribute__((packed));

/* ITCH50_MSG_MARKET_PARTICIPANT_POS */
struct itch50_msg_market_participant_pos {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	char			MPID[4];
	char			Stock[8];
	char			PrimaryMarketMaker;
	char			MarketMakerMode;
	char			MarketParticipantState;
} __attribute__((packed));

/* ITCH50_MSG_MWCB_DECLINE_LEVEL */
struct itch50_msg_mwcb_decline_level {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			Level1;
	be64			Level2;
	be64			Level3;
} __attribute__((packed));

/* ITCH50_MSG_MWCB_STATUS */
struct itch50_msg_mwcb_status {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	char			BreachedLevel;
} __attribute__((packed));

/* ITCH50_MSG_IPO_QUOTING_PERIOD_UPDATE */
struct itch50_msg_ipo_quoting_period_update {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	char			Stock[8];
	be32			IPOQuotationReleaseTime;
	char			IPOQuotationReleaseQualifier;
	be32			IPOPrice;
} __attribute__((packed));

/* ITCH50_MSG_ADD_ORDER */
struct itch50_msg_add_order {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
} __attribute__((packed));

/* ITCH50_MSG_ADD_ORDER_MPID */
struct itch50_msg_add_order_mpid {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
	char			Attribution[4];
} __attribute__((packed));

/* ITCH50_MSG_ORDER_EXECUTED */
struct itch50_msg_order_executed {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			OrderReferenceNumber;
	be32			ExecutedShares;
	be64			MatchNumber;
} __attribute__((packed));

/* ITCH50_MSG_ORDER_EXECUTED_WITH_PRICE */
struct itch50_msg_order_executed_with_price {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			OrderReferenceNumber;
	be32			ExecutedShares;
	be64			MatchNumber;
	char			Printable;
	be32			ExecutionPrice;
} __attribute__((packed));

/* ITCH50_MSG_ORDER_CANCEL */
struct itch50_msg_order_cancel {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			OrderReferenceNumber;
	be32			CanceledShares;
} __attribute__((packed));

/* ITCH50_MSG_ORDER_DELETE */
struct itch50_msg_order_delete {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			OrderReferenceNumber;
} __attribute__((packed));

/* ITCH50_MSG_ORDER_REPLACE */
struct itch50_msg_order_replace {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			OriginalOrderReferenceNumber;
	be64			NewOrderReferenceNumber;
	be32			Shares;
	be32			Price;
} __attribute__((packed));

/* ITCH50_MSG_TRADE */
struct itch50_msg_trade {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
	be64			MatchNumber;
} __attribute__((packed));

/* ITCH50_MSG_CROSS_TRADE */
struct itch50_msg_cross_trade {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			Shares;
	char			Stock[8];
	be32			CrossPrice;
	be64			MatchNumber;
	char			CrossType;
} __attribute__((packed));

/* ITCH50_MSG_BROKEN_TRADE */
struct itch50_msg_broken_trade {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			MatchNumber;
} __attribute__((packed));

/* ITCH50_MSG_NOII */
struct itch50_msg_noii {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	be64			PairedShares;
	be64			ImbalanceShares;
	char			ImbalanceDirection;
	char			Stock[8];
	be32			FarPrice;
	be32			NearPrice;
	be32			CurrentReferencePrice;
	char			CrossType;
	char			PriceVariationIndicator;
} __attribute__((packed));

/* ITCH50_MSG_RPII */
struct itch50_msg_rpii {
	u8			MessageType;
        be16			StockLocate;
        be16			TrackingNumber;
	u8			Timestamp[6];
	char			Stock[8];
	char			InterestFlag;
} __attribute__((packed));

struct itch50_message *itch50_message_decode(struct buffer *buf);

#ifdef __cplusplus
}
#endif

#endif
