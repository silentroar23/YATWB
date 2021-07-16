#pragma once

#include <functional>

#include "macro.h"

class EventLoop;

/**
 * A selectable I/O channel
 *
 * This class doesn't own the file descriptor. The file descriptor could be a
 * socket, an eventfd, a timerfd, or a signalfd
 */

class Channel {
 public:
  using Callback = std::function<void()>;

  Channel(EventLoop* loop, int fd);

  DISALLOW_COPY(Channel);

  int getFd() { return fd_; }

  int getEvents() { return events_; }

  void setRevents(int revents) { revents_ = revents; }

  int getIndex() { return index_; }

  void setIndex(int index) { index_ = index; }

  void handleEvents();

  bool isNoneEvent() { return events_ == NoneEvent; }

  EventLoop* getOwnerLoop() { return owner_loop_; }

  void setOwnerLoop(EventLoop* owner_loop) { owner_loop_ = owner_loop; }

  void setReadHandler(const Callback& cb) { readHandler_ = cb; }

  void setWriteHandler(const Callback& cb) { writeHandler_ = cb; }

  void setErrorHandler(const Callback& cb) { errorHandler_ = cb; }

  // Set ReadEvent and register this channel to Poller::poll_fds_
  void enableReading() {
    events_ |= ReadEvent;
    update();
  }

 private:
  void update();

  Callback readHandler_;
  Callback writeHandler_;
  Callback errorHandler_;

  static const int NoneEvent;
  static const int ReadEvent;
  static const int WriteEvent;

  EventLoop* loop_;
  const int fd_;
  int events_;
  int revents_;
  // This Channel's index in ChannelList
  int index_;

  EventLoop* owner_loop_;
};
