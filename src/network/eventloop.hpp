#pragma once

#include "acceptor.hpp"
// #include "demultiplex.hpp"
#include "group.hpp"

#include <memory>
#include <vector>

namespace reactor {

class EventLoop
{
public:
    using SocketFd = int;

public:
    EventLoop(std::shared_ptr<Acceptor> acceptor, int subloop = 1);
    EventLoop(const EventLoop &) = delete;
    EventLoop(EventLoop &&) = delete;
    EventLoop & operator=(const EventLoop &) = delete;
    EventLoop & operator=(EventLoop &&) = delete;
    ~EventLoop();

public:
    void loop();

    void stop();

    std::shared_ptr<Demultiplex> dmp() noexcept;

    std::vector<std::shared_ptr<Demultiplex>> dmpForLoopGroup() noexcept;

private:
    std::atomic_bool            stop_;
    // std::shared_ptr<Demultiplex> dmp_ptr_;
    LoopThreadGroup             loop_group_;
    std::shared_ptr<Acceptor>   acceptor_;
    std::mutex                  mx_;
};

} // namespace reactor