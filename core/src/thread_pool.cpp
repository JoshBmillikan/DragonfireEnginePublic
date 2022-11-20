//
// Created by josh on 11/20/22.
//

#include "thread_pool.h"

namespace df {
ThreadPool ThreadPool::globalThreadPool;

ThreadPool::ThreadPool(UInt threadCount)
{
    workerThreads.reserve(threadCount);
    for(UInt i=0;i<threadCount;i++)
        workerThreads.emplace_back(&ThreadPool::workerThread, this, stop.get_token());
}

ThreadPool::~ThreadPool()
{
    stop.request_stop();
    for (auto& thread : workerThreads)
        thread.join();
}

void ThreadPool::workerThread(const std::stop_token& stopToken)
{
    while (!stopToken.stop_requested()) {
        std::function<void()> fn;

        if (queue.wait_dequeue_timed(fn, TIMEOUT_U_SEC))
            fn();
    }
}
}   // namespace df