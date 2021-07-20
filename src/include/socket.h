#include "macro.h"
class InetAddress;

/**
 * Wrapper of socket file descriptor.
 *
 * It closes the sockfd when desctructs.
 * It is thread safe, all operations are delagated to OS.
 */
class Socket {
 public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}

  DISALLOW_COPY(Socket);

  ~Socket();

  int getFd() const { return sockfd_; }

  /* abort if address in use */
  void bindAddress(const InetAddress& localaddr);

  /* abort if address in use */
  void listen();

  /**
   * 1. On success, returns a non-negative integer that is a descriptor for the
   * accepted socket, which has been set to non-blocking and close-on-exec and
   * peeraddr will be assigned
   * 2. On error, -1 is returned, and peeraddr is untouched.
   */
  int accept(InetAddress* peeraddr);

  /* Enable/disable SO_REUSEADDR */
  void setReuseAddr(bool on);

 private:
  const int sockfd_;
};
