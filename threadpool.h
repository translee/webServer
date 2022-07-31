/**
 * @class    ThreadPool
 * @author   translee
 * @date     2022/06/05
 * @brief    
 */

#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

class ThreadPool
{
public:
    explicit ThreadPool(size_t threadCount=8);
    ThreadPool(const ThreadPool& rhs) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
    ThreadPool(ThreadPool&& rhs) = default;
    ThreadPool& operator=(ThreadPool&& rhs) = default;
    template <class T>
    void addTask(T&& task)
    {
        {
	    std::lock_guard<std::mutex> locker(m_mtx);
	    m_tasks.emplace(std::forward<T>(task));
	}
	m_cond.notify_one();
    }
    ~ThreadPool();
private:
    std::mutex m_mtx;
    std::condition_variable m_cond;
    bool m_isClosed;
    std::queue<std::function<void()>> m_tasks;
};
