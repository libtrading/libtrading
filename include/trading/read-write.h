#ifndef LIBTRADING_READ_WRITE_H
#define LIBTRADING_READ_WRITE_H

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

ssize_t xread(int fd, void *buf, size_t count);
ssize_t xwrite(int fd, const void *buf, size_t count);
ssize_t xwritev(int fd, const struct iovec *iov, int iovcnt);

#endif
