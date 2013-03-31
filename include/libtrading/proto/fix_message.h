#ifndef LIBTRADING_FIX_MESSAGE_H
#define LIBTRADING_FIX_MESSAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

struct buffer;

/*
 * Message types:
 */
enum fix_msg_type {
	FIX_MSG_TYPE_HEARTBEAT		= 0,
	FIX_MSG_TYPE_TEST_REQUEST	= 1,
	FIX_MSG_TYPE_RESEND_REQUEST	= 2,
	FIX_MSG_TYPE_REJECT		= 3,
	FIX_MSG_TYPE_SEQUENCE_RESET	= 4,
	FIX_MSG_TYPE_LOGOUT		= 5,
	FIX_MSG_TYPE_EXECUTION_REPORT	= 6,
	FIX_MSG_TYPE_LOGON		= 7,
	FIX_MSG_TYPE_NEW_ORDER_SINGLE	= 8,

	FIX_MSG_TYPE_MAX,		/* non-API */

	FIX_MSG_TYPE_UNKNOWN		= ~0UL,
};

/*
 * Maximum FIX message size
 */
#define FIX_MAX_HEAD_LEN	64UL
#define FIX_MAX_BODY_LEN	256UL
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
	MsgSeqNum		= 34,
	MsgType			= 35,
	NewSeqNo		= 36,
	OrderID			= 37,
	OrderQty		= 38,
	OrdStatus		= 39,
	OrdType			= 40,
	PossDupFlag		= 43,
	Price			= 44,
	RefSeqNum		= 45,
	SenderCompID		= 49,
	SendingTime		= 52,
	Side			= 54,
	Symbol			= 55,
	TargetCompID		= 56,
	Text			= 58,
	TransactTime		= 60,
	EncryptMethod		= 98,
	HeartBtInt		= 108,
	TestReqID		= 112,
	GapFillFlag		= 123,
	ResetSeqNumFlag		= 141,
	ExecType		= 150,
	LeavesQty		= 151,
};

struct fix_field {
	enum fix_tag			tag;
	enum fix_type			type;

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
		{ .int_value	= v },			\
	}

#define FIX_STRING_FIELD(t, s)				\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_STRING,	\
		{ .string_value	= s },			\
	}

#define FIX_FLOAT_FIELD(t, v)				\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_FLOAT,	\
		{ .float_value  = v },			\
	}

#define FIX_CHECKSUM_FIELD(t, v)			\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_CHECKSUM,	\
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

	/*
	 * These buffers are used for wire protocol parsing and unparsing.
	 */
	struct buffer			*head_buf;	/* first three fields */
	struct buffer			*body_buf;	/* rest of the fields including checksum */

	unsigned long			nr_fields;
	struct fix_field		*fields;
};

bool fix_field_unparse(struct fix_field *self, struct buffer *buffer);

struct fix_message *fix_message_new(void);
void fix_message_free(struct fix_message *self);

void fix_message_add_field(struct fix_message *msg, struct fix_field *field);

int fix_message_parse(struct fix_message *self, struct buffer *buffer);
struct fix_field *fix_get_field(struct fix_message *self, int tag);
const char *fix_get_string(struct fix_field *field, char *buffer, unsigned long len);
void fix_message_validate(struct fix_message *self);
int fix_message_send(struct fix_message *self, int sockfd, int flags);

enum fix_msg_type fix_msg_type_parse(const char *s);
bool fix_message_type_is(struct fix_message *self, enum fix_msg_type type);

char *fix_timestamp_now(char *buf, size_t len);

#endif
