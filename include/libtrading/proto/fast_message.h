#ifndef LIBTRADING_FAST_MESSAGE_H
#define LIBTRADING_FAST_MESSAGE_H

#include <libtrading/types.h>

#include <stdbool.h>
#include <stdint.h>

#define	FAST_PMAP_MAX_BYTES		8
#define	FAST_FIELD_MAX_NUMBER		128
#define	FAST_STRING_MAX_BYTES		256
#define	FAST_MESSAGE_MAX_SIZE		2048

#define	FAST_TEMPLATE_MAX_NUMBER	1

#define	FAST_MSG_STATE_PARTIAL	(-1)
#define	FAST_MSG_STATE_GARBLED	(-2)

struct buffer;

enum fast_type {
	FAST_TYPE_INT,
	FAST_TYPE_UINT,
	FAST_TYPE_STRING,
	FAST_TYPE_DECIMAL,
};

enum fast_op {
	FAST_OP_NONE,
	FAST_OP_COPY,
	FAST_OP_INCR,
	FAST_OP_DELTA,
	FAST_OP_CONSTANT,
};

enum fast_presence {
	FAST_PRESENCE_OPTIONAL,
	FAST_PRESENCE_MANDATORY,
};

enum fast_state {
	FAST_STATE_UNDEFINED,
	FAST_STATE_ASSIGNED,
	FAST_STATE_EMPTY,
};

struct fast_pmap {
	unsigned long	nr_bytes;
	char	bytes[FAST_PMAP_MAX_BYTES];
};

struct fast_decimal {
	i64			exp;
	i64			mnt;
};

struct fast_field {
	enum fast_presence	presence;
	enum fast_type		type;
	enum fast_op		op;

	unsigned long		pmap_bit;

	enum fast_state		state;
	enum fast_state		state_previous;

	bool			has_reset;

	union {
		i64			int_value;
		u64			uint_value;
		char			string_value[FAST_STRING_MAX_BYTES];
		struct fast_decimal	decimal_value;
	};

	union {
		i64			int_reset;
		u64			uint_reset;
		char			string_reset[FAST_STRING_MAX_BYTES];
		struct fast_decimal	decimal_reset;
	};

	union {
		i64			int_previous;
		u64			uint_previous;
		char			string_previous[FAST_STRING_MAX_BYTES];
		struct fast_decimal	decimal_previous;
	};
};

static inline bool field_state_empty(struct fast_field *field)
{
	return field->state == FAST_STATE_EMPTY;
}

static inline bool field_state_empty_previous(struct fast_field *field)
{
	return field->state_previous == FAST_STATE_EMPTY;
}

static inline void field_set_empty(struct fast_field *field)
{
	field->state = FAST_STATE_EMPTY;

	return;
}

static inline bool field_is_mandatory(struct fast_field *field)
{
	return field->presence == FAST_PRESENCE_MANDATORY;
}

static inline bool field_has_reset_value(struct fast_field *field)
{
	return field->has_reset;
}

struct fast_message {
	unsigned long		nr_fields;
	struct fast_field	*fields;

	struct fast_pmap	*pmap;

	unsigned long		tid;

	struct buffer		*pmap_buf;
	struct buffer		*msg_buf;
};

static inline bool pmap_is_set(struct fast_pmap *pmap, unsigned long bit)
{
	if ((bit / 7) >= pmap->nr_bytes)
		return false;

	return pmap->bytes[bit / 7] & (1 << (6 - bit % 7));
}

static inline bool pmap_set(struct fast_pmap *pmap, unsigned long bit)
{
	if ((bit / 7) >= pmap->nr_bytes)
		return false;

	pmap->bytes[bit / 7] |= (1 << (6 - bit % 7));

	return true;
}

static inline int transfer_size_int(i64 data)
{
	i64 tmp = data >= 0 ? data : ~data;

	if (!(tmp >> 6))
		return 1;
	else if (!(tmp >> 13))
		return 2;
	else if (!(tmp >> 20))
		return 3;
	else if (!(tmp >> 27))
		return 4;
	else if (!(tmp >> 34))
		return 5;
	else if (!(tmp >> 41))
		return 6;
	else if (!(tmp >> 48))
		return 7;
	else if (!(tmp >> 55))
		return 8;
	else
		return 9;
}

static inline int transfer_size_uint(u64 data)
{
	if (!(data >> 7))
		return 1;
	else if (!(data >> 14))
		return 2;
	else if (!(data >> 21))
		return 3;
	else if (!(data >> 28))
		return 4;
	else if (!(data >> 35))
		return 5;
	else if (!(data >> 42))
		return 6;
	else if (!(data >> 49))
		return 7;
	else if (!(data >> 56))
		return 8;
	else
		return 9;
}

struct fast_message *fast_message_new(int nr_messages);
void fast_message_free(struct fast_message *self, int nr_messages);
bool fast_message_copy(struct fast_message *dest, struct fast_message *src);
struct fast_message *fast_message_decode(struct fast_message *msgs, struct buffer *buffer, u64 last_tid);
int fast_message_send(struct fast_message *self, int sockfd, int flags);
int fast_message_encode(struct fast_message *msg);
void fast_message_init(struct fast_message *self);

#endif
