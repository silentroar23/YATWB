#include <map>
#include <vector>

#include "event_loop.h"
#include "macro.h"
#include "timestamp.h"

struct pollfd;

class Channel;

/**
 * I/O multiplexing with poll(2)
 *
 * This class doesn't own the Channel objects
 */

class Poller {
 public:
  using ChannelList = std::vector<Channel*>;
  using PollFdList = std::vector<struct pollfd>;
  using ChannelMap = std::map<int, Channel*>;

  Poller(EventLoop* loop);

  DISALLOW_COPY(Poller);

  ~Poller() = default;

  /**
   * Poll the I/O events
   * Must be called in the loop thread
   */
  Timestamp poll(int timeoutMs, ChannelList* activeChannels);

  /**
   * Change the interested I/O events
   * Must be called in the loop thread
   */
  void updateChannel(Channel* channel);

  /**
   * Remove the channel
   * Must be called in the loop thread
   */
  void removeChannel(Channel* channel);

  void assertInLoopThread() { owner_loop_->assertInLoopThread(); }

 private:
  void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

  EventLoop* owner_loop_;
  PollFdList poll_fds_;
  ChannelMap channel_map_;
};
