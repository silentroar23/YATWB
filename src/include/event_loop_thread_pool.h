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

  void setThreadNum(int numThreads) { numThreads_ = numThreads; }

  void start();

  EventLoop* getNextLoop();

 private:
  EventLoop* baseLoop_;
  bool started_;
  int numThreads_;
  int next_;  // always in loop thread
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};
