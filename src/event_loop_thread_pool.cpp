#include "event_loop_thread_pool.h"

#include "event_loop.h"
#include "event_loop_thread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop)
    : base_loop_(base_loop), started_(false), num_threads_(0), next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {
  // Don't delete loop, it's stack variable
}

/**
 * Create @num_threads_ EventLoopThread-s
 */
void EventLoopThreadPool::start() {
  assert(!started_);
  base_loop_->assertInLoopThread();

  started_ = true;

  for (int i = 0; i < num_threads_; ++i) {
    EventLoopThread* t = new EventLoopThread;
    threads_.emplace_back(t);
    loops_.push_back(t->startLoop());
  }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
  base_loop_->assertInLoopThread();
  EventLoop* loop = base_loop_;

  if (!loops_.empty()) {
    /* round-robin */
    loop = loops_[next_];
    ++next_;
    if (static_cast<size_t>(next_) >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}
