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

size_t buffer_inflate(struct buffer *buffer, int fd, z_stream *stream)
{
	unsigned char in[INFLATE_SIZE];
	ssize_t nr;
	int ret;

	nr = xread(fd, in, INFLATE_SIZE);
	if (nr < 0)
		return -1;

	if (!nr)
		return 0;

	stream->avail_in	= nr;
	stream->next_in		= in;
	stream->avail_out	= buffer_remaining(buffer);
	stream->next_out	= (void *) buffer_end(buffer);

	ret = inflate(stream, Z_NO_FLUSH);
	switch (ret) {
	case Z_STREAM_ERROR:
	case Z_DATA_ERROR:
	case Z_MEM_ERROR:
	case Z_NEED_DICT:
		return -1;
	default:
		break;
	}

	nr = buffer_remaining(buffer) - stream->avail_out;

	buffer->end += nr;

	return nr;
}
