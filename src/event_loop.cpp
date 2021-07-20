#include "event_loop.h"

#include <unistd.h>

#include <mutex>

#include "logging.h"
#include "poller.h"

/* Used to check if current thread only has one EventLoop */
thread_local EventLoop* event_loop_in_this_thread = nullptr;
const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    : looping_(false), quit_(true), thread_id_(CurrentThread::tid()) {
  LOG << "EventLoop created" << this << " in thread" << thread_id_;

  if (event_loop_in_this_thread) {
    LOG << "Another EventLoop " << event_loop_in_this_thread
        << " exists in this thread " << thread_id_;
  } else {
    event_loop_in_this_thread = this;
  }
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
  return event_loop_in_this_thread;
}

void EventLoop::loop() {
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;

  while (!quit_) {
    active_channels_.clear();
    poller_->poll(kPollTimeMs, &active_channels_);
    for (auto it = active_channels_.begin(); it != active_channels_.end();
         ++it) {
      (*it)->handleEvents();
    }

    doPendingFunctors();
  }

  LOG << "EventLoop " << this << " stops looping";
  looping_ = false;
}

void EventLoop::updateChannel(Channel* channel) {
  assert(channel->getOwnerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

/* Wake up I/O threads by writing 8 bytes on wakeup_fd_ */
void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = write(wakeup_fd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = read(wakeup_fd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::runInLoop(const Functor& cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(const Functor& cb) {
  std::lock_guard<std::mutex> lock(mutex_);
  pending_functors_.push_back(cb);

  /**
   * Wake up I/O threads in the following scenarios
   * 1. Current thread is not I/O thread
   * 2. Current thead is I/O thread and is executing doPendingFunctors().
   * Note: if current thread is already invoked doPendingFunctors(), also need
   * to wakeup() cos queueInLoop may be called in functors of pending_functors_.
   * Only when queueInLoop is called in handleEvents() wakeup() need not be
   * called, cos all push_back()-ed cbs will be called in doPendingFunctors() in
   * the same iteration of the loop
   */
  if (!isInLoopThread() || calling_pending_functors_) {
    wakeup();
  }
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  calling_pending_functors_ = true;

  std::lock_guard<std::mutex> lock(mutex_);
  functors.swap(pending_functors_);

  for (size_t i = 0; i < functors.size(); ++i) {
    functors[i]();
  }
  calling_pending_functors_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  // If current thread is not I/O thread, wake up the I/O thread to handle tasks
  // this thread has assigned
  if (!isInLoopThread()) {
    wakeup();
  }
}

EventLoop::~EventLoop() {
  assert(!looping_);
  event_loop_in_this_thread = nullptr;
}
