#ifndef LIBTRADING_BUFFER_H
#define LIBTRADING_BUFFER_H

#include <libtrading/types.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>	/* for struct iovec */
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>	/* for ssize_t */
#include <zlib.h>	/* for z_stream */

struct buffer {
	unsigned long		start;
	unsigned long		end;
	unsigned long		capacity;
	char			*data;
};

struct buffer *buffer_new(unsigned long capacity);
void buffer_delete(struct buffer *self);
bool buffer_printf(struct buffer *self, const char *format, ...);
char *buffer_find(struct buffer *self, u8 c);
u8 buffer_sum_range(struct buffer *buf, const char *start, const char *end);
u8 buffer_sum(struct buffer *self);

ssize_t buffer_recv(struct buffer *self, int sockfd, size_t size);
ssize_t buffer_xwritev(int fd, struct msghdr *msg, int flags);
ssize_t buffer_sendmsg(int fd, struct msghdr *msg, int flags);
ssize_t buffer_xread(struct buffer *self, int fd);
ssize_t buffer_nxread(struct buffer *buf, int fd, size_t size);
ssize_t buffer_xwrite(struct buffer *self, int fd);
ssize_t buffer_read(struct buffer *self, int fd);
ssize_t buffer_nread(struct buffer *buf, int fd, size_t size);
ssize_t buffer_write(struct buffer *self, int fd);

static inline u8 buffer_peek_8(struct buffer *self)
{
	return self->data[self->start];
}

static inline u8 buffer_peek_le16(struct buffer *self)
{
	return (self->data[self->start + 1] << 8) | (self->data[self->start]);
}

static inline u8 buffer_get_8(struct buffer *self)
{
	return self->data[self->start++];
}

static inline char buffer_get_char(struct buffer *self)
{
	return buffer_get_8(self);
}
static inline u16 buffer_get_le16(struct buffer *self)
{
	return buffer_get_8(self) | buffer_get_8(self) << 8;
}

static inline u32 buffer_get_le32(struct buffer *self)
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

static inline u16 buffer_get_be16(struct buffer *self)
{
	return buffer_get_8(self) << 8 | buffer_get_8(self);
}

static inline void buffer_get_n(struct buffer *self, int n, char *dst)
{
	int i;

	for (i = 0; i < n; i++)
		*dst++ = buffer_get_8(self);
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

ssize_t buffer_inflate(struct buffer *comp_buf, struct buffer *uncomp_buf, z_stream *stream);

#endif
