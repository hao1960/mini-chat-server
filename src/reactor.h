#pragma once

#include <functional>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

class EventLoop;

class Channel {
public:
    Channel(EventLoop* loop, int fd);

    void setReadCallback(std::function<void()> cb)  { readCb_ = std::move(cb); }
    void setCloseCallback(std::function<void()> cb) { closeCb_ = std::move(cb); }
    void enableReading(bool et = true);
    void disableAll();
    void handleEvent(uint32_t revents);

    int fd() const { return fd_; }

private:
    EventLoop* loop_;
    int fd_;
    uint32_t events_ = 0;
    std::function<void()> readCb_;
    std::function<void()> closeCb_;
    bool added_ = false;

    friend class EventLoop;
    uint32_t events() const   { return events_; }
    bool added() const        { return added_; }
    void setAdded(bool v)     { added_ = v; }
};

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void updateChannel(Channel* ch);
    void removeChannel(Channel* ch);
    void loop();
    void quit() { running_ = false; }

private:
    int epfd_;
    bool running_ = true;
};
