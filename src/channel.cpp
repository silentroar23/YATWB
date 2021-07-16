#include "channel.h"

#include "event_loop.h"
#include "logging.h"

const int Channel::NoneEvent = 0;
const int Channel::ReadEvent = POLLIN || POLLPRI;
const int Channel::WriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), last_events_(-1) {}

void Channel::update() { loop_->updateChannel(this); }

// Use nonblocking I/O
void Channel::handleEvents() {
  if (revents_ & POLLNVAL) {
    LOG << "Channel::handle_events() POLLNVAL";
  }

  if (revents_ & (POLLERR | POLLNVAL)) {
    if (errorHandler_) errorHandler_();
  }

  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (readHandler_) readHandler_();
  }

  if (revents_ & POLLOUT) {
    if (writeHandler_) writeHandler_();
  }
}

