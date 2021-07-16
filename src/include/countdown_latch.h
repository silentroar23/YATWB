#pragma once

#include <condition_variable>
#include <mutex>

#include "macro.h"

// Ensure func has started
class CountDownLatch {
 public:
  explicit CountDownLatch(int count);
  DISALLOW_COPY(CountDownLatch);
  void wait();
  void countDown();

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  int count_;
};
