#include "poller.h"

#include <poll.h>

#include "channel.h"
#include "logging.h"

Poller::Poller(EventLoop* loop) : owner_loop_(loop) {}

Timestamp Poller::poll(int timeoutMs, ChannelList* activeChannels) {
  int numEvents = ::poll(poll_fds_.data(), poll_fds_.size(), timeoutMs);
  Timestamp now(Timestamp::now());
  if (numEvents > 0) {
    LOG << numEvents << " events happened";
    fillActiveChannels(numEvents, activeChannels);
  } else if (numEvents == 0) {
    LOG << " nothing happened";
  } else {
    LOG << "Poller::poll()";
  }
  return now;
}

// Time complexity of poll(2) is O(n) cos it needs to iterate through poll_fds_
// and dispatch events to corresponding Channels
void Poller::fillActiveChannels(int numEvents,
                                ChannelList* activeChannels) const {
  for (PollFdList::const_iterator pfd = poll_fds_.begin();
       pfd != poll_fds_.end() && numEvents > 0; ++pfd) {
    if (pfd->revents > 0) {
      numEvents--;
      // Dispatch fds with operable events to Channels
      ChannelMap::const_iterator ch = channel_map_.find(pfd->fd);
      assert(ch != channel_map_.end());
      Channel* channel = ch->second;
      assert(channel->getFd() == pfd->fd);
      channel->setRevents(pfd->revents);
      activeChannels->push_back(channel);
    }
  }
}

void Poller::updateChannel(Channel* channel) {
  assertInLoopThread();
  LOG << "fd = " << channel->getFd() << " events = " << channel->getEvents();

  if (channel->getIndex() < 0) {
    // New Channel, add it to poll_fds_
    assert(channel_map_.find(channel->getFd()) == channel_map_.end());
    struct pollfd pfd;
    pfd.fd = channel->getFd();
    pfd.events = static_cast<short>(channel->getEvents());
    pfd.revents = 0;
    poll_fds_.push_back(pfd);
    channel->setIndex(poll_fds_.size() - 1);
    channel_map_.emplace(pfd.fd, channel);
  } else {
    // Update an existing Channel
    assert(channel_map_.find(channel->getFd()) != channel_map_.end());
    assert(channel_map_[channel->getFd()] == channel);
    int idx = channel->getIndex();
    assert(idx >= 0 && idx < poll_fds_.size());

    // Update
    struct pollfd& pfd = poll_fds_[idx];
    assert(pfd.fd == channel->getFd() || pfd.fd == -1);
    pfd.events = channel->getEvents();
    pfd.revents = 0;
    if (channel->isNoneEvent()) {
      // Ignore this pollfd
      pfd.fd = -1;
    }
  }
}
