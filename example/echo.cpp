#include "echo.h"

#include "logging.h"
#include "thread.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

EchoServer::EchoServer(EventLoop* loop, const InetAddress& listenAddr)
    : server_(loop, listenAddr) {
  server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&EchoServer::onMessage, this, _1, _2, _3));
}

void EchoServer::start() { server_.start(); }

void EchoServer::onConnection(const TcpConnectionPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): tid=%d new connection [%s] from %s\n",
           CurrentThread::tid(), conn->name().c_str(),
           conn->peerAddress().toHostPort().c_str());
  } else {
    printf("onConnection(): tid=%d connection [%s] is down\n",
           CurrentThread::tid(), conn->name().c_str());
  }
}

void EchoServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf,
                           Timestamp recv_time) {
  printf("onMessage(): tid=%d received %zd bytes from connection [%s] at %s\n",
         CurrentThread::tid(), buf->readableBytes(), conn->name().c_str(),
         recv_time.toFormattedString().c_str());
  conn->send(buf->retrieveAsString());
}

