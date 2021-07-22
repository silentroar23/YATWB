#include "tcp_server.h"

#include <stdio.h>  // snprintf

#include "acceptor.h"
#include "event_loop.h"
#include "event_loop_thread_pool.h"
#include "inet_addr.h"
#include "logging.h"
#include "sockets_options.h"

/**
 * Once a TcpServer object is constructed, a Socket has been created and binded
 * through the instantiation of acceptor_
 */
TcpServer::TcpServer(EventLoop* loop, const InetAddress& listen_addr)
    : loop_(loop),
      name_(listen_addr.toHostPort()),
      acceptor_(new Acceptor(loop, listen_addr)),
      thread_pool_(new EventLoopThreadPool(loop)),
      started_(false),
      next_conn_id_(1) {
  /**
   * Use placeholders to provide arguments when invoking
   * TcpServer::newConnection
   */
  acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

TcpServer::~TcpServer() {}

void TcpServer::setThreadNum(int num_threads) {
  assert(0 <= num_threads);
  thread_pool_->setThreadNum(num_threads);
}

/**
 * Start listen on acceptor_.socket_
 */
void TcpServer::start() {
  if (!started_) {
    started_ = true;
    thread_pool_->start();
  }

  if (!acceptor_->listenning()) {
    loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
  }
}

/**
 * Create a new connection by constructing a TcpConnection object and pass
 * connection_cb_ and message_cb_ to it. Call connection->establishConnection to
 * register fd returned by syscall accept() on poll_fds_
 */
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
  loop_->assertInLoopThread();
  char buf[32];
  snprintf(buf, sizeof buf, "#%d", next_conn_id_);
  ++next_conn_id_;
  std::string connName = name_ + buf;

  LOG << "TcpServer::newConnection [" << name_ << "] - new connection ["
      << connName << "] from " << peerAddr.toHostPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  EventLoop* io_loop = thread_pool_->getNextLoop();
  TcpConnectionPtr conn(
      new TcpConnection(io_loop, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;
  conn->setConnectionCallback(connection_cb_);
  conn->setMessageCallback(message_cb_);
  conn->setWriteCallback(write_cmpl_cb_);
  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
  conn->establishConnection();
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
  loop_->assertInLoopThread();
  LOG << "TcpServer::removeConnection [" << name_ << "] - connection "
      << conn->name();
  size_t n = connections_.erase(conn->name());
  assert(n == 1);
  (void)n;
  loop_->queueInLoop(std::bind(&TcpConnection::destroyConnection, conn));
}
