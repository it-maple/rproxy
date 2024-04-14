#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <future>
#include <thread>

#include "queue.hpp"

namespace pubsub {

class WorkerGroup
{
public:
    WorkerGroup(size_t n_threads = std::thread::hardware_concurrency())
        : stop_(false)
        , workers_()
        , task_queue_()
    {
        for (size_t i = 0; i < n_threads; i++)
            workers_.emplace_back([this] {
                while (!stop_.load(std::memory_order_acquire))
                {
                    task_queue_.pop()();
                }
            });
    }

    ~WorkerGroup()
    {
        for (auto & w : workers_)
        {
            if (w.joinable())
                w.join();
        }
    }

    void submit(std::function<void()> const & f)
    {
        task_queue_.push(f);
    }

    template<typename Func, typename... Args>
    auto submit(Func && f, Args && ...args) ->
        std::future<decltype(func(args...))>
    {
        auto task = std::bind(std::forward(f), std::forward(args)...);
        std::packaged_task<decltype(f(args...))> packaged_task(task);
        task_queue_.push([&packaged_task] { packaged_task(); });

        return packaged_task.get_future();
    }

    void stop()
    {
        stop_.store(true, std::memory_order_release);
    }

private:
    std::atomic_bool                            stop_;
    std::vector<std::thread>                    workers_;
    ::reactor::queue::LockedQueue<
                std::function<void()>>          task_queue_;
};

} // namespace reactor