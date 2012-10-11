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

uint8_t buffer_sum_range(struct buffer *buf, const char *start, const char *end)
{
	unsigned long sum = 0;
	const char *ptr;

	for (ptr = start; ptr < end; ptr++)
		sum += *ptr;

	return sum;
}

uint8_t buffer_sum(struct buffer *buf)
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

char *buffer_find(struct buffer *buf, char c)
{
	while (buffer_first_char(buf) != c) {
		if (!buffer_size(buf))
			return NULL;

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
