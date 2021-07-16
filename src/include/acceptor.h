#include "channel.h"
#include "macro.h"
#include "socket.h"

class EventLoop;
class InetAddress;

/**
 * Acceptor of incoming TCP connections.
 */

class Acceptor {
 public:
  using NewConnectionCallback =
      std::function<void(int sockfd, const InetAddress&)>;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr);

  void setNewConnectionCallback(const NewConnectionCallback& cb) {
    new_conn_cb_ = cb;
  }

  bool listenning() const { return listenning_; }

  void listen();

 private:
  void handleRead();

  EventLoop* loop_;
  /* Listening socket */
  Socket accept_socket_;
  /* Channel dealing with listening socket */
  Channel accept_channel_;
  NewConnectionCallback new_conn_cb_;
  bool listenning_;
};

