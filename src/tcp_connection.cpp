#include "tcp_connection.h"

#include <errno.h>
#include <stdio.h>

#include "buffer.h"
#include "channel.h"
#include "event_loop.h"
#include "logging.h"
#include "socket.h"
#include "sockets_options.h"

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name_arg,
                             int sockfd, const InetAddress& local_addr,
                             const InetAddress& peer_addr)
    : loop_(loop),
      name_(name_arg),
      state_(States::Connecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr) {
  LOG << "TcpConnection::ctor[" << name_ << "] at " << this << " fd=" << sockfd;
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
}

/**
 * The destructor of Socket will close fd
 */
TcpConnection::~TcpConnection() {
  LOG << "TcpConnection::dtor[" << name_ << "] at " << this
      << " fd=" << channel_->getFd();
}

/**
 * Register ReadEvent on socket_->getFd() and invoke connection_cb_ passed by
 * TcpServer
 */
void TcpConnection::establishConnection() {
  loop_->assertInLoopThread();
  assert(state_ == States::Connecting);
  setState(States::Connected);
  channel_->enableReading();

  connection_cb_(shared_from_this());
}

void TcpConnection::handleRead() {
  char buf[65536];
  ssize_t n = ::read(channel_->getFd(), buf, sizeof buf);
  /* Invoke message callback when readable events arrive */
  if (n > 0) {
    message_cb_(shared_from_this(), buf, n);
  } else if (n == 0) {
    handleClose();
  } else {
    handleError();
  }
}

void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = write(channel_->getFd(), output_buffer_.peek(),
                      output_buffer_.readableBytes());
    if (n > 0) {
      output_buffer_.retrieve(n);
      if (output_buffer_.readableBytes() == 0) {
        channel_->disableWriting();
        if (state_ == States::Disconnecting) {
          shutdownInLoop();
        }
      } else {
        LOG << "I am going to write more data";
      }
    } else {
      LOG << "TcpConnection::handleWrite";
    }
  } else {
    LOG << "Connection is down, no more writing";
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG << "TcpConnection::handleClose state = " << state_;
  assert(state_ == States::Connected || state_ == States::Disconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  channel_->disableAllEvents();
  // must be the last line
  closeCallback_(shared_from_this());
}

void TcpConnection::handleError() {
  int err = sockets::getSocketError(channel_->getFd());
  LOG << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err
      << " " << strerror_tl(err);
}

void TcpConnection::destroyConnection() {
  loop_->assertInLoopThread();
  assert(state_ == States::Connected || state_ == States::Disconnecting);
  setState(States::Disconnected);
  channel_->disableAllEvents();
  connection_cb_(shared_from_this());

  loop_->removeChannel(channel_.get());
}
