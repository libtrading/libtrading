#ifndef FIX__BOE_H
#define FIX__BOE_H

#include <stdint.h>

struct buffer;

#define BOE_MAGIC 0xBABA

/*
 *	List of Message Types
 */

enum {
	/* Participant to BATS.  */
	LoginRequest			= 0x01,
	LogoutRequest			= 0x02,
	ClientHeartbeat			= 0x03,
	NewOrder			= 0x04,
	CancelOrder			= 0x05,
	ModifyOrder			= 0x06,

	/* BATS to Participant.  */
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

#define packed				__attribute__ ((packed))

struct boe_header {
	uint16_t			StartOfMessage;
	uint16_t			MessageLength;
	uint8_t				MessageType;
	uint8_t				MatchingUnit;
	uint32_t			SequenceNumber;
} packed;

/*
 *	List of Return Bitfields
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

#define BOE_SESSION_SUB_ID_LEN		4
#define BOE_USERNAME_LEN		4
#define BOE_PASSWORD_LEN		10

struct boe_login_request_unit {
	uint8_t				UnitNumber;
	uint32_t			UnitSequence;
} packed;

struct boe_login_request {
	char				SessionSubID[BOE_SESSION_SUB_ID_LEN];
	char				Username[BOE_USERNAME_LEN];
	char				Password[BOE_PASSWORD_LEN];
	uint8_t				NoUnspecifiedUnitReplay;
	uint64_t			OrderAcknowledgementBitfields;
	uint64_t			OrderRejectedBitfields;
	uint64_t			OrderModifiedBitfields;
	uint64_t			OrderRestatedBitfields;
	uint64_t			UserModifyRejectedBitfields;
	uint64_t			OrderCancelledBitfields;
	uint64_t			CancelRejectedBitfields;
	uint64_t			OrderExecutionBitfields;
	uint64_t			TradeCancelOrCorrectBitfields;
	uint64_t			ReservedBitfields1;
	uint64_t			ReservedBitfields2;
	uint8_t				NumberOfUnits;
	struct boe_login_request_unit	Units[];
} packed;

int boe_decode_header(struct buffer *buf, struct boe_header *header);
struct boe_login_request *boe_decode_login_request(struct boe_header *header, struct buffer *buf);

#endif /* FIX__BOE_H */
