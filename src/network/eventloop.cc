#include "eventloop.hpp"
#include "nw_ctx.hpp"

namespace reactor {

EventLoop::EventLoop(std::shared_ptr<Acceptor> acceptor, int subloop)
    : stop_(false)
    // , dmp_ptr_(std::make_shared<Demultiplex>())
    , loop_group_()
    , acceptor_(acceptor)
{
    // dmp_ptr_->center()->unRegisterPub(dmp_ptr_->id());
    // dmp_ptr_->demultiplexRegister(acceptor_->server());
    loop_group_.init(subloop);
}
    
EventLoop::~EventLoop()
{
    stop_ = true;
    loop_group_.joinAll();
}

void EventLoop::loop()
{
    while (!stop_)
    {
        try {
            int fd = acceptor_->accept();
            loop_group_.registerFd(fd);
        } catch (std::exception & e) {

        }
    }
}

void EventLoop::stop()
{
    stop_ = true;
    loop_group_.joinAll();
}

// std::shared_ptr<Demultiplex> EventLoop::dmp() noexcept
// {
//     return dmp_ptr_;
// }

std::vector<std::shared_ptr<Demultiplex>> EventLoop::dmpForLoopGroup() noexcept
{
    return loop_group_.dmp();
}

} // namespace reactor