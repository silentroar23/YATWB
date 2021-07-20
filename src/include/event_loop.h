#pragma once

#include <sys/_types/_pid_t.h>

#include <atomic>
#include <mutex>
#include <vector>

#include "channel.h"
#include "macro.h"
#include "thread.h"
#include "timer_queue.h"

class Poller;

class EventLoop {
 public:
  using ChannelList = std::vector<Channel*>;
  using Functor = std::function<void()>;

  EventLoop();

  DISALLOW_COPY(EventLoop);

  ~EventLoop();

  void loop();

  // loop() can only run in I/O thread
  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }

  /* If the current thread is the thread that creates this EventLoop, then
   * current thread is the I/O thread */
  bool isInLoopThread() const { return thread_id_ == CurrentThread::tid(); }

  static EventLoop* getEventLoopOfCurrentThread();

  void updateChannel(Channel* channel);

  void removeChannel(Channel* channel);

  /* Should be able to be called in non-I/O thread */
  TimerId runAt(const Timestamp& time, const Timer::TimerCallback& cb) {
    return timer_queue_->addTimer(cb, time, 0.0);
  }

  /* Should be able to be called in non-I/O thread */
  TimerId runAfter(double delay, const Timer::TimerCallback& cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
  }

  /* Should be able to be called in non-I/O thread */
  TimerId runEvery(double interval, const Timer::TimerCallback& cb) {
    Timestamp time(addTime(Timestamp::now(), interval));
    return timer_queue_->addTimer(cb, time, interval);
  }

  void wakeup();

  /**
   * If current thread is I/O thread, run cb immediately, else put cb in one I/O
   * thread's task queue
   */
  void runInLoop(const Functor& cb);

  void queueInLoop(const Functor& cb);

  void quit();

 private:
  void abortNotInLoopThread();

  /* Handle wakeup event */
  void handleRead();

  void doPendingFunctors();

  std::atomic<bool> looping_;
  std::atomic<bool> quit_;
  std::atomic<bool> calling_pending_functors_;
  // The thread creating this EventLoop
  const pid_t thread_id_;
  Timestamp poll_return_time_;
  std::unique_ptr<Poller> poller_;
  std::unique_ptr<TimerQueue> timer_queue_;

  int wakeup_fd_;
  std::unique_ptr<Channel> wakeup_channel_;
  ChannelList active_channels_;

  // Protect pending_functors_
  std::mutex mutex_;
  // Will be called in other threads
  std::vector<Functor> pending_functors_;
};
