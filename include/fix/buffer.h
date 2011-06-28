#ifndef FIX__BUFFER_H
#define FIX__BUFFER_H

#include <stdbool.h>
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>	/* for ssize_t */

struct buffer {
	unsigned long		offset;
	unsigned long		capacity;
	char			data[];
};

struct buffer *buffer_new(unsigned long capacity);
void buffer_delete(struct buffer *self);
bool buffer_printf(struct buffer *self, const char *format, ...);
char *buffer_find(struct buffer *self, char delim);
uint8_t buffer_checksum(struct buffer *self);

ssize_t buffer_read(struct buffer *self, int fd);
ssize_t buffer_write(struct buffer *self, int fd);

static inline char *buffer_start(struct buffer *self)
{
	return &self->data[self->offset];
}

static inline char buffer_first_char(struct buffer *self)
{
	const char *start = buffer_start(self);

	return *start;
}

static inline void buffer_advance(struct buffer *self)
{
	/* FIXME: overflow */
	assert(self->offset < self->capacity);

	self->offset++;
}

static inline unsigned long buffer_size(struct buffer *self)
{
	return self->offset;
}

static inline unsigned long buffer_remaining(struct buffer *self)
{
	assert(self->offset <= self->capacity);

	return self->capacity - self->offset;
}

#endif /* FIX__BUFFER_H */
