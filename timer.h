/**
 * @class   TimerManager
 * @author  github.com/translee
 * @date    2022/06/07
 * @brief   
 */

#pragma once
#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>

using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;

struct TimerNode
{
    int fd;
    TimeStamp expire;
    TimeoutCallBack cb;
    bool operator<(const TimerNode& rhs)
    { return this->expire < rhs.expire; }
};

class TimerManager
{
public:
    TimerManager() { m_heapTimer.reserve(64); }
    ~TimerManager() = default;
    void addTimer(int fd, int timeout, const TimeoutCallBack& cb);
    void updateTimer(int fd, int timeout);
    int getNextHandle();
private:
    void __handleTimeoutNode();
    void __swapNode(size_t i, size_t j);
    void __deleteNode(size_t i);
    void __upAdjust(size_t i);
    bool __downAdjust(size_t index, size_t n);
private:
    std::vector<TimerNode> m_heapTimer;
    std::unordered_map<int, size_t> m_fdToIndex;
};
