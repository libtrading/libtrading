#ifndef LIBTRADING_BOE_MESSAGE_H
#define LIBTRADING_BOE_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libtrading/types.h"

#include <stddef.h>

struct buffer;

/*
 * Maximum number of bytes for a 'struct boe_message' - i.e. number of bytes
 * that is big enough to accommodate largest possible BOE message.
 */
#define BOE_MAX_MESSAGE_LEN	183

/*
 * Start of message bytes:
 */
#define BOE_MAGIC		0xBABA

/*
 * Message types:
 */
enum {
	/*
	 * Participant to BATS:
	 */
	LoginRequest			= 0x01,
	LogoutRequest			= 0x02,
	ClientHeartbeat			= 0x03,
	NewOrder			= 0x04,
	CancelOrder			= 0x05,
	ModifyOrder			= 0x06,

	/*
	 * BATS to Participant:
	 */
	LoginResponse			= 0x07,
	Logout				= 0x08,
	ServerHeartbeat			= 0x09,
	ReplayComplete			= 0x13,
	OrderAcknowledgement		= 0x0A,
	OrderRejected			= 0x0B,
	OrderModified			= 0x0C,
	OrderRestarted			= 0x0D,
	UserModifyRejected		= 0x0E,
	OrderCancelled			= 0x0F,
	CancelRejected			= 0x10,
	OrderExecution			= 0x11,
	TradeCancelOrCorrect		= 0x12,
};

struct boe_header {
	le16				StartOfMessage;
	le16				MessageLength;
	u8				MessageType;
	u8				MatchingUnit;
	le32				SequenceNumber;
} packed;

struct boe_message {
	struct boe_header		header;
};

/*
 * Return bitfields:
 */

#define ReturnBitfield1 		0

#define Side				(1ULL << (ReturnBitfield1 + 0))
#define PegDifference			(1ULL << (ReturnBitfield1 + 1))
#define Price				(1ULL << (ReturnBitfield1 + 2))
#define ExecInst			(1ULL << (ReturnBitfield1 + 3))
#define OrdType				(1ULL << (ReturnBitfield1 + 4))
#define TimeInForce			(1ULL << (ReturnBitfield1 + 5))
#define MinQty				(1ULL << (ReturnBitfield1 + 6))
#define MaxRemovePct			(1ULL << (ReturnBitfield1 + 7))

#define ReturnBitfield2 		8

#define Symbol				(1ULL << (ReturnBitfield2 + 0))
#define Currency			(1ULL << (ReturnBitfield2 + 2))
#define IDSource			(1ULL << (ReturnBitfield2 + 3))
#define SecurityID			(1ULL << (ReturnBitfield2 + 4))
#define SecurityExchange		(1ULL << (ReturnBitfield2 + 5))
#define Capacity			(1ULL << (ReturnBitfield2 + 6))
#define CrossFlag			(1ULL << (ReturnBitfield2 + 7))

#define ReturnBitfield3			16

#define Account				(1ULL << (ReturnBitfield3 + 0))
#define ClearingFirm			(1ULL << (ReturnBitfield3 + 1))
#define ClearingAccount			(1ULL << (ReturnBitfield3 + 2))
#define DisplayIndicator		(1ULL << (ReturnBitfield3 + 3))
#define MaxFloor			(1ULL << (ReturnBitfield3 + 4))
#define OrderQty			(1ULL << (ReturnBitfield3 + 5))
#define PreventParticipantMatch		(1ULL << (ReturnBitfield3 + 7))

/* ReturnBitField4 - Reserved For Future Use */

#define ReturnBitField5			32

#define OrigClOrdID			(1ULL << (ReturnBitfield5 + 0))
#define LeavesQty			(1ULL << (ReturnBitfield5 + 1))
#define LastShares			(1ULL << (ReturnBitfield5 + 2))
#define LastPx				(1ULL << (ReturnBitfield5 + 3))
#define DisplayPrice			(1ULL << (ReturnBitfield5 + 4))
#define WorkingPrice			(1ULL << (ReturnBitfield5 + 5))
#define BaseLiquidityIndicator		(1ULL << (ReturnBitfield5 + 6))
#define ExpireTime			(1ULL << (ReturnBitfield5 + 7))

/* ReturnBitField6 - Reserved For Future Use */

/* ReturnBitField7 - Reserved For Future Use */

/* ReturnBitField8 - Reserved For Future Use */

/*
 * Participant to BATS messages:
 */

struct boe_unit {
	u8				UnitNumber;
	le32				UnitSequence;
} packed;

struct boe_login_request {
	char				SessionSubID[4];
	char				Username[4];
	char				Password[10];
	u8				NoUnspecifiedUnitReplay;
	le64				OrderAcknowledgementBitfields;
	le64				OrderRejectedBitfields;
	le64				OrderModifiedBitfields;
	le64				OrderRestatedBitfields;
	le64				UserModifyRejectedBitfields;
	le64				OrderCancelledBitfields;
	le64				CancelRejectedBitfields;
	le64				OrderExecutionBitfields;
	le64				TradeCancelOrCorrectBitfields;
	le64				ReservedBitfields1;
	le64				ReservedBitfields2;
	u8				NumberOfUnits;
	struct boe_unit			Units[];
} packed;

/*
 * BATS to participant messages:
 */

struct boe_login_response {
	u8				LoginResponseStatus;
	char				LoginResponseText[60];
	u8				NoUnspecifiedUnitReplay;
	le64				OrderAcknowledgementBitfields;
	le64				OrderRejectedBitfields;
	le64				OrderModifiedBitfields;
	le64				OrderRestatedBitfields;
	le64				UserModifyRejectedBitfields;
	le64				OrderCancelledBitfields;
	le64				CancelRejectedBitfields;
	le64				OrderExecutionBitfields;
	le64				TradeCancelOrCorrectBitfields;
	le64				ReservedBitfields1;
	le64				ReservedBitfields2;
	le32				LastReceivedSequenceNumber;
	u8				NumberOfUnits;
	struct boe_unit			Units[];
} packed;

struct boe_logout {
	u8				LogoutReason;
	char				LogoutReasonText[60];
	le32				LastReceivedSequenceNumber;
	u8				NumberOfUnits;
	struct boe_unit			Units[];
} packed;

int boe_message_decode(struct buffer *buf, struct boe_message *msg, size_t size);

static inline void *boe_message_payload(struct boe_message *msg)
{
	return (void *) msg + sizeof(struct boe_header);
}

#ifdef __cplusplus
}
#endif

#endif
