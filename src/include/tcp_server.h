#include <map>

#include "callbacks.h"
#include "macro.h"
#include "tcp_connection.h"

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

/**
 * Directly managed by the user
 */
class TcpServer {
 public:
  TcpServer(EventLoop* loop, const InetAddress& listenAddr);

  DISALLOW_COPY(TcpServer);

  ~TcpServer();  // force out-line dtor, for unique_ptr members.

  /**
   * Set the number of threads for handling input.
   *
   * Always accepts new connection in loop's thread.
   * Must be called before @c start
   * @param numThreads
   * - 0 means all I/O in loop's thread, no thread will created.
   * this is the default value.
   * - 1 means all I/O in another thread.
   * - N means a thread pool with N threads, new connections
   * are assigned on a round-robin basis.
   */
  void setThreadNum(int num_threads);
  /**
   * Starts the server if it's not listening.
   *
   * It's harmless to call it multiple times.
   * Thread safe.
   */
  void start();

  /**
   * Set connection callback. This callback will be passed TcpConnection
   * Not thread safe.
   */
  void setConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }

  /**
   * Set message callback. This callback will be passed TcpConnection
   * Not thread safe.
   */
  void setMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }

  void setWriteCallback(const WriteCompleteCallback& cb) {
    write_cmpl_cb_ = cb;
  }

 private:
  /* Not thread safe, but in loop */
  void newConnection(int sockfd, const InetAddress& peerAddr);

  /* Not thread safe, but in loop */
  void removeConnection(const TcpConnectionPtr& conn);

  void removeConnectionInLoop(const TcpConnectionPtr& conn);

  using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

  /* The acceptor loop */
  EventLoop* loop_;
  const std::string name_;

  /**
   * Acceptor does not know the existence of TcpServer. Avoid revealing Acceptor
   */
  std::unique_ptr<Acceptor> acceptor_;
  std::unique_ptr<EventLoopThreadPool> thread_pool_;
  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;
  WriteCompleteCallback write_cmpl_cb_;
  bool started_;
  int next_conn_id_;  // always in loop thread
  ConnectionMap connections_;
};

