#pragma once

#include "macro.h"

class Timer;

/**
 * An opache identifier for canceling timer
 */
class TimerId {
 public:
  TimerId(Timer* timer) : timer_(timer) {}

 private:
  Timer* timer_;
};
