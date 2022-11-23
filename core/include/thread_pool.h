//
// Created by josh on 11/20/22.
//

#pragma once
#include <blockingconcurrentqueue.h>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

namespace df {

class ThreadPool {
public:
    static ThreadPool globalThreadPool;
    explicit ThreadPool(UInt threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();
    DF_NO_MOVE_COPY(ThreadPool);

private:
    static constexpr Int TIMEOUT_U_SEC = 1000 * 1000;
    std::vector<std::thread> workerThreads;
    std::stop_source stop;
    std::condition_variable condVar;
    moodycamel::BlockingConcurrentQueue<std::function<void()>> queue;
    void workerThread(const std::stop_token& stopToken);
};

}   // namespace df