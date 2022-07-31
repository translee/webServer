#include "threadpool.h"
#include <future>
#include <iostream>

ThreadPool::ThreadPool(size_t threadCount)
{   
    auto doAsyncTask = [this]()
    {
        std::unique_lock<std::mutex> locker(this->m_mtx);
		while (true)
		{   
		    if (!this->m_tasks.empty())
		    {	
		    	auto oneTask = std::move(this->m_tasks.front());
				this->m_tasks.pop();
		    	locker.unlock();
				oneTask();
				locker.lock();
		    }
		    else if (this->m_isClosed) 
		    	break;
		    else
		        this->m_cond.wait(locker);
		}
    };
    for (size_t i = 0; i < threadCount; i++)
    {	
        std::thread(doAsyncTask).detach();
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> locker(m_mtx);
		m_isClosed = true;
    }
    m_cond.notify_all();
}
