#ifndef FIX__FIELD_H
#define FIX__FIELD_H

#include <stdbool.h>
#include <stdint.h>

struct buffer;

enum fix_type {
	FIX_TYPE_INT,
	FIX_TYPE_FLOAT,
	FIX_TYPE_CHAR,
	FIX_TYPE_STRING,
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
		.type		= FIX_TYPE_INT,	\
		{ .int_value	= v },			\
	}

#define FIX_STRING_FIELD(t, s)				\
	(struct fix_field) {				\
		.tag		= t,			\
		.type		= FIX_TYPE_STRING,	\
		{ .string_value	= s },			\
	}

bool fix_field_unparse(struct fix_field *self, struct buffer *buffer);

#endif /* FIX__FIELD_H */
