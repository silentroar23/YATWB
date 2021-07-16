#include "sockets_options.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>    // snprintf
#include <strings.h>  // bzero
#include <sys/socket.h>
#include <unistd.h>

#include "logging.h"

using SA = struct sockaddr;

void setNonBlockAndCloseOnExec(int sockfd) {
  // non-block
  int flags = ::fcntl(sockfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sockfd, F_SETFL, flags);
  // FIXME check

  // close-on-exec
  flags = ::fcntl(sockfd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sockfd, F_SETFD, flags);
  // FIXME check
}

int sockets::createNonblockingOrDie() {
// socket
#if VALGRIND
  int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG << "sockets::createNonblockingOrDie() failed";
  }

  setNonBlockAndCloseOnExec(sockfd);
#else
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
  if (sockfd < 0) {
    LOG << "sockets::createNonblockingOrDie() failed";
  }
#endif
  return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr_in& addr) {
  int ret = ::bind(sockfd, reinterpret_cast<const SA*>(&addr), sizeof addr);
  if (ret < 0) {
    LOG << "sockets::bindOrDie";
  }
}

void sockets::listenOrDie(int sockfd) {
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0) {
    LOG << "sockets::listenOrDie";
  }
}

int sockets::accept(int sockfd, struct sockaddr_in* addr) {
  socklen_t addrlen = sizeof *addr;
#if VALGRIND
  int connfd = ::accept(sockfd, reinterpret_cast<SA*>(addr), &addrlen);
  setNonBlockAndCloseOnExec(connfd);
#else
  int connfd = ::accept4(sockfd, addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
  if (connfd < 0) {
    int savedErrno = errno;
    LOG << "Socket::accept";
    switch (savedErrno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO:  // ???
      case EPERM:
      case EMFILE:  // per-process lmit of open file desctiptor ???
        // expected errors
        errno = savedErrno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        LOG << "unexpected error of ::accept " << savedErrno;
        break;
      default:
        LOG << "unknown error of ::accept " << savedErrno;
        break;
    }
  }
  return connfd;
}

void sockets::close(int sockfd) {
  if (::close(sockfd) < 0) {
    LOG << "sockets::close";
  }
}

void sockets::toHostPort(char* buf, size_t size,
                         const struct sockaddr_in& addr) {
  char host[INET_ADDRSTRLEN] = "INVALID";
  ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof host);
  uint16_t port = sockets::networkToHost16(addr.sin_port);
  snprintf(buf, size, "%s:%u", host, port);
}

void sockets::fromHostPort(const char* ip, uint16_t port,
                           struct sockaddr_in* addr) {
  addr->sin_family = AF_INET;
  addr->sin_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
    LOG << "sockets::fromHostPort";
  }
}
