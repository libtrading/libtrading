#ifndef LIBTRADING_FIX_MESSAGE_H
#define LIBTRADING_FIX_MESSAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

struct buffer;

struct fix_dialect;

/*
 * Message types:
 */
enum fix_msg_type {
	FIX_MSG_TYPE_HEARTBEAT			= 0,
	FIX_MSG_TYPE_TEST_REQUEST		= 1,
	FIX_MSG_TYPE_RESEND_REQUEST		= 2,
	FIX_MSG_TYPE_REJECT			= 3,
	FIX_MSG_TYPE_SEQUENCE_RESET		= 4,
	FIX_MSG_TYPE_LOGOUT			= 5,
	FIX_MSG_TYPE_EXECUTION_REPORT		= 6,
	FIX_MSG_TYPE_LOGON			= 7,
	FIX_MSG_TYPE_NEW_ORDER_SINGLE		= 8,
	FIX_MSG_TYPE_SNAPSHOT_REFRESH		= 9,
	FIX_MSG_TYPE_INCREMENT_REFRESH		= 10,
	FIX_MSG_TYPE_SESSION_STATUS		= 11,
	FIX_MSG_TYPE_SECURITY_STATUS		= 12,
	FIX_MSG_ORDER_CANCEL_REPLACE		= 13,
	FIX_MSG_ORDER_CANCEL_REJECT		= 14,
	FIX_MSG_ORDER_CANCEL_REQUEST		= 15,
	FIX_MSG_ORDER_MASS_CANCEL_REQUEST	= 16,
	FIX_MSG_ORDER_MASS_CANCEL_REPORT	= 17,

	FIX_MSG_QUOTE_REQUEST			= 18,
	FIX_MSG_SECURITY_DEFINITION_REQUEST	= 19,
	FIX_MSG_NEW_ORDER_CROSS			= 20,
	FIX_MSG_MASS_QUOTE			= 21,
	FIX_MSG_QUOTE_CANCEL			= 22,
	FIX_MSG_SECURITY_DEFINITION		= 23,
	FIX_MSG_QUOTE_ACKNOWLEDGEMENT		= 24,
	FIX_MSG_ORDER_MASS_STATUS_REQUEST	= 25,
	FIX_MSG_ORDER_MASS_ACTION_REQUEST	= 26,
	FIX_MSG_ORDER_MASS_ACTION_REPORT	= 27,

	FIX_MSG_TYPE_MAX,		/* non-API */

	FIX_MSG_TYPE_UNKNOWN		= ~0UL,
};

/*
 * Maximum FIX message size
 */
#define FIX_MAX_HEAD_LEN	256UL
#define FIX_MAX_BODY_LEN	1024UL
#define FIX_MAX_MESSAGE_SIZE	(FIX_MAX_HEAD_LEN + FIX_MAX_BODY_LEN)

/* Total number of elements of fix_tag type*/
#define FIX_MAX_FIELD_NUMBER	32

#define	FIX_MSG_STATE_PARTIAL	1
#define	FIX_MSG_STATE_GARBLED	2

enum fix_type {
	FIX_TYPE_INT,
	FIX_TYPE_FLOAT,
	FIX_TYPE_CHAR,
	FIX_TYPE_STRING,
	FIX_TYPE_CHECKSUM,
	FIX_TYPE_MSGSEQNUM,
};

enum fix_tag {
	Account			= 1,
	AvgPx			= 6,
	BeginSeqNo		= 7,
	BeginString		= 8,
	BodyLength		= 9,
	CheckSum		= 10,
	ClOrdID			= 11,
	CumQty			= 14,
	EndSeqNo		= 16,
	ExecID			= 17,
	ExecTransType		= 20,
	LastPx			= 31,
	LastShares		= 32,
	MsgSeqNum		= 34,
	MsgType			= 35,
	NewSeqNo		= 36,
	OrderID			= 37,
	OrderQty		= 38,
	OrdStatus		= 39,
	OrdType			= 40,
	OrigClOrdID		= 41,
	PossDupFlag		= 43,
	Price			= 44,
	RefSeqNum		= 45,
	SecurityID		= 48,
	SenderCompID		= 49,
	SendingTime		= 52,
	Side			= 54,
	Symbol			= 55,
	TargetCompID		= 56,
	Text			= 58,
	TransactTime		= 60,
	RptSeq			= 83,
	EncryptMethod		= 98,
	HeartBtInt		= 108,
	TestReqID		= 112,
	GapFillFlag		= 123,
	ResetSeqNumFlag		= 141,
	ExecType		= 150,
	LeavesQty		= 151,
	MDEntryType		= 269,
	MDEntryPx		= 270,
	MDEntrySize		= 271,
	MDUpdateAction		= 279,
	TradingSessionID	= 336,
	LastMsgSeqNumProcessed	= 369,
	Password		= 554,
	MDPriceLevel		= 1023,
};

struct fix_field {
	int				tag;
	enum fix_type			type;

	unsigned long			buf_off;
	int				fixed_len;

	union {
		int64_t			int_value;
		double			float_value;
		char			char_value;
		const char		*string_value;
	};
};

#define FIX_INT_FIELD(t, v)				\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_INT,		\
		.buf_off	= -1,			\
		.fixed_len	= -1,			\
		{ .int_value	= v },			\
	}

#define FIX_INT_FIXED_FIELD(t, v, l)			\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_INT,		\
		.buf_off	= -1,			\
		.fixed_len	= l,			\
		{ .int_value	= v },			\
	}

#define FIX_STRING_FIELD(t, s)				\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_STRING,	\
		.buf_off	= -1,			\
		.fixed_len	= -1,			\
		{ .string_value	= s },			\
	}

#define FIX_STRING_FIXED_FIELD(t, s, l)			\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_STRING,	\
		.buf_off	= -1,			\
		.fixed_len	= l,			\
		{ .string_value	= s },			\
	}

#define FIX_FLOAT_FIELD(t, v)				\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_FLOAT,	\
		.buf_off	= -1,			\
		.fixed_len	= -1,			\
		{ .float_value  = v },			\
	}

#define FIX_FLOAT_FIXED_FIELD(t, v, l)			\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_FLOAT,	\
		.buf_off	= -1,			\
		.fixed_len	= l,			\
		{ .float_value  = v },			\
	}

#define FIX_CHECKSUM_FIELD(t, v)			\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_CHECKSUM,	\
		.buf_off	= -1,			\
		.fixed_len	= 3,			\
		{ .int_value	= v },			\
	}

struct fix_message {
	enum fix_msg_type		type;

	/*
	 * These are required fields.
	 */
	const char			*begin_string;
	unsigned long			body_length;
	const char			*msg_type;
	const char			*sender_comp_id;
	const char			*target_comp_id;
	unsigned long			msg_seq_num;
	/* XXX: SendingTime */
	const char			*check_sum;
	char				*str_now;

	/*
	 * These buffers are used for wire protocol parsing and unparsing.
	 */
	struct buffer			*head_buf;	/* first three fields */
	struct buffer			*body_buf;	/* rest of the fields including checksum */

	unsigned long			nr_fields;
	struct fix_field		*fields;
};

struct fix_message_unparse_context {
	struct fix_field sender_comp_id;
	struct fix_field target_comp_id;
	struct fix_field begin_string;
	struct fix_field sending_time;
	struct fix_field body_length;
	struct fix_field msg_seq_num;
	struct fix_field check_sum;
	struct fix_field msg_type;
};

bool fix_field_unparse(struct fix_field *self, struct buffer *buffer);
bool fix_field_replace(struct fix_field *self, struct buffer *buffer);

struct fix_message *fix_message_new(void);
void fix_message_free(struct fix_message *self);

void fix_message_add_field(struct fix_message *msg, struct fix_field *field);

void fix_message_unparse(struct fix_message *self);
int fix_message_parse(struct fix_message *self, struct fix_dialect *dialect, struct buffer *buffer);

void fix_message_pre_unparse_fixed(struct fix_message *self, struct fix_message_unparse_context *ctx);
void fix_message_replace_fixed(struct fix_message *self, struct fix_message_unparse_context *ctx);

int fix_get_field_count(struct fix_message *self);
struct fix_field *fix_get_field_at(struct fix_message *self, int index);
struct fix_field *fix_get_field(struct fix_message *self, int tag);

const char *fix_get_string(struct fix_field *field, char *buffer, unsigned long len);

void fix_message_validate(struct fix_message *self);
int fix_message_send(struct fix_message *self, int sockfd, int flags);

enum fix_msg_type fix_msg_type_parse(const char *s, const char delim);
bool fix_message_type_is(struct fix_message *self, enum fix_msg_type type);

char *fix_timestamp_now(char *buf, size_t len);

#endif
