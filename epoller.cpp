#include "epoller.h"
#include <iostream>
#include <unistd.h>

Epoller::Epoller(int maxEvent)
    : m_epollFd(epoll_create(1))
    , m_events(maxEvent)
{
}

Epoller::~Epoller()
{
    close(m_epollFd);
}

bool Epoller::addFd(int fd, uint32_t events)
{
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(m_epollFd, EPOLL_CTL_ADD, 
        fd, &ev);
}

bool Epoller::modFd(int fd, uint32_t events)
{
    if (fd < 0)
        return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(m_epollFd, EPOLL_CTL_MOD, 
        fd, &ev);
}

bool Epoller::delFd(int fd)
{
    if (fd < 0)
        return false;
    return 0 == epoll_ctl(m_epollFd, EPOLL_CTL_DEL, 
        fd, nullptr);
}

int Epoller::wait(int timewait)
{
    return epoll_wait(m_epollFd, m_events.data(), 
        static_cast<int>(m_events.size()), timewait);   
}


int Epoller::getEventFd(size_t i) const
{
    if (i > m_events.size())
        return -1;
    else 
        return m_events[i].data.fd;
}

uint32_t Epoller::getEvents(size_t i) const
{
    if (i > m_events.size())
        return 0;
    else 
        return m_events[i].events;
}
