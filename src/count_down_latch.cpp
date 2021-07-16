#include <mutex>

#include "countdown_latch.h"

CountDownLatch::CountDownLatch(int count) : count_(count) {}

void CountDownLatch::wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (count_ > 0) cv_.wait(lock);
}

void CountDownLatch::countDown() {
  std::unique_lock<std::mutex> lock(mutex_);
  --count_;
  if (count_ == 0) cv_.notify_all();
}
