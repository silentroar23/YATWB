#pragma once

#include <functional>

#include "macro.h"

class EventLoop;
class Timestamp;

/**
 * A selectable I/O channel
 *
 * This class doesn't own the file descriptor. The file descriptor could be a
 * socket, an eventfd, a timerfd, or a signalfd
 */

class Channel {
 public:
  using Callback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  Channel(EventLoop* loop, int fd);

  DISALLOW_COPY(Channel);

  ~Channel();

  int getFd() { return fd_; }

  int getEvents() { return events_; }

  void setRevents(int revents) { revents_ = revents; }

  /* For removing this Channel from Poller::poll_fds_ in O(1) */
  int getIndex() { return index_; }

  /* For removing this Channel from Poller::poll_fds_ in O(1) */
  void setIndex(int index) { index_ = index; }

  void handleEvents(Timestamp recv_time);

  bool isNoneEvent() { return events_ == NoneEvent; }

  EventLoop* getOwnerLoop() { return owner_loop_; }

  void setOwnerLoop(EventLoop* owner_loop) { owner_loop_ = owner_loop; }

  void setReadCallback(const ReadEventCallback& cb) { read_cb_ = cb; }

  void setWriteCallback(const Callback& cb) { write_cb_ = cb; }

  void setErrorCallback(const Callback& cb) { error_cb_ = cb; }

  void setCloseCallback(const Callback& cb) { close_cb_ = cb; }

  /* Set ReadEvent and register this channel to Poller::poll_fds_ */
  void enableReading() {
    events_ |= ReadEvent;
    update();
  }

  /* Set WriteEvent and register this channel to Poller::poll_fds_ */
  void enableWriting() {
    events_ |= WriteEvent;
    update();
  }

  void disableReading() {
    events_ &= ~ReadEvent;
    update();
  }

  void disableWriting() {
    events_ &= ~WriteEvent;
    update();
  }

  void disableAllEvents() {
    events_ = NoneEvent;
    update();
  }

  bool isWriting() { return events_ & WriteEvent; }

 private:
  void update();

  ReadEventCallback read_cb_;
  Callback write_cb_;
  Callback error_cb_;
  Callback close_cb_;

  static const int NoneEvent;
  static const int ReadEvent;
  static const int WriteEvent;

  EventLoop* loop_;
  const int fd_;
  int events_;
  int revents_;

  /**
   * This Channel's index in ChannelList
   * For removing this Channel from Poller::poll_fds_ in O(1)
   */
  int index_;

  EventLoop* owner_loop_;
  bool events_handling_;
};
