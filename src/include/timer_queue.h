#pragma once

#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "channel.h"
#include "macro.h"
#include "timer.h"
#include "timer_id.h"

/**
 * A best efforts timer queue
 *
 * No guarantee that the callbacks will be called on time
 */

class EventLoop;

class TimerQueue {
 public:
  class TimerEntryCmp {
    // Cannot use auto inside class, so make TimerEntryCmp a function object
    bool operator()(const std::unique_ptr<Timer>& l,
                    const std::unique_ptr<Timer>& r) {
      return l->getExpiration() < r->getExpiration();
    }
  };

  using TimerEntry = std::pair<Timestamp, std::unique_ptr<Timer>>;
  using TimerSet = std::set<TimerEntry, TimerEntryCmp>;

  TimerQueue(EventLoop* loop);

  DISALLOW_COPY(TimerQueue);

  ~TimerQueue();

  // Schedule the callback to be run at a given time, repeat if interval > 0.
  // Must be thread safe. Usually called from non-I/O threads
  TimerId addTimer(const Timer::TimerCallback& cb, Timestamp when,
                   double interval);

  void cancel(TimerId timer_id);

 private:
  void addTimerInLoop(std::unique_ptr<Timer>& timer);

  // Clear all expired timers
  std::vector<TimerEntry> getExpired(Timestamp now);

  void reset(std::vector<TimerEntry>& expired, Timestamp now);

  // Pass by value
  bool insert(std::unique_ptr<Timer> timer);

  // Called when timerfd alarms
  void handleRead();

  EventLoop* loop_;
  const int timerfd_;
  Channel timerfd_channel_;

  // Timer set sorted by expiration
  TimerSet timers_;
};
