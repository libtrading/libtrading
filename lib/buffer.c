#include "libtrading/buffer.h"

#include "libtrading/read-write.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

struct buffer *buffer_new(unsigned long capacity)
{
	struct buffer *buf;

	buf = malloc(sizeof *buf + capacity);
	if (!buf)
		return NULL;

	buf->data	= (void *) buf + sizeof(*buf);
	buf->capacity	= capacity;
	buf->start	= 0;
	buf->end	= 0;

	return buf;
}

void buffer_delete(struct buffer *buf)
{
	free(buf);
}

u8 buffer_sum_range(struct buffer *buf, const char *start, const char *end)
{
	unsigned long sum = 0;
	const char *ptr;

	for (ptr = start; ptr < end; ptr++)
		sum += *ptr;

	return sum;
}

u8 buffer_sum(struct buffer *buf)
{
	unsigned long sum = 0;
	int i;

	for (i = buf->start; i < buf->end; i++)
		sum += buf->data[i];

	return sum;
}

bool buffer_printf(struct buffer *buf, const char *format, ...)
{
	size_t size;
	va_list ap;
	char *end;
	int len;

	end	= buffer_end(buf);
	size	= buffer_remaining(buf);

	va_start(ap, format);
	len = vsnprintf(end, size, format, ap);
	va_end(ap);

	if (len < 0 || len >= size)
		return false;

	buf->end += len;

	return true;
}

char *buffer_find(struct buffer *buf, u8 c)
{
	while (true) {
		if (!buffer_size(buf))
			return NULL;

		if (buffer_peek_8(buf) == c)
			break;

		buffer_advance(buf, 1);
	}

	return buffer_start(buf);
}

ssize_t buffer_read(struct buffer *buf, int fd)
{
	size_t count;
	ssize_t len;
	void *end;

	end	= buffer_end(buf);
	count	= buffer_remaining(buf);

	len = read(fd, end, count);
	if (len < 0)
		return len;

	buf->end += len;

	return len;
}

ssize_t buffer_xread(struct buffer *buf, int fd)
{
	size_t count;
	ssize_t len;
	void *end;

	end	= buffer_end(buf);
	count	= buffer_remaining(buf);

	len = xread(fd, end, count);
	if (len < 0)
		return len;

	buf->end += len;

	return len;
}

ssize_t buffer_nread(struct buffer *buf, int fd, size_t size)
{
	size_t count;
	ssize_t len;
	void *end;

	end	= buffer_end(buf);
	count	= buffer_remaining(buf);

	if (count > size)
		count = size;

	len = read(fd, end, count);
	if (len < 0)
		return len;

	buf->end += len;

	return len;
}

ssize_t buffer_nxread(struct buffer *buf, int fd, size_t size)
{
	size_t count;
	ssize_t len;
	void *end;

	end	= buffer_end(buf);
	count	= buffer_remaining(buf);

	if (count > size)
		count = size;

	len = xread(fd, end, count);
	if (len < 0)
		return len;

	buf->end += len;

	return len;
}

ssize_t buffer_write(struct buffer *buf, int fd)
{
	size_t count;
	void *start;

	start	= buffer_start(buf);
	count	= buffer_size(buf);

	return write(fd, start, count);
}

ssize_t buffer_xwrite(struct buffer *buf, int fd)
{
	size_t count;
	void *start;

	start	= buffer_start(buf);
	count	= buffer_size(buf);

	return xwrite(fd, start, count);
}

void buffer_compact(struct buffer *buf)
{
	size_t count;
	void *start;

	start	= buffer_start(buf);
	count	= buffer_size(buf);

	memmove(buf->data, start, count);

	buf->start	= 0;
	buf->end	= count;
}

#define INFLATE_SIZE	(1ULL << 18) /* 256 KB */

ssize_t buffer_inflate(struct buffer *comp_buf, struct buffer *uncomp_buf, z_stream *stream)
{
	unsigned long nr;
	ssize_t ret;
	int err;

	nr = buffer_size(comp_buf);
	if (!nr)
		return 0;

	if (nr > INFLATE_SIZE)
		nr = INFLATE_SIZE;

	stream->avail_in	= nr;
	stream->avail_out	= buffer_remaining(uncomp_buf);
	stream->next_out	= (void *) buffer_end(uncomp_buf);

retry:
	err = inflate(stream, Z_NO_FLUSH);
	switch (err) {
	case Z_STREAM_END:
	case Z_BUF_ERROR:
	case Z_OK:
		/* OK to continue */
		break;
	default:
		return -1;
	}

	if (!err && !stream->avail_out)
		goto retry;

	buffer_advance(comp_buf, nr - stream->avail_in);

	ret = buffer_remaining(uncomp_buf) - stream->avail_out;

	uncomp_buf->end += ret;

	return ret;
}
