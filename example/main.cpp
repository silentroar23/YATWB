#include <unistd.h>

#include "echo.h"
#include "event_loop.h"
#include "logging.h"

int main() {
  LOG << "pid = " << getpid();
  EventLoop loop;
  InetAddress listenAddr(2007);
  EchoServer server(&loop, listenAddr);
  server.start();
  loop.loop();
}

