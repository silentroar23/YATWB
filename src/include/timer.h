#pragma once

#include <functional>

#include "macro.h"
#include "timestamp.h"

/**
 * Internal class for a timer event, owned by TImerQueue
 */
class Timer {
 public:
  using TimerCallback = std::function<void()>;
  Timer(const TimerCallback& cb, Timestamp expiration, double interval)
      : callback_(cb),
        expiration_(expiration),
        interval_(interval),
        repeat_(interval > 0) {}

  DISALLOW_COPY(Timer);

  void run() { callback_(); }

  Timestamp getExpiration() { return expiration_; }

  bool isRepeat() { return repeat_; }

  void restart(Timestamp now) {
    if (repeat_) {
      expiration_ = addTime(now, interval_);
    } else {
      expiration_ = Timestamp::invalid();
    }
  }

 private:
  const TimerCallback callback_;
  Timestamp expiration_;
  const double interval_;
  const bool repeat_;
};
