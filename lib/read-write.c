#include "libtrading/read-write.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

ssize_t sys_sendmsg(int fd, struct iovec *iov, size_t length, int flags)
{
	struct msghdr msg = (struct msghdr) {
		.msg_iov	= iov,
		.msg_iovlen	= length,
	};
	return sendmsg(fd, &msg, flags);
}

io_recv_t io_recv = &recv;
io_sendmsg_t io_sendmsg = &sys_sendmsg;

size_t iov_byte_length(struct iovec *iov, size_t iov_len)
{
	size_t len = 0;
	size_t i;

	for (i = 0; i < iov_len; ++i)
		len += iov[i].iov_len;

	return len;
}

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
