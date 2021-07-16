#include "acceptor.h"

#include "event_loop.h"
#include "inet_addr.h"
#include "logging.h"
#include "sockets_options.h"

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr)
    : loop_(loop),
      accept_socket_(sockets::createNonblockingOrDie()),
      accept_channel_(loop, accept_socket_.fd()),
      listenning_(false) {
  accept_socket_.setReuseAddr(true);
  accept_socket_.bindAddress(listen_addr);
  accept_channel_.setReadHandler(std::bind(&Acceptor::handleRead, this));
}

void Acceptor::listen() {
  loop_->assertInLoopThread();
  listenning_ = true;
  accept_socket_.listen();
  accept_channel_.enableReading();
}

void Acceptor::handleRead() {
  loop_->assertInLoopThread();
  InetAddress peer_addr(0);
  // FIXME loop until no more
  int connfd = accept_socket_.accept(&peer_addr);
  if (connfd >= 0) {
    if (new_conn_cb_) {
      new_conn_cb_(connfd, peer_addr);
    } else {
      sockets::close(connfd);
    }
  }
}
