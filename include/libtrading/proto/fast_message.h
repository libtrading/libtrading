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

struct fast_pmap {
	unsigned long	nr_bytes;
	char	bytes[FAST_PMAP_MAX_BYTES];
};

struct fast_field {
	enum fast_presence	presence;
	enum fast_type		type;
	enum fast_op		op;

	unsigned long		pmap_bit;
	bool			is_none;

	union {
		i64		int_value;
		u64		uint_value;
		char		string_value[FAST_STRING_MAX_BYTES];
	};

	union {
		i64		int_reset;
		u64		uint_reset;
		char		string_reset[FAST_STRING_MAX_BYTES];
	};

	union {
		i64		int_previous;
		u64		uint_previous;
		char		string_previous[FAST_STRING_MAX_BYTES];
	};
};

static inline bool field_is_none(struct fast_field *field)
{
	return field->is_none;
}

static inline bool field_is_mandatory(struct fast_field *field)
{
	return field->presence == FAST_PRESENCE_MANDATORY;
}

struct fast_message {
	unsigned long		nr_fields;
	struct fast_field	*fields;

	struct fast_pmap	*pmap;

	int			tid;
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

struct fast_message *fast_message_new(int nr_messages);
void fast_message_free(struct fast_message *self, int nr_messages);
bool fast_message_copy(struct fast_message *dest, struct fast_message *src);
struct fast_message *fast_message_decode(struct fast_message *msgs, struct buffer *buffer, u64 last_tid);

#endif
