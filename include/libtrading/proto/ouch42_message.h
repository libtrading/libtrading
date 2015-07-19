#ifndef LIBTRADING_OUCH42_MESSAGE_H
#define LIBTRADING_OUCH42_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/types.h"

struct buffer;

/*
 * Message types:
 */
enum ouch42_msg_type {
	OUCH42_MSG_ENTER_ORDER		= 'O',
	OUCH42_MSG_REPLACE_ORDER	= 'U',
	OUCH42_MSG_CANCEL_ORDER		= 'X',
	OUCH42_MSG_MODIFY_ORDER		= 'M',
	OUCH42_MSG_SYSTEM_EVENT		= 'S',
	OUCH42_MSG_ACCEPTED		= 'A',
	OUCH42_MSG_REPLACED		= 'U',
	OUCH42_MSG_CANCELED		= 'C',
	OUCH42_MSG_AIQ_CANCELED		= 'D',
	OUCH42_MSG_EXECUTED		= 'E',
	OUCH42_MSG_BROKEN_TRADE		= 'B',
	OUCH42_MSG_REJECTED		= 'J',
	OUCH42_MSG_CANCEL_PENDING	= 'P',
	OUCH42_MSG_CANCEL_REJECT	= 'I',
	OUCH42_MSG_ORDER_PRIO_UPDATE	= 'T',
	OUCH42_MSG_ORDER_MODIFIED	= 'M',
};

struct ouch42_message {
	char			MessageType;
} packed;

struct ouch42_msg_enter_order {
	char			MessageType;
	char			OrderToken[14];
	char			BuySellIndicator;
	u32			Shares;
	char			Stock[8];
	u32			Price;
	u32			TimeInForce;
	char			Firm[4];
	char			Display;
	char			Capacity;
	char			IntermarketSweepEligibility;
	u32			MinimumQuantity;
	char			CrossType;
} packed;

struct ouch42_msg_replace_order {
	u8			MessageType;
	char			ExistingOrderToken[14];
	char			ReplacementOrderToken[14];
	u32			Shares;
	u32			Price;
	u32			TimeInForce;
	char			Display;
	char			IntermarketSweepEligibility;
	u32			MinimumQuantity;
} packed;

struct ouch42_msg_cancel_order {
	u8			MessageType;
	char			OrderToken[14];
	u32			Shares;
} packed;

struct ouch42_msg_modify_order {
	u8			MessageType;
	char			OrderToken[14];
	char			BuySellIndicator;
	u32			Shares;
} packed;

struct ouch42_msg_system_event {
	u8			MessageType;
	u64			Timestamp;
	char			EventCode;
} packed;

struct ouch42_msg_accepted {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
	char			BuySellIndicator;
	u32			Shares;
	char			Stock[8];
	u32			Price;
	u32			TimeInForce;
	char			Firm[4];
	char			Display;
	u64			OrderReferenceNumber;
	char			Capacity;
	char			IntermarketSweepEligibility;
	u32			MinimumQuantity;
	char			CrossType;
	char			OrderState;
	char			BBOWeightIndicator;
} packed;

struct ouch42_msg_replaced {
	u8			MessageType;
	u64			Timestamp;
	char			ReplacementOrderToken[14];
	char			BuySellIndicator;
	u32			Shares;
	char			Stock[8];
	u32			Price;
	u32			TimeInForce;
	char			Firm[4];
	char			Display;
	u64			OrderReferenceNumber;
	char			Capacity;
	char			IntermarketSweepEligibility;
	u32			MinimumQuantity;
	char			CrossType;
	char			OrderState;
	char			PreviousOrderToken[14];
	char			BBOWeightIndicator;
} packed;

struct ouch42_msg_canceled {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
	u32			DecrementShares;
	char			Reason;
} packed;

struct ouch42_msg_aiq_canceled {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
	u32			DecrementShares;
	char			Reason;
	u32			QuantityPreventedFromTrading;
	u32			ExecutionPrice;
	char			LiquidityFlag;
} packed;

struct ouch42_msg_executed {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
	u32			ExecutedShares;
	u32			ExecutionPrice;
	char			LiquidityFlag;
	u64			MatchNumber;
} packed;

struct ouch42_msg_broken_trade {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
	u64			MatchNumber;
	char			Reason;
} packed;

struct ouch42_msg_rejected {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
	char			Reason;
} packed;

struct ouch42_msg_cancel_pending {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
} packed;

struct ouch42_msg_cancel_reject {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
} packed;

struct ouch42_msg_order_prio_update {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
	u32			Price;
	char			Display;
	u64			OrderReferenceNumber;
} packed;

struct ouch42_msg_order_modified {
	u8			MessageType;
	u64			Timestamp;
	char			OrderToken[14];
	char			BuySellIndicator;
	u32			Shares;
} packed;

int ouch42_in_message_decode(struct buffer *buf, struct ouch42_message *msg);
int ouch42_out_message_decode(struct buffer *buf, struct ouch42_message *msg);

#ifdef __cplusplus
}
#endif

#endif
