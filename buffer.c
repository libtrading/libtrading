#include "trading/buffer.h"

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

	self->capacity	= capacity;
	self->offset	= 0;

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

	for (i = 0; i < self->offset; i++)
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

	buf	= buffer_start(self);
	size	= buffer_remaining(self);

	va_start(ap, format);
	len = vsnprintf(buf, size, format, ap);
	va_end(ap);

	if (len < 0 || len >= size)
		return false;

	self->offset += len;

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
	void *buf;

	buf		= buffer_start(self);
	count		= buffer_remaining(self);

	return read(fd, buf, count);
}

ssize_t buffer_write(struct buffer *self, int fd)
{
	return write(fd, self->data, self->offset);
}
