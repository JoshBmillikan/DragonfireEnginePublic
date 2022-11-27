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
    struct ITask {
        virtual void run() = 0;
        virtual ~ITask() = default;
    };
    template<typename T>
    class Task : public ITask {
        std::promise<T> promise;
        std::function<T()> function;

    public:
        Task(std::function<T()>&& func) : function(std::move(func)) {}
        void run() override
        {
            try {
                promise.set_value(function());
            }
            catch (...) {
                promise.set_exception(std::current_exception());
            }
        }
        std::future<T> getFuture() { return promise.get_future(); }
    };

    class VoidTask : public ITask {
        std::promise<void> promise;
        std::function<void()> function;

    public:
        VoidTask(std::function<void()>&& func) : function(std::move(func)) {}
        void run() override;
        std::future<void> getFuture() { return promise.get_future(); }
    };

public:
    explicit ThreadPool(UInt threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();
    DF_NO_MOVE_COPY(ThreadPool);

    template<typename T>
        requires(!std::is_void_v<T>)
    std::future<T> spawn(std::function<T()>&& func)
    {
        auto task = std::make_unique<Task<T>>(std::move(func));
        auto future = task->getFuture();
        queue.enqueue(std::move(task));
        return future;
    }

    template<typename T>
        requires std::is_void_v<T>
    std::future<T> spawn(std::function<T()>&& func)
    {
        auto task = std::make_unique<VoidTask>(std::move(func));
        auto future = task->getFuture();
        queue.enqueue(std::move(task));
        return future;
    }

    static ThreadPool globalPool;

private:
    static constexpr Int TIMEOUT_U_SEC = 1000 * 1000;
    std::vector<std::thread> workerThreads;
    std::stop_source stop;
    std::condition_variable condVar;
    moodycamel::BlockingConcurrentQueue<std::unique_ptr<ITask>> queue;
    void workerThread(const std::stop_token& stopToken);
};

}   // namespace df