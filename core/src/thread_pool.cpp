//
// Created by josh on 11/20/22.
//

#include "thread_pool.h"

namespace df {
ThreadPool ThreadPool::globalPool;

ThreadPool::ThreadPool(UInt threadCount)
{
    workerThreads.reserve(threadCount);
    for (UInt i = 0; i < threadCount; i++)
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
        std::unique_ptr<ITask> task;
        if (queue.wait_dequeue_timed(task, TIMEOUT_U_SEC))
            task->run();
    }
}

void ThreadPool::VoidTask::run()
{
    try {
        function();
        promise.set_value();
    }
    catch (...) {
        promise.set_exception(std::current_exception());
    }
}
}   // namespace df