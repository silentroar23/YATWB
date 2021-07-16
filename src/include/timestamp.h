#pragma once

#include <stdint.h>

#include <string>

/**
 * Timestamp in UTC, in microseconds resolution.
 * Only works for time after the Epoch.
 *
 * This class is immutable.
 * It's recommended to pass it by value, since it's passed in register on x64.
 */

class Timestamp {
 public:
  // Constuct an invalid Timestamp.
  Timestamp();

  // Constuct a Timestamp at specific time
  // @param microSecondsSinceEpoch
  explicit Timestamp(int64_t microSecondsSinceEpoch);

  void swap(Timestamp &that) {
    std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
  }

  // Default copy/assignment/dtor are okay

  std::string toString() const;
  std::string toFormattedString() const;

  bool valid() const { return microSecondsSinceEpoch_ > 0; }

  // For internal usage.
  int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

  // Get time of now.
  static Timestamp now();

  // Get an invalid time.
  static Timestamp invalid();

  static const int microSecondsPerSecond = 1000 * 1000;

 private:
  int64_t microSecondsSinceEpoch_;
};

inline bool operator<(const Timestamp &lhs, const Timestamp &rhs) {
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(const Timestamp &lhs, const Timestamp &rhs) {
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

/**
 * Gets time difference of two timestamps, result in seconds.
 *
 * @param high, low
 * @return (high-low) in seconds
 * @c double has 52-bit precision, enough for one-microseciond
 * resolution for next 100 years.
 */
inline double timeDifference(Timestamp high, Timestamp low) {
  int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
  return static_cast<double>(diff) / Timestamp::microSecondsPerSecond;
}

/**
 * Add @c seconds to given timestamp.
 *
 * @return timestamp+seconds as Timestamp
 */
inline Timestamp addTime(Timestamp timestamp, double seconds) {
  int64_t delta =
      static_cast<int64_t>(seconds * Timestamp::microSecondsPerSecond);
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

