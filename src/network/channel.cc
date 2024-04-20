#include "channel.hpp"
#include "nw_ctx.hpp"

namespace reactor {

Channel::Channel(int fd, std::shared_ptr<Demultiplex> dmp) noexcept
    : leave_(false)
    , socket_(fd)
    , dmp_ptr_(dmp)
    {}

Channel::~Channel() { if (!leave_) leave(); }


int Channel::socket() noexcept
{
    return socket_;
}

std::shared_ptr<Demultiplex> Channel::dmp() noexcept
{
    return dmp_ptr_;
}

void Channel::leave()
{
    leave_ = true;
    auto del = new context::DmpDeleteContext(::pubsub::DMPDELETE);
    del->fd_ = socket_;
    std::shared_ptr<::pubsub::Context> ctx(del);
    // FIXME: ? Is it really not necessary to notify subscribers (TcpServer)
    // to remove TcpConnection object cached in it?
    dmp_ptr_->demultiplexRemove(ctx);
}

bool Channel::isLeave() noexcept
{
    return leave_;
}

void Channel::enableRead(std::shared_ptr<::pubsub::Context> ctx)
{
    auto dmp_ctx = std::dynamic_pointer_cast<context::DmpModifyContext>(ctx);
    dmp_ctx->fd_ = socket_;
    dmp_ctx->events_ |= Demultiplex::READABLE;
    dmp_ptr_->demultiplexModify(ctx);
}

void Channel::enableWrite(std::shared_ptr<::pubsub::Context> ctx)
{
    auto dmp_ctx = std::dynamic_pointer_cast<context::DmpModifyContext>(ctx);
    dmp_ctx->fd_ = socket_;
    dmp_ctx->events_ |= Demultiplex::WRITABLE;
    dmp_ptr_->demultiplexModify(ctx);
}

void Channel::disableRead(std::shared_ptr<::pubsub::Context> ctx)
{
    auto dmp_ctx = std::dynamic_pointer_cast<context::DmpModifyContext>(ctx);
    dmp_ctx->fd_ = socket_;
    dmp_ctx->events_ &= (~Demultiplex::READABLE);
    dmp_ptr_->demultiplexModify(ctx);
}

void Channel::disableWrite(std::shared_ptr<::pubsub::Context> ctx)
{
    auto dmp_ctx = std::dynamic_pointer_cast<context::DmpModifyContext>(ctx);
    dmp_ctx->fd_ = socket_;
    // dmp_ctx->events_ &= (~Demultiplex::WRITABLE);
    dmp_ctx->events_ = Demultiplex::DefaultEvents;
    dmp_ptr_->demultiplexModify(ctx);
}

} // namespace reactor