#include "reactor.h"

// ── Channel ──

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd) {}

void Channel::enableReading(bool et) {
    events_ = EPOLLIN | (et ? EPOLLET : 0);
    loop_->updateChannel(this);
}

void Channel::disableAll() {
    loop_->removeChannel(this);
}

void Channel::handleEvent(uint32_t revents) {
    if (revents & (EPOLLIN | EPOLLPRI))
        if (readCb_) readCb_();
    if (revents & (EPOLLHUP | EPOLLERR))
        if (closeCb_) closeCb_();
}

// ── EventLoop ──

EventLoop::EventLoop() {
    epfd_ = epoll_create1(0);
    if (epfd_ < 0) {
        std::cerr << "epoll_create1() failed: " << strerror(errno) << std::endl;
        exit(-1);
    }
}

EventLoop::~EventLoop() {
    close(epfd_);
}

void EventLoop::updateChannel(Channel* ch) {
    struct epoll_event ev;
    ev.events   = ch->events();
    ev.data.ptr = ch;
    int op = ch->added() ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_ctl(epfd_, op, ch->fd(), &ev);
    ch->setAdded(true);
}

void EventLoop::removeChannel(Channel* ch) {
    epoll_ctl(epfd_, EPOLL_CTL_DEL, ch->fd(), nullptr);
    ch->setAdded(false);
}

void EventLoop::loop() {
    struct epoll_event events[64];
    while (running_) {
        int n = epoll_wait(epfd_, events, 64, -1);
        if (n < 0) break;
        for (int i = 0; i < n; i++) {
            auto* ch = static_cast<Channel*>(events[i].data.ptr);
            ch->handleEvent(events[i].events);
        }
    }
}
