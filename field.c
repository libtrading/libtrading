#include "fix/field.h"

#include "fix/buffer.h"

#include <inttypes.h>

bool fix_field_unparse(struct fix_field *self, struct buffer *buffer)
{
	switch (self->type) {
	case FIX_TYPE_STRING:
		return buffer_printf(buffer, "%d=%s\x01", self->tag, self->string_value);
	case FIX_TYPE_CHAR:
		return buffer_printf(buffer, "%d=%c\x01", self->tag, self->char_value);
	case FIX_TYPE_FLOAT:
		return buffer_printf(buffer, "%d=%f\x01", self->tag, self->float_value);
	case FIX_TYPE_INT:
		return buffer_printf(buffer, "%d=%" PRId64 "\x01", self->tag, self->int_value);
	case FIX_TYPE_CHECKSUM:
		return buffer_printf(buffer, "%d=%03" PRId64 "\x01", self->tag, self->int_value);
	default:
		/* unknown type */
		break;
	};

	return false;
}
