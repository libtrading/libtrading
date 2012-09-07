#include "trading/buffer.h"

#include "trading/read-write.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

struct buffer *buffer_new(unsigned long capacity)
{
	struct buffer *self;

	self = malloc(sizeof *self + capacity);
	if (!self)
		return NULL;

	self->data	= (void *) self + sizeof(*self);
	self->capacity	= capacity;
	self->start	= 0;
	self->end	= 0;

	return self;
}

void buffer_delete(struct buffer *self)
{
	free(self);
}

uint8_t buffer_sum(struct buffer *self)
{
	unsigned long sum = 0;
	int i;

	for (i = self->start; i < self->end; i++)
		sum += self->data[i];

	return sum;
}

//! Formatted output to 'struct buffer'
/*!
  \return true if @format was appended to @self - false upon error of if
          @format was truncated due to buffer being to small.
 */
bool buffer_printf(struct buffer *self, const char *format, ...)
{
	size_t size;
	va_list ap;
	char *buf;
	int len;

	buf	= buffer_end(self);
	size	= buffer_remaining(self);

	va_start(ap, format);
	len = vsnprintf(buf, size, format, ap);
	va_end(ap);

	if (len < 0 || len >= size)
		return false;

	self->end += len;

	return true;
}

char *buffer_find(struct buffer *self, char delim)
{
	while (buffer_first_char(self) != delim) {
		if (!buffer_remaining(self))
			return NULL;

		buffer_advance(self, 1);
	}

	return buffer_start(self);
}

ssize_t buffer_read(struct buffer *self, int fd)
{
	size_t count;
	ssize_t len;
	void *buf;

	buf		= buffer_end(self);
	count		= buffer_remaining(self);

	len = xread(fd, buf, count);
	if (len < 0)
		return len;

	self->end	+= len;

	return len;
}

ssize_t buffer_write(struct buffer *self, int fd)
{
	return xwrite(fd, buffer_start(self), buffer_size(self));
}

void buffer_compact(struct buffer *buf)
{
	size_t count;
	void *start;

	start		= buffer_start(buf);
	count		= buffer_size(buf);

	memcpy(buf->data, start, count);

	buf->start	= 0;
	buf->end	= count;
}
