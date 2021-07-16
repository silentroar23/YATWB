#include "timer_queue.h"

#include <sys/timerfd.h>
#include <unistd.h>

#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "event_loop.h"
#include "logging.h"

int createTimerfd() {
  int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0) {
    LOG << "Failed in timerfd_create";
  }
  return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when) {
  int64_t microseconds =
      when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
  if (microseconds < 100) {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec =
      static_cast<time_t>(microseconds / Timestamp::microSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(
      (microseconds % Timestamp::microSecondsPerSecond) * 1000);
  return ts;
}

void readTimerfd(int timerfd, Timestamp now) {
  uint64_t howmany;
  ssize_t n = read(timerfd, &howmany, sizeof howmany);
  LOG << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
  if (n != sizeof howmany) {
    LOG << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
  }
}

void resetTimerfd(int timerfd, Timestamp expiration) {
  // wake up loop by timerfd_settime()
  struct itimerspec newValue;
  struct itimerspec oldValue;
  bzero(&newValue, sizeof newValue);
  bzero(&oldValue, sizeof oldValue);
  newValue.it_value = howMuchTimeFromNow(expiration);
  int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
  if (ret) {
    LOG << "timerfd_settime()";
  }
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfd_channel_(loop, timerfd_),
      timers_() {
  timerfd_channel_.setReadHandler(std::bind(&TimerQueue::handleRead, this));
  // we are always reading the timerfd, we disarm it with timerfd_settime.
  timerfd_channel_.enableReading();
}

// Timer object is auto managed by unique_ptr
TimerQueue::~TimerQueue() { close(timerfd_); }

TimerId TimerQueue::addTimer(const Timer::TimerCallback& cb, Timestamp when,
                             double interval) {
  auto timer = std::make_unique<Timer>(cb, when, interval);
  loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
  return TimerId(timer.get());
}

void TimerQueue::addTimerInLoop(std::unique_ptr<Timer>& timer) {
  // Can only add timer event in I/O thread
  loop_->assertInLoopThread();
  bool earliest_to_alarm = insert(std::move(timer));

  if (earliest_to_alarm) {
    resetTimerfd(timerfd_, timer->getExpiration());
  }
}

std::vector<TimerQueue::TimerEntry> TimerQueue::getExpired(Timestamp now) {
  std::vector<TimerEntry> expired;
  Timer* ptr = reinterpret_cast<Timer*>(UINTPTR_MAX);
  auto sentry = std::make_pair(now, std::unique_ptr<Timer>(ptr));
  // Get the iterator pointing to the first Timer that has nor expired
  auto it = timers_.lower_bound(sentry);
  assert(it == timers_.end() || now < it->first);
  // Expired Timers now owned by expired
  std::copy(timers_.begin(), it, std::back_inserter(expired));
  timers_.erase(timers_.begin(), it);
  return expired;
}

void TimerQueue::reset(std::vector<TimerEntry>& expired, Timestamp now) {
  Timestamp next_expire;

  for (auto it = expired.begin(); it != expired.end(); ++it) {
    if (it->second->isRepeat()) {
      it->second->restart(now);
      insert(std::move(it->second));
    } else {
      it->second.reset();
    }
  }

  if (!timers_.empty()) {
    next_expire = timers_.begin()->second->getExpiration();
  }

  if (next_expire.valid()) {
    resetTimerfd(timerfd_, next_expire);
  }
}

bool TimerQueue::insert(std::unique_ptr<Timer> timer) {
  bool earliest_to_alarm = false;
  Timestamp when = timer->getExpiration();
  auto it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliest_to_alarm = true;
  }
  // Timer is owned by timers_
  auto result = timers_.insert(std::make_pair(when, std::move(timer)));
  assert(result.second);
  return earliest_to_alarm;
}
