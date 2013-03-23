#ifndef LIBTRADING_BUFFER_H
#define LIBTRADING_BUFFER_H

#include <sys/uio.h>	/* for struct iovec */
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>	/* for ssize_t */

struct buffer {
	unsigned long		start;
	unsigned long		end;
	unsigned long		capacity;
	char			*data;

	/* To store private info */
	void			*ptr;
};

struct buffer *buffer_new(unsigned long capacity);
void buffer_delete(struct buffer *self);
bool buffer_printf(struct buffer *self, const char *format, ...);
char *buffer_find(struct buffer *self, char delim);
uint8_t buffer_sum_range(struct buffer *buf, const char *start, const char *end);
uint8_t buffer_sum(struct buffer *self);

ssize_t buffer_xread(struct buffer *self, int fd);
ssize_t buffer_nxread(struct buffer *buf, int fd, size_t size);
ssize_t buffer_xwrite(struct buffer *self, int fd);

static inline uint8_t buffer_peek_8(struct buffer *self)
{
	return self->data[self->start];
}

static inline uint8_t buffer_peek_le16(struct buffer *self)
{
	return (self->data[self->start + 1] << 8) | (self->data[self->start]);
}

static inline uint8_t buffer_get_8(struct buffer *self)
{
	return self->data[self->start++];
}

static inline char buffer_get_char(struct buffer *self)
{
	return buffer_get_8(self);
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

static inline void *buffer_get_ptr(struct buffer *self)
{
	return self->ptr;
}

static inline void buffer_set_ptr(struct buffer *self, void *ptr)
{
	self->ptr = ptr;
}

/* User must ensure that there is space to store a byte */
static inline void buffer_put(struct buffer *self, char byte)
{
	self->data[self->end++] = byte;
}

static inline char *buffer_start(struct buffer *self)
{
	return &self->data[self->start];
}

static inline char *buffer_end(struct buffer *self)
{
	return &self->data[self->end];
}

static inline char buffer_first_char(struct buffer *self)
{
	const char *start = buffer_start(self);

	return *start;
}

static inline void buffer_advance(struct buffer *self, long n)
{
	self->start += n;
}

static inline unsigned long buffer_size(struct buffer *self)
{
	return self->end - self->start;
}

static inline unsigned long buffer_remaining(struct buffer *self)
{
	return self->capacity - self->end;
}

static inline void buffer_to_iovec(struct buffer *b, struct iovec *iov)
{
	iov->iov_base	= b->data;
	iov->iov_len	= b->end;
}

static inline void buffer_reset(struct buffer *buf)
{
	buf->start = buf->end = 0;
}

void buffer_compact(struct buffer *buf);

struct buffer *buffer_mmap(int fd, size_t len);
void buffer_munmap(struct buffer *buf);

#endif
