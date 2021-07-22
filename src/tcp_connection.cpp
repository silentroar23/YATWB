#include "tcp_connection.h"

#include <errno.h>
#include <stdio.h>

#include <functional>

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
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
}

/**
 * The destructor of Socket will close fd
 */
TcpConnection::~TcpConnection() {
  LOG << "TcpConnection::dtor[" << name_ << "] at " << this
      << " fd=" << channel_->getFd();
}

/**
 * Send data actively
 *
 * Delegating the actual I/O work to sendInLoop to make sure it is done in I/O
 * threads
 */
void TcpConnection::send(const std::string& message) {
  if (state_ == States::Connected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, message));
    }
  }
}

void TcpConnection::sendInLoop(const std::string& message) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  /**
   * If channel_ is not writing and output_buffer_ has no remaining data, write
   * immediately
   */
  if (!channel_->isWriting() && output_buffer_.readableBytes() == 0) {
    nwrote = write(channel_->getFd(), message.data(), message.size());
    if (nwrote >= 0) {
      if (static_cast<size_t>(nwrote) < message.size()) {
        LOG << "I am going to write more data";
      } else if (write_cmpl_cb_) {
        loop_->queueInLoop(std::bind(write_cmpl_cb_, shared_from_this()));
      }
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG << "Error: TcpConnection::sendInLoop";
      }
    }
  }

  /**
   * Several scenarios:
   * 1. None of @message is sent cos output_buffer_ has remaining data;
   * 2. Only part of @message is sent;
   * to prevent data be out of order, append data at the end of output_buffer_,
   * register channel_'s WriteEvent, and send data together later in
   * handleWrite()
   */
  assert(nwrote >= 0);
  // TODO(Q): resizing?
  if (static_cast<size_t>(nwrote) < message.size()) {
    output_buffer_.append(message.data() + nwrote, message.size() - nwrote);
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

/**
 * Shutdown write side of the connected socket
 * Delegate the actual shutdown work to shutdownInLoop() to ensure thread safe
 */
void TcpConnection::shutdown() {
  // FIXME: use compare and swap
  if (state_ == States::Connected) {
    setState(States::Disconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

/**
 * Only called in I/O threads
 */
void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  if (!channel_->isWriting()) {
    // we are not writing
    socket_->shutdownWrite();
  }
}

void TcpConnection::setTcpNoDelay(bool on) { socket_->setTcpNoDelay(on); }

void TcpConnection::setTcpKeepAlive(bool on) { socket_->setKeepAlive(on); }

/**
 * Called in TcpServer::newConnection()
 *
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

/**
 * ReadEventCallback of channel_
 */
void TcpConnection::handleRead(Timestamp recv_time) {
  int saved_errno = 0;
  ssize_t n = input_buffer_.readFd(channel_->getFd(), &saved_errno);
  /* Invoke message callback when readable events arrive */
  if (n > 0) {
    message_cb_(shared_from_this(), &input_buffer_, recv_time);
  } else if (n == 0) {
    handleClose();
  } else {
    errno = saved_errno;
    LOG << "Error: TcpConnection::handleRead";
    handleError();
  }
}

/**
 * WriteCallback of channel_
 */
void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = write(channel_->getFd(), output_buffer_.peek(),
                      output_buffer_.readableBytes());
    if (n > 0) {
      output_buffer_.retrieve(n);
      /**
       * Data has been written completely, unregistering WriteEvent of this fd
       */
      if (output_buffer_.readableBytes() == 0) {
        channel_->disableWriting();
        if (write_cmpl_cb_) {
          loop_->queueInLoop(std::bind(write_cmpl_cb_, shared_from_this()));
        }
        if (state_ == States::Disconnecting) {
          shutdownInLoop();
        }
      } else { /* if (output_buffer_.readableBytes() == 0) */
        LOG << "I am going to write more data";
      }
    } else { /* if (n > 0) */
      LOG << "TcpConnection::handleWrite";
    }
    /* Write side of this socket has been shutdown() */
  } else { /* if (channel_->isWriting()) */
    LOG << "Connection is down, no more writing";
  }
}

/**
 * CloseCallback of channel_
 */
void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG << "TcpConnection::handleClose state = " << state_;
  assert(state_ == States::Connected || state_ == States::Disconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  channel_->disableAllEvents();
  // must be the last line
  close_cb_(shared_from_this());
}

/**
 * ErrorCallback of channel_
 */
void TcpConnection::handleError() {
  int err = sockets::getSocketError(channel_->getFd());
  LOG << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err
      << " " << strerror_tl(err);
}

/**
 * Registered as CloseCallback by TcpServer and passed during
 * TcpServer::newConnection
 */
void TcpConnection::destroyConnection() {
  loop_->assertInLoopThread();
  assert(state_ == States::Connected || state_ == States::Disconnecting);
  setState(States::Disconnected);
  channel_->disableAllEvents();
  connection_cb_(shared_from_this());

  loop_->removeChannel(channel_.get());
}
