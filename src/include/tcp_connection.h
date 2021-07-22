#include <memory>

#include "buffer.h"
#include "callbacks.h"
#include "inet_addr.h"

class Channel;
class EventLoop;
class Socket;
class Buffer;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

/**
 * TCP connection, for both client and server usage.
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
 public:
  /**
   * Constructs a TcpConnection with a connected sockfd
   *
   * User should not create this object.
   */
  TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                const InetAddress& localAddr, const InetAddress& peerAddr);
  ~TcpConnection();

  EventLoop* getLoop() const { return loop_; }

  const std::string& name() const { return name_; }

  const InetAddress& localAddress() { return local_addr_; }

  const InetAddress& peerAddress() { return peer_addr_; }

  bool connected() const { return state_ == States::Connected; }

  void setTcpNoDelay(bool on);

  void setTcpKeepAlive(bool on);

  /* Thread safe */
  void send(const std::string& message);

  /* Thread safe */
  void shutdown();

  /* Callback provided by user, passed in TcpServer::newConnection */
  void setConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }

  void setWriteCallback(const WriteCompleteCallback& cb) {
    write_cmpl_cb_ = cb;
  }

  /* Callback provided by user, passed in TcpServer::newConnection */
  void setMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }

  /* Close calback is TcpServer::removeConnection */
  void setCloseCallback(const CloseCallback& cb) { close_cb_ = cb; }

  /**
   * Internal use only
   *
   * Called when TcpServer accepts a new connection
   */
  void establishConnection();  // should be called only once

  /**
   * Internal use only
   *
   * Called when TcpServer remove me from ConnectionMap
   */
  void destroyConnection();

 private:
  enum class States { Connecting, Connected, Disconnecting, Disconnected };

  void setState(States s) { state_ = s; }

  void handleRead(Timestamp recv_time);

  void handleWrite();

  void handleClose();

  void handleError();

  void sendInLoop(const std::string& message);

  void shutdownInLoop();

  EventLoop* loop_;
  std::string name_;
  States state_;  // FIXME: use atomic variable
  // we don't expose those classes to client.

  /* TcpConnection owns Socket and Channel */
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  InetAddress local_addr_;
  InetAddress peer_addr_;

  /**
   * The following callbacks are computation tasks registered by user, and will
   * be called in channel_->handle*() after it completes I/O
   */

  /* Invoked when this connection is established and destroyed  */
  ConnectionCallback connection_cb_;

  /**
   * Low water callback for output_buffer_
   * Invoked when output_buffer_ is empty
   */
  WriteCompleteCallback write_cmpl_cb_;

  /* Invoked when readable events arrive */
  MessageCallback message_cb_;

  /* Invoked when */
  CloseCallback close_cb_;
  Buffer input_buffer_;
  Buffer output_buffer_;
};

