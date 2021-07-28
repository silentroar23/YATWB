#pragma once

#include <vector>

#include "macro.h"
#include "thread.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool {
 public:
  EventLoopThreadPool(EventLoop* baseLoop);

  DISALLOW_COPY(EventLoopThreadPool);

  ~EventLoopThreadPool();

  void setThreadNum(int numThreads) { num_threads_ = numThreads; }

  void start();

  EventLoop* getNextLoop();

 private:
  EventLoop* base_loop_;
  bool started_;
  int num_threads_;
  int next_;  // always in loop thread
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};
