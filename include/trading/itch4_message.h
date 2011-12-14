#ifndef LIBTRADING_ITCH_MESSAGE_H
#define LIBTRADING_ITCH_MESSAGE_H

struct buffer;

/** @file */

/*
 *	ITCH 4.1 / SOUP BIN TCP protocol implementation
 */

#include "trading/types.h"

/*
 * 	SOUP BIN TCP Packet Types
 */
enum soup_packet_type {
	SOUP_PACKET_DEBUG			= '+', /* Section 2.1 */

	/* Section 2.2. - Packets Sent by the Server */
	SOUP_PACKET_LOGIN_ACCEPTED		= 'A', /* Section 2.2.1. */
	SOUP_PACKET_LOGIN_REJECTED		= 'J', /* Section 2.2.2. */
	SOUP_PACKET_SEQ_DATA		= 'S', /* Section 2.2.3. */
	SOUP_PACKET_SERVER_HEARTBEAT		= 'H', /* Section 2.2.4. */
	SOUP_PACKET_END_OF_SESSION		= 'Z', /* Section 2.2.5. */

	/* Section 2.3. - Packets Sent by the Client */
	SOUP_PACKET_LOGIN_REQUEST		= 'L', /* Section 2.3.1. */
	SOUP_PACKET_UNSEQ_DATA			= 'U', /* Section 2.3.2. */
	SOUP_PACKET_CLIENT_HEARTBEAT		= 'R', /* Section 2.3.3. */
	SOUP_PACKET_LOGOUT_REQUEST		= 'O', /* Section 2.3.4. */
};

struct soup_packet {
	be16			PacketLength;
	char			PacketType;	/* enum soup_packet_type */
};

struct soup_packet_debug {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_DEBUG */
	char			Text[];
} packed;

struct soup_packet_login_accepted {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_LOGIN_ACCEPTED */
	char			Session[10];
	char			SequenceNumber[20];
} packed;

struct soup_packet_login_rejected {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_LOGIN_REJECTED */
	char			RejectReasonCode;
} packed;

struct soup_packet_seq_data {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_SEQ_DATA */
	char			Message[];
} packed;

struct soup_packet_server_heartbeat {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_SERVER_HEARTBEAT */
} packed;

struct soup_packet_end_of_session {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_END_OF_SESSION */
} packed;

struct soup_packet_login_request {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_LOGIN_REQUEST */
	char			Username[6];
	char			Password[10];
	char			RequestedSession[10];
	char			RequestedSequenceNumber[20];
} packed;

struct soup_packet_unseq_data {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_UNSEQ_DATA */
	char			Message[];
} packed;

struct soup_packet_client_heartbeat {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_CLIENT_HEARTBEAT */
} packed;

struct soup_packet_logout_request {
	be16			PacketLength;
	char			PacketType;	/* SOUP_PACKET_LOGOUT_REQUEST */
} packed;

/*
 *	ITCH 4.1 Message Types
 */
enum itch4_msg_type {
	ITCH_MSG_TIMESTAMP_SECONDS		= 'T',	/* Section 4.1 */
	ITCH_MSG_SYSTEM_EVENT			= 'S',	/* Section 4.2 */
	ITCH_MSG_STOCK_DIRECTORY		= 'R',	/* Section 4.3.1. */
	ITCH_MSG_STOCK_TRADING_ACTION		= 'H',	/* Section 4.3.2. */
	ITCH_MSG_REG_SHO_RESTRICTION		= 'Y',	/* Section 4.3.3. */
	ITCH_MSG_MARKET_PARTICIPANT_POS		= 'L',	/* Section 4.3.4. */
	ITCH_MSG_ADD_ORDER			= 'A',	/* Section 4.4.1. */
	ITCH_MSG_ADD_ORDER_MPID			= 'F',	/* Section 4.4.2. */
	ITCH_MSG_ORDER_EXECUTED			= 'E',	/* Section 4.5.1. */
	ITCH_MSG_ORDER_EXECUTED_WITH_PRICE	= 'C',	/* Section 4.5.2. */
	ITCH_MSG_ORDER_CANCEL			= 'X',	/* Section 4.5.3. */
	ITCH_MSG_ORDER_DELETE			= 'D',	/* Section 4.5.4. */
	ITCH_MSG_ORDER_REPLACE			= 'U',	/* Section 4.5.5. */
	ITCH_MSG_TRADE				= 'P',	/* Section 4.6.1. */
	ITCH_MSG_CROSS_TRADE			= 'Q',	/* Section 4.6.2. */
	ITCH_MSG_BROKEN_TRADE			= 'B',	/* Section 4.6.3. */
	ITCH_MSG_NOII				= 'I',	/* Section 4.7. */
};

/*
 * 	ITCH 4.1 System Event Codes
 */
enum itch4_event_code {
	/* Daily */
	ITCH_EVENT_START_OF_MESSAGES		= 'O',
	ITCH_EVENT_START_OF_SYSTEM_HOURS	= 'S',
	ITCH_EVENT_START_OF_MARKET_HOURS	= 'Q',
	ITCH_EVENT_END_OF_MARKET_HOURS		= 'M',
	ITCH_EVENT_END_OF_SYSTEM_HOURS		= 'E',
	ITCH_EVENT_END_OF_MESSAGES		= 'C',

	/* As needed */
	ITCH_EVENT_EMERGENCY_HALT		= 'A',
	ITCH_EVENT_EMERGENCY_QUOTE_ONLY		= 'R',
	ITCH_EVENT_EMERGENCY_RESUMPTION		= 'B',
};

struct itch4_message {
	u8			MessageType;
};

struct itch4_msg_timestamp_seconds {
	u8			MessageType;	/* ITCH_MSG_TIMESTAMP_SECONDS */
	be32			Second;	
} packed;

struct itch4_msg_system_event {
	u8			MessageType;	/* ITCH_MSG_SYSTEM_EVENT */
	be32			Timestamp;
	char			EventCode;	/* enum itch4_system_event_code */
} packed;

struct itch4_msg_stock_directory {
	u8			MessageType;	/* ITCH_MSG_STOCK_DIRECTORY */
	be32			TimestampNanoseconds;
	char			Stock[8];
	char			MarketCategory;
	char			FinancialStatusIndicator;
	be32			RoundLotSize;
	char			RoundLotsOnly;
} packed;

struct itch4_msg_stock_trading_action {
	u8			MessageType;	/* ITCH_MSG_STOCK_TRADING_ACTION */
	be32			TimestampNanoseconds;
	char			Stock[8];
	char			TradingState;
	char			Reserved;
	char			Reason[4];
} packed;

struct itch4_msg_reg_sho_restriction {
	u8			MessageType;	/* ITCH_MSG_REG_SHO_RESTRICTION */
	be32			TimestampNanoseconds;
	char			Stock[8];
	char			RegSHOAction;
} packed;

struct itch4_msg_market_participant_pos {
	u8			MessageType;	/* ITCH_MSG_MARKET_PARTICIPANT_POS */
	be32			TimestampNanoseconds;
	char			MPID[4];
	char			Stock[8];
	char			PrimaryMarketMaker;
	char			MarketMakerMode;
	char			MarketParticipantState;	
} packed;

struct itch4_msg_add_order {
	u8			MessageType;	/* ITCH_MSG_ADD_ORDER */
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
} packed;

struct itch4_msg_add_order_mpid {
	u8			MessageType;	/* ITCH_MSG_ADD_ORDER_MPID */
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
	char			Attribution[4];
} packed;

struct itch4_msg_order_executed {
	u8			MessageType;	/* ITCH_MSG_ORDER_EXECUTED */
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	be32			ExecutedShares;
	be64			MatchNumber;
} packed;

struct itch4_msg_order_executed_with_price {
	u8			MessageType;	/* ITCH_MSG_ORDER_EXECUTED_WITH_PRICE */
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	be32			ExecutedShares;
	be64			MatchNumber;
	char			Printable;
	be32			ExecutionPrice;
} packed;

struct itch4_msg_order_cancel {
	u8			MessageType;	/* ITCH_MSG_ORDER_CANCEL */
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	be32			CanceledShares;
} packed;

struct itch4_msg_order_delete {
	u8			MessageType;	/* ITCH_MSG_ORDER_DELETE */
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
} packed;

struct itch4_msg_order_replace {
	u8			MessageType;	/* ITCH_MSG_ORDER_REPLACE */
	be32			TimestampNanoseconds;
	be64			OriginalOrderReferenceNumber;
	be64			NewOrderReferenceNumber;
	be32			Shares;
	be32			Price;
} packed;

struct itch4_msg_trade {
	u8			MessageType;	/* ITCH_MSG_TRADE */
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			Price;
	be64			MatchNumber;
} packed;

struct itch4_msg_cross_trade {
	u8			MessageType;	/* ITCH_MSG_CROSS_TRADE */
	be32			TimestampNanoseconds;
	be64			OrderReferenceNumber;
	char			BuySellIndicator;
	be32			Shares;
	char			Stock[8];
	be32			CrossPrice;
	be64			MatchNumber;
	char			CrossType;
} packed;

struct itch4_msg_broken_trade {
	u8			MessageType;	/* ITCH_MSG_BROKEN_TRADE */
	be32			TimestampNanoseconds;
	be64			MatchNumber;
} packed;

struct itch4_msg_noii {
	u8			MessageType;	/* ITCH_MSG_NOII */
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

/**
 * Decode an ITCH message.
 *
 * This function decodes.
 *
 * @param buf buffer to decode message from
 * @return pointer to struct itch4_message if successful, or NULL if an error occurred
 * @note If this function returns a non-NULL pointer, the message is consumed
 * from the buffer.
 */
struct itch4_message *itch4_message_decode(struct buffer *buf);

#endif /* LIBTRADING_ITCH_MESSAGE_H */
