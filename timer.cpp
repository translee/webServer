#include "timer.h"
#include <iostream>
#include <cassert>

void TimerManager::addTimer(int fd, int timeout, const TimeoutCallBack& cb) 
{
    assert(fd >= 0);
    if(m_fdToIndex.find(fd) == m_fdToIndex.end())
    {
        size_t i = m_heapTimer.size();
        m_fdToIndex[fd] = i;
        m_heapTimer.push_back({fd, Clock::now() + MS(timeout), cb});
        __upAdjust(i);
    } 
    else 
    {   
        size_t i = m_fdToIndex[fd];
        m_heapTimer[i].expire = Clock::now() + MS(timeout);
        m_heapTimer[i].cb = cb;
        if(!__downAdjust(i, m_heapTimer.size())) {
            __upAdjust(i);
        }
    }
    return;
}

void TimerManager::updateTimer(int fd, int timeout) 
{   
    // assume new timeout is bigger than old
    auto it = m_fdToIndex.find(fd);
    assert(it != m_fdToIndex.end());
    m_heapTimer[it->second].expire = Clock::now() + MS(timeout);
    __downAdjust(m_fdToIndex[fd], m_heapTimer.size());
    return;
}

int TimerManager::getNextHandle() 
{
    __handleTimeoutNode();
    int res = -1;
    if(!m_heapTimer.empty()) 
    {
        auto nextTime = std::chrono::duration_cast<MS>(
            m_heapTimer.front().expire - Clock::now()).count();
        if (nextTime < 0)
            res = 0;
        else
            res = nextTime;
    }
    return res;
}

void TimerManager::__handleTimeoutNode()
{
    while(!m_heapTimer.empty())
    {
        TimerNode node = m_heapTimer.front();
        if(std::chrono::duration_cast<MS>(node.expire - 
            Clock::now()).count() > 0) 
            break;
        node.cb();
        __deleteNode(0);
    }
    return;
}

void TimerManager::__swapNode(size_t i, size_t j) 
{
    assert(i >= 0 && i < m_heapTimer.size());
    assert(j >= 0 && j < m_heapTimer.size());
    std::swap(m_heapTimer[i], m_heapTimer[j]);
    m_fdToIndex[m_heapTimer[i].fd] = i;
    m_fdToIndex[m_heapTimer[j].fd] = j;
    return;
} 

void TimerManager::__deleteNode(size_t index) 
{
    assert(index >= 0 && index < m_heapTimer.size());
    size_t endNode = m_heapTimer.size() - 1;
    if (index < endNode) 
    {
        __swapNode(index, endNode);
        if (!__downAdjust(index, endNode)) 
        {   
            __upAdjust(index);
        }
    }
    m_fdToIndex.erase(m_heapTimer.back().fd);
    m_heapTimer.pop_back();
    return;
}

void TimerManager::__upAdjust(size_t i) 
{
    assert(i >= 0 && i < m_heapTimer.size());
    size_t j = (i - 1) / 2;
    while(j >= 0) 
    {
        if(m_heapTimer[j] < m_heapTimer[i]) { break; }
        __swapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

bool TimerManager::__downAdjust(size_t index, size_t n) 
{
    assert(index >= 0 && index < m_heapTimer.size());
    assert(n >= 0 && n <= m_heapTimer.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && m_heapTimer[j + 1] < m_heapTimer[j]) j++;
        if(m_heapTimer[i] < m_heapTimer[j]) break;
        __swapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}
