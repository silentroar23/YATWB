#include "acceptor.h"

#include "event_loop.h"
#include "inet_addr.h"
#include "logging.h"
#include "sockets_options.h"

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr)
    : loop_(loop),
      socket_(sockets::createNonblockingOrDie()),
      channel_(loop, socket_.getFd()),
      listenning_(false) {
  socket_.setReuseAddr(true);
  socket_.bindAddress(listen_addr);
  channel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

/**
 * Make socket_ start listen and register fd on poll_fds_
 */
void Acceptor::listen() {
  loop_->assertInLoopThread();
  listenning_ = true;
  socket_.listen();
  channel_.enableReading();
}

void Acceptor::handleRead() {
  loop_->assertInLoopThread();
  InetAddress peer_addr(0);
  // FIXME loop until no more
  int connfd = socket_.accept(&peer_addr);
  if (connfd >= 0) {
    if (new_conn_cb_) {
      new_conn_cb_(connfd, peer_addr);
    } else {
      sockets::close(connfd);
    }
  }
}
