#pragma once
#include <condition_variable>
#include <mutex>

#include "macro.h"
#include "thread.h"

class EventLoop;

class EventLoopThread {
 public:
  EventLoopThread();

  DISALLOW_COPY(EventLoopThread);

  ~EventLoopThread();

  EventLoop* startLoop();

 private:
  void IOThreadFunc();

  EventLoop* loop_;
  bool exiting_;
  /* A new thread created by EventLoopThread */
  Thread thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
};
