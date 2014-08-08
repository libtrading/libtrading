#ifndef LIBTRADING_READ_WRITE_H
#define LIBTRADING_READ_WRITE_H

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

typedef ssize_t (*io_recv_t)(int fd, void *buffer, size_t length, int flags);
typedef ssize_t (*io_sendmsg_t)(int fd, struct iovec *iov, size_t length, int flags);

ssize_t sys_sendmsg(int fd, struct iovec *iov, size_t length, int flags);

extern io_recv_t io_recv;
extern io_sendmsg_t io_sendmsg;

ssize_t xread(int fd, void *buf, size_t count);
ssize_t xwrite(int fd, const void *buf, size_t count);
ssize_t xwritev(int fd, const struct iovec *iov, int iovcnt);

#endif
