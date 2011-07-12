#ifndef FIX_READ_WRITE_H
#define FIX_READ_WRITE_H

#include <sys/uio.h>
#include <unistd.h>

ssize_t xwritev(int fd, const struct iovec *iov, int iovcnt);

#endif /* FIX_READ_WRITE_H */
