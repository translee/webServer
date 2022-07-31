/**
 * @class   Epoller
 * @author  github.com/translee
 * @date    2022/06/07
 * @brief   
 */

#pragma once
#include "vector"
#include "sys/epoll.h"

class Epoller
{
public:
    explicit Epoller(int maxEvent=1024);
    ~Epoller();
    bool addFd(int fd, uint32_t events);
    bool modFd(int fd, uint32_t events);
    bool delFd(int fd);
    int wait(int timewait=-1);
    int getEventFd(size_t i) const;
    uint32_t getEvents(size_t i) const;
private:
    int m_epollFd;
    std::vector<epoll_event> m_events;
};
