#include "libtrading/read-write.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

/* Same as read(2) except that this function never returns EAGAIN or EINTR. */
ssize_t xread(int fd, void *buf, size_t count)
{
	ssize_t nr;

restart:
	nr = read(fd, buf, count);
	if ((nr < 0) && ((errno == EAGAIN) || (errno == EINTR)))
		goto restart;

	return nr;
}

/* Same as write(2) except that this function never returns EAGAIN or EINTR. */
ssize_t xwrite(int fd, const void *buf, size_t count)
{
	ssize_t nr;

restart:
	nr = write(fd, buf, count);
	if ((nr < 0) && ((errno == EAGAIN) || (errno == EINTR)))
		goto restart;

	return nr;
}

/* Same as writev(2) except that this function never returns EAGAIN or EINTR. */
ssize_t xwritev(int fd, const struct iovec *iov, int iovcnt)
{
	ssize_t nr;

restart:
	nr = writev(fd, iov, iovcnt);
	if ((nr < 0) && ((errno == EAGAIN) || (errno == EINTR)))
		goto restart;

	return nr;
}
