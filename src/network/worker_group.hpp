#pragma once

#include <cassert>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

#include "nw_ctx.hpp"
#include "demultiplex.hpp"
#include "queue.hpp"

namespace reactor {

class LoopThreadGroup
{
public:
    LoopThreadGroup()
        : stop_()
        , dmpes_()
        , threads_()
    {}
    LoopThreadGroup(const LoopThreadGroup &) = delete;
    LoopThreadGroup(LoopThreadGroup &&) = delete;
    LoopThreadGroup & operator=(const LoopThreadGroup &) = delete;
    LoopThreadGroup & operator=(LoopThreadGroup &&) = delete;

    ~LoopThreadGroup()
    {
        joinAll();
    }

    void init(int n)
    {
        assert(n >= 1);
        for (int i = 0; i < n; i++)
        {
            dmpes_.emplace_back(std::make_shared<Demultiplex>());
            threads_.emplace_back(std::bind(&LoopThreadGroup::run, this, dmpes_[i]));
        }
    }

    std::vector<std::shared_ptr<Demultiplex>> dmp()
    {
        return dmpes_;
    }

    void registerFd(int fd)
    {
        auto cnt = dmpes_.size();
        dmpes_[fd % cnt]->demultiplexRegister(fd);
    }

    void joinAll()
    {
        for (auto & t : threads_)
        {
            if (t.joinable())
                t.join();
        }
    }

    void stop()
    {
        stop_.store(true, std::memory_order_release);
    }

private:
    void run(std::shared_ptr<Demultiplex> dmp)
    {
        std::shared_ptr<::pubsub::Context> ctx = std::make_shared<context::DmpWaitContext>(::pubsub::DMPWAIT);
        while (!stop_.load(std::memory_order_relaxed))
        {
            std::cout << "thread_id: " << std::this_thread::get_id()
                    //   << ", Dmp Object " << dmp.get()
                      << ", count: " << dmp.use_count() << "\n";
            dmp->demultiplexWait(ctx);
        }
    }

private:
    std::atomic_bool                                stop_;
    std::vector<std::shared_ptr<Demultiplex>>       dmpes_;
    std::vector<std::thread>                        threads_;
};


class WorkerGroup
{
public:
    // WorkerGroup(size_t n_threads = std::thread::hardware_concurrency())
    WorkerGroup(size_t n_threads)
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
    queue::LockedQueue<std::function<void()>>   task_queue_;
};

} // namespace reactor