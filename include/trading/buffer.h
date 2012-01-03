#ifndef LIBTRADING_BUFFER_H
#define LIBTRADING_BUFFER_H

#include <sys/uio.h>	/* for struct iovec */
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
uint8_t buffer_sum(struct buffer *self);

ssize_t buffer_read(struct buffer *self, int fd);
ssize_t buffer_write(struct buffer *self, int fd);

static inline uint8_t buffer_get_8(struct buffer *self)
{
	return self->data[self->offset++];
}

static inline uint16_t buffer_get_le16(struct buffer *self)
{
	return buffer_get_8(self) | buffer_get_8(self) << 8;
}

static inline uint32_t buffer_get_le32(struct buffer *self)
{
	return buffer_get_8(self)
		| buffer_get_8(self) << 8
		| buffer_get_8(self) << 16
		| buffer_get_8(self) << 24;
}

static inline uint64_t buffer_get_le64(struct buffer *self)
{
	return (uint64_t) buffer_get_8(self)
		| (uint64_t) buffer_get_8(self) << 8
		| (uint64_t) buffer_get_8(self) << 16
		| (uint64_t) buffer_get_8(self) << 24
		| (uint64_t) buffer_get_8(self) << 32
		| (uint64_t) buffer_get_8(self) << 40
		| (uint64_t) buffer_get_8(self) << 48
		| (uint64_t) buffer_get_8(self) << 56;
}

static inline uint16_t buffer_get_be16(struct buffer *self)
{
	return buffer_get_8(self) << 8 | buffer_get_8(self);
}

static inline void buffer_get_n(struct buffer *self, int n, char *dst)
{
	int i;

	for (i = 0; i < n; i++)
		*dst++ = buffer_get_8(self);
}

static inline char *buffer_start(struct buffer *self)
{
	return &self->data[self->offset];
}

static inline char buffer_first_char(struct buffer *self)
{
	const char *start = buffer_start(self);

	return *start;
}

static inline void buffer_advance(struct buffer *self, unsigned long n)
{
	/* FIXME: overflow */
	assert(self->offset < self->capacity);

	self->offset += n;
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

static inline void buffer_to_iovec(struct buffer *b, struct iovec *iov)
{
	iov->iov_base	= b->data;
	iov->iov_len	= b->offset;
}

static inline void buffer_reset(struct buffer *buf)
{
	buf->offset = 0;
}

#endif /* LIBTRADING_BUFFER_H */
