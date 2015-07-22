#ifndef LIBTRADING_LSE_ITCH_MESSAGE_H
#define LIBTRADING_LSE_ITCH_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/types.h"

struct buffer;

/*
 * Message types:
 */
enum lse_itch_msg_type {
	/*
	 * Administrative messages:
	 */
	LSE_ITCH_MSG_LOGIN_REQUEST			= 0x01,
	LSE_ITCH_MSG_LOGIN_RESPONSE			= 0x02,
	LSE_ITCH_MSG_LOGOUT_REQUEST			= 0x05,
	LSE_ITCH_MSG_REPLAY_REQUEST			= 0x03,
	LSE_ITCH_MSG_REPLAY_RESPONSE			= 0x04,
	LSE_ITCH_MSG_SNAPSHOT_REQUEST			= 0x81,
	LSE_ITCH_MSG_SNAPSHOT_RESPONSE			= 0x82,
	LSE_ITCH_MSG_SNAPSHOT_COMPLETE			= 0x83,

	/*
	 * Application messages:
	 */
	LSE_ITCH_MSG_TIME				= 0x54,
	LSE_ITCH_MSG_SYSTEM_EVENT			= 0x53,
	LSE_ITCH_MSG_SYMBOL_DIRECTORY			= 0x52,
	LSE_ITCH_MSG_SYMBOL_STATUS			= 0x48,
	LSE_ITCH_MSG_ADD_ORDER				= 0x41,
	LSE_ITCH_MSG_ADD_ATTIRUBTED_ORDER		= 0x46,
	LSE_ITCH_MSG_ORDER_DELETED			= 0x44,
	LSE_ITCH_MSG_ORDER_MODIFIED			= 0x55,
	LSE_ITCH_MSG_ORDER_BOOK_CLEAR			= 0x79,
	LSE_ITCH_MSG_ORDER_EXECUTED			= 0x45,
	LSE_ITCH_MSG_ORDER_EXECUTED_WITH_PRICE_SIZE	= 0x43,
	LSE_ITCH_MSG_TRADE				= 0x50,
	LSE_ITCH_MSG_AUCTION_TRADE			= 0x51,
	LSE_ITCH_MSG_OFF_BOOK_TRADE			= 0x78,
	LSE_ITCH_MSG_TRADE_BREAK			= 0x42,
	LSE_ITCH_MSG_AUCTION_INFO			= 0x49,
	LSE_ITCH_MSG_STATISTICS				= 0x77,
};

enum lse_itch_login_status_type {
	LSE_ITCH_LOGIN_STATUS_ACCEPTED			= 'A',
	LSE_ITCH_LOGIN_STATUS_INVALID_PASSWORD		= 'N',
	LSE_ITCH_LOGIN_STATUS_COMPID_INACTIVE_LOCKED	= 'a',
	LSE_ITCH_LOGIN_STATUS_LOGIN_LIMIT_REACHED	= 'b',
	LSE_ITCH_LOGIN_STATUS_SERVICE_UNAVAILABLE	= 'c',
	LSE_ITCH_LOGIN_STATUS_CONCURRENT_LIMIT_REACHED	= 'd',
	LSE_ITCH_LOGIN_STATUS_FAILED			= 'e',
};

enum lse_itch_replay_status_type {
	LSE_ITCH_REPLAY_STATUS_REQUEST_ACCEPTED		= 'A',
	LSE_ITCH_REPLAY_STATUS_REQUEST_LIMIT_REACHED	= 'D',
	LSE_ITCH_REPLAY_STATUS_INVALID_MARKET_DATA_GROUP= 'I',
	LSE_ITCH_REPLAY_STATUS_OUT_OF_RANGE		= 'O',
	LSE_ITCH_REPLAY_STATUS_REPLAY_UNAVAILABLE	= 'U',
	LSE_ITCH_REPLAY_STATUS_UNSUPPORTED_MESSAGE_TYPE	= 'd',
	LSE_ITCH_REPLAY_STATUS_FAILED			= 'e',
	LSE_ITCH_REPLAY_STATUS_CONCURRENT_LIMIT_REACHED	= 'c',
};

enum lse_itch_snapshot_status_type {
	LSE_ITCH_SNAPSHOT_STATUS_REQUEST_ACCEPTED		= 'A',
	LSE_ITCH_SNAPSHOT_STATUS_OUT_OF_RANGE			= 'O',
	LSE_ITCH_SNAPSHOT_STATUS_SNAPSHOT_UNAVAILABLE		= 'U',
	LSE_ITCH_SNAPSHOT_STATUS_VALID_SEGMENT_OR_SYMBOL_NOT_SPECIFIED = 'a',
	LSE_ITCH_SNAPSHOT_STATUS_REQUEST_LIMIT_REACHED		= 'b',
	LSE_ITCH_SNAPSHOT_STATUS_CONCURRENT_LIMIT_REACHED	= 'c',
	LSE_ITCH_SNAPSHOT_STATUS_UNSUPPORTED_MESSAGE_TYPE	= 'd',
	LSE_ITCH_SNAPSHOT_STATUS_FAILED				= 'e',
};

enum lse_itch_snapshot_complete_flags {
	LSE_ITCH_SNAPSHOT_COMPLETE_FLAG_QUOTE		= 1U << 5,
};

/*
 * Section 4.6. ("Unit Header")
 */
struct lse_itch_unit_header {
	le16			Length;
	u8			MessageCount;
	u8			MarketDataGroup;
	le32			SequenceNumber;
};

struct lse_itch_message {
	u8			Length;
	u8			MessageType;
};

/* LSE_ITCH_MSG_LOGIN_REQUEST */
struct lse_itch_login_request {
	u8			Length;
	u8			MessageType;
	char			Username[6];
	char			Password[8];
} __attribute__((packed));

/* LSE_ITCH_MSG_REPLAY_REQUEST */
struct lse_itch_replay_request {
	u8			Length;
	u8			MessageType;
	u8			MarketDataGroup;
	le32			FirstMessage;
	le16			Count;
} __attribute__((packed));

/* LSE_ITCH_MSG_SNAPSHOT_REQUEST */
struct lse_itch_snapshot_request {
	u8			Length;
	u8			MessageType;
	le32			SequenceNumber;
	char			Segment[6];
	le32			InstrumentID;
} __attribute__((packed));

/* LSE_ITCH_MSG_LOGOUT_REQUEST */
struct lse_itch_logout_request {
	u8			Length;
	u8			MessageType;
} __attribute__((packed));

/* LSE_ITCH_MSG_LOGIN_RESPONSE */
struct lse_itch_logout_response {
	u8			Length;
	u8			MessageType;
	u8			Status;		/* See LSE_ITCH_LOGIN_STATUS enum */
} __attribute__((packed));

/* LSE_ITCH_MSG_REPLAY_RESPONSE */
struct lse_itch_replay_response {
	u8			Length;
	u8			MessageType;
	u8			MarketDataGroup;
	le32			FirstMessage;
	le16			Count;
	u8			Status;		/* See LSE_ITCH_REPLAY_STATUS enum */
} __attribute__((packed));

/* LSE_ITCH_MSG_SNAPSHOT_RESPONSE */
struct lse_itch_snapshot_response {
	u8			Length;
	u8			MessageType;
	le32			SequenceNumber;
	le32			OrderCount;
	u8			Status;		/* See LSE_ITCH_SNAPSHOT_STATUS enum */
} __attribute__((packed));

/* LSE_ITCH_MSG_SNAPSHOT_COMPLETE */
struct lse_itch_snapshot_complete {
	u8			Length;
	u8			MessageType;
	le32			SequenceNumber;
	char			Segment[6];
	le32			InstrumentID;
	u8			Flags;
} __attribute__((packed));

int lse_itch_message_decode(struct buffer *buf, struct lse_itch_message *msg);

#ifdef __cplusplus
}
#endif

#endif
