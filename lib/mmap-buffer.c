#include "libtrading/buffer.h"

#include <sys/mman.h>
#include <stdlib.h>

struct buffer *buffer_mmap(int fd, size_t len)
{
	struct buffer *buf;
	void *p;

	buf = calloc(1, sizeof(*buf));
	if (!buf)
		return NULL;

	p = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED)
		goto mmap_failed;

	buf->data	= p;
	buf->start	= 0;
	buf->end	= len;
	buf->capacity	= len;

	return buf;

mmap_failed:
	free(buf);
	return NULL;
}

void buffer_munmap(struct buffer *buf)
{
	munmap(buf->data, buf->capacity);

	free(buf);
}
