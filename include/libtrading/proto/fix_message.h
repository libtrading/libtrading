#ifndef LIBTRADING_FIX_MESSAGE_H
#define LIBTRADING_FIX_MESSAGE_H

#include <stdbool.h>
#include <stdint.h>

struct buffer;

/*
 * Message types:
 */
#define	FIX_MSG_HEARTBEAT	"0"
#define FIX_MSG_TEST_REQUEST	"1"
#define FIX_MSG_RESEND_REQUEST	"2"
#define FIX_MSG_REJECT		"3"
#define FIX_MSG_SEQUENCE_REST	"4"
#define FIX_MSG_LOGOUT		"5"
#define FIX_MSG_LOGON		"A"

enum fix_type {
	FIX_TYPE_INT,
	FIX_TYPE_FLOAT,
	FIX_TYPE_CHAR,
	FIX_TYPE_STRING,
	FIX_TYPE_CHECKSUM,
};

enum fix_tag {
	BeginString		= 8,
	BodyLength		= 9,
	CheckSum		= 10,
	MsgSeqNum		= 34,
	MsgType			= 35,
	SenderCompID		= 49,
	SendingTime		= 52,
	TargetCompID		= 56,
	EncryptMethod		= 98,
	HeartBtInt		= 108,
	TestReqID		= 112,
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

#define FIX_CHECKSUM_FIELD(t, v)			\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_CHECKSUM,	\
		{ .int_value	= v },			\
	}

struct fix_message {
	/*
	 * These are required fields.
	 */
	const char			*begin_string;
	/* XXX: BodyLength */
	const char			*msg_type;
	const char			*sender_comp_id;
	const char			*target_comp_id;
	unsigned long			msg_seq_num;
	/* XXX: SendingTime */

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

struct fix_message *fix_message_parse(struct buffer *buffer);
void fix_message_validate(struct fix_message *self);
struct fix_message *fix_message_recv(int sockfd, int flags);
int fix_message_send(struct fix_message *self, int sockfd, int flags);

bool fix_message_type_is(struct fix_message *self, const char *type);

#endif
