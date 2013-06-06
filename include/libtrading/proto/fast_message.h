#ifndef LIBTRADING_FAST_MESSAGE_H
#define LIBTRADING_FAST_MESSAGE_H

#include <libtrading/types.h>

#include <stdbool.h>
#include <search.h>
#include <stdint.h>

#define	FAST_PMAP_MAX_BYTES		8
#define	FAST_FIELD_MAX_NUMBER		128
#define	FAST_STRING_MAX_BYTES		256
#define	FAST_MESSAGE_MAX_SIZE		2048

#define	FAST_FIELDS_HASH_SIZE		128

#define	FAST_TEMPLATE_MAX_NUMBER	128

#define	FAST_SEQUENCE_ELEMENTS		32

#define	FAST_MSG_STATE_GARBLED		(-1)
#define	FAST_MSG_STATE_PARTIAL		(-2)

#define	FAST_MSG_FLAGS_RESET			0x00000001

#define	FAST_FIELD_FLAGS_UNICODE		0x00000001
#define	FAST_FIELD_FLAGS_PMAPREQ		0x00000002
#define	FAST_FIELD_FLAGS_DECIMAL_INDIVID	0x00000004

struct buffer;
struct fast_session;

enum fast_type {
	FAST_TYPE_INT,
	FAST_TYPE_UINT,
	FAST_TYPE_STRING,
	FAST_TYPE_DECIMAL,
	FAST_TYPE_SEQUENCE,
};

enum fast_op {
	FAST_OP_NONE,
	FAST_OP_COPY,
	FAST_OP_INCR,
	FAST_OP_DELTA,
	FAST_OP_DEFAULT,
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
	bool		is_valid;
	long		pmap_bit;
	unsigned long	nr_bytes;
	char	bytes[FAST_PMAP_MAX_BYTES];
};

struct fast_decimal {
	struct fast_field	*fields;
	i64			exp;
	i64			mnt;
};

struct fast_field {
	enum fast_presence	presence;
	enum fast_type		type;
	enum fast_op		op;

	enum fast_state		state;
	enum fast_state		state_previous;

	bool			has_reset;
	int			flags;

	char			name[32];
	int			id;

	union {
		i64			int_value;
		u64			uint_value;
		void			*ptr_value;
		char			string_value[FAST_STRING_MAX_BYTES];
		struct fast_decimal	decimal_value;
	};

	union {
		i64			int_reset;
		u64			uint_reset;
		void			*ptr_reset;
		char			string_reset[FAST_STRING_MAX_BYTES];
		struct fast_decimal	decimal_reset;
	};

	union {
		i64			int_previous;
		u64			uint_previous;
		void			*ptr_previous;
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
}

static inline bool field_is_mandatory(struct fast_field *field)
{
	return field->presence == FAST_PRESENCE_MANDATORY;
}

static inline bool field_has_reset_value(struct fast_field *field)
{
	return field->has_reset;
}

static inline void field_set_flags(struct fast_field *field, int flags)
{
	field->flags = flags;
}

static inline void field_add_flags(struct fast_field *field, int flags)
{
	field->flags |= flags;
}

static inline void field_clear_flags(struct fast_field *field, int flags)
{
	field->flags &= ~flags;
}

static inline int field_has_flags(struct fast_field *field, int flags)
{
	return field->flags & flags;
}

struct fast_message {
	unsigned long		nr_fields;
	unsigned long		decoded;
	struct fast_field	*fields;
	struct hsearch_data	*htab;

	char			name[32];
	int			flags;
	unsigned long		tid;

	struct buffer		*pmap_buf;
	struct buffer		*msg_buf;
};

static inline void fast_msg_set_flags(struct fast_message *msg, int flags)
{
	msg->flags = flags;
}

static inline void fast_msg_add_flags(struct fast_message *msg, int flags)
{
	msg->flags |= flags;
}

static inline void fast_msg_clear_flags(struct fast_message *msg, int flags)
{
	msg->flags &= ~flags;
}

static inline int fast_msg_has_flags(struct fast_message *msg, int flags)
{
	return msg->flags & flags;
}

struct fast_sequence {
	struct fast_pmap pmap;
	unsigned long decoded;
	struct fast_field length;
	struct fast_message elements[FAST_SEQUENCE_ELEMENTS];
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

static inline int pmap_required(struct fast_field *field)
{
	int ret = 0;

	switch (field->op) {
	case FAST_OP_CONSTANT:
		if (!field_is_mandatory(field))
			ret = 1;
		break;
	case FAST_OP_COPY:
	case FAST_OP_INCR:
		ret = 1;
		break;
	case FAST_OP_NONE:
	case FAST_OP_DELTA:
		break;
	default:
		break;
	}

	return ret;
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

int fast_message_copy(struct fast_message *dst, struct fast_message *src);
struct fast_message *fast_message_new(int nr_messages);
void fast_fields_free(struct fast_message *self);
void fast_message_free(struct fast_message *self, int nr_messages);
void fast_message_reset(struct fast_message *msg);
struct fast_field *fast_get_field(struct fast_message *msg, const char *name);
struct fast_message *fast_message_decode(struct fast_session *session);
int fast_message_send(struct fast_message *self, int sockfd, int flags);
int fast_message_encode(struct fast_message *msg);

#endif
