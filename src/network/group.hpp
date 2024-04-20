#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "nw_ctx.hpp"
#include "demultiplex.hpp"

namespace reactor {

class LoopThreadGroup
{
public:
    LoopThreadGroup(bool notify = true)
        : stop_()
        , dmpes_()
        , threads_()
        , notify_(notify)
        , iter_()
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
        iter_ = dmpes_.begin();
    }

    std::vector<std::shared_ptr<Demultiplex>> dmp()
    {
        return dmpes_;
    }

    void registerFd(int fd)
    {
        auto i = iter_;
        {
            std::lock_guard<std::mutex> guard(mx_ );
            i = iter_++;
            if (iter_._M_is_end())
                iter_ = dmpes_.begin();
        }
        (*i)->demultiplexRegister(fd);
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
            dmp->demultiplexWait(ctx, notify_);
        }
    }

private:
    std::atomic_bool                                stop_;
    std::vector<std::shared_ptr<Demultiplex>>       dmpes_;
    std::vector<std::thread>                        threads_;
    std::mutex                                      mx_;
    bool                                            notify_;
    std::vector<std::shared_ptr<Demultiplex>>
                                        ::iterator  iter_;
};

} // namespace reactor