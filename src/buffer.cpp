#include "buffer.h"

#include <errno.h>
#include <memory.h>
#include <sys/uio.h>

#include "logging.h"
#include "sockets_options.h"

ssize_t Buffer::readFd(int fd, int* saved_errno) {
  /* Buffer on the stack */
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin() + writer_idx_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  /**
   * Level trigger, only need to read once
   * Note: If use Edge Trigger, need to read until EAGAIN
   */
  const ssize_t n = readv(fd, vec, 2);
  if (n < 0) {
    *saved_errno = errno;
  } else if (static_cast<size_t>(n) <= writable) {
    /* extrabuf is not needed */
    writer_idx_ += n;
  } else {
    /* extrbuf needed */
    writer_idx_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}

