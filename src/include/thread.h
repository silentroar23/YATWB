#pragma once

#include <sys/syscall.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <string>

#include "countdown_latch.h"

class Thread {
 public:
  using ThreadFunc = std::function<void()>;
  explicit Thread(const ThreadFunc&, const std::string& name = std::string());

  DISALLOW_COPY(Thread);

  ~Thread();

  /* Create a new thread */
  void start();

  int join();

  bool started() const { return started_; }

  pid_t tid() const { return tid_; }

  const std::string& name() const { return name_; }

 private:
  void setDefaultName();
  bool started_;
  bool joined_;
  pthread_t pthread_id_;
  pid_t tid_;
  ThreadFunc func_;
  std::string name_;
  CountDownLatch latch_;
  static std::atomic<int> num_created_;
};

// internal use
namespace CurrentThread {
// Cache tid to prevent traping into kernel every time we want to use it
extern thread_local int t_cachedTid;
extern thread_local const char* t_threadName;

pid_t tid();

// For logging
const char* name();

bool isMainThread();
}  // namespace CurrentThread

