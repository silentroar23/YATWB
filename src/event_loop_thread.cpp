#include "event_loop_thread.h"

#include <mutex>

#include "event_loop.h"

EventLoopThread::EventLoopThread()
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::IOThreadFunc, this)) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  loop_->quit();
  thread_.join();
}

EventLoop* EventLoopThread::startLoop() {
  assert(!thread_.started());
  thread_.start();

  std::unique_lock<std::mutex> lock(mutex_);
  // Wait until an EvenLoop is created in ThreadFunc
  cv_.wait(lock, [&] { return loop_ != nullptr; });

  return loop_;
}

void EventLoopThread::IOThreadFunc() {
  // This loop is invalid when IOThreadFunc exits
  EventLoop loop;

  // Scope so the lock is freed before calling loop.loop()_
  {
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = &loop;
    cv_.notify_one();
  }

  loop.loop();
  // assert(exiting_);
}
