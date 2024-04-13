#include <stdexcept>
#include <unistd.h>

#include "demultiplex.hpp"
#include "nw_ctx.hpp"
#include "stream_socket.hpp"

namespace reactor {

Demultiplex::Demultiplex()
    : id_(::pubsub::ID::pub_id())
    , dmpfd_(-1)
    , notify_(true)
    , center_(::pubsub::PubSubCenter::instance())
{
    if ((dmpfd_ = ::epoll_create(MAX_SIZE)) < 0)
        throw std::runtime_error("can not create epoll instance");

    center_->registerPub(id_, this);
}

Demultiplex::~Demultiplex() 
{
    if (dmpfd_ != -1)
    {
        ::close(dmpfd_);
        center_->unRegisterPub(id_);
    }
}

void Demultiplex::enableNotify(bool enabled)
{
    notify_ = enabled;
}

uint16_t Demultiplex::pubID() const
{
    return id_;
}

void Demultiplex::notify(::pubsub::PubType type, const std::shared_ptr<::pubsub::Context> ctx)
{
    if (notify_)
        center_->notifySubscriber(id_, type, ctx);
}

std::shared_ptr<::pubsub::PubSubCenter> Demultiplex::center() noexcept
{
    return center_;
}

bool Demultiplex::demultiplexAccepted()
{
    struct epoll_event eps[1];
    auto ret = ::epoll_wait(dmpfd_, eps, 1, -1);

    if (ret < 0)
        throw std::logic_error("epoll_wait() error");

    return ret;
}

int Demultiplex::demultiplexRegister(int sockfd, bool notify)
{
    StreamSocket::setNonBlocking(sockfd);

    auto regiser_ctx = new context::DmpRegisterContext(::pubsub::DMPREGISTER);
    regiser_ctx->fd_ = sockfd;
    regiser_ctx->events_ = DefaultEvents;
    regiser_ctx->dmp_ptr_ = shared_from_this();

    std::shared_ptr<::pubsub::Context> ctx(regiser_ctx);

    if ((!notify_ && notify) || (notify_ && notify))
        center_->notifySubscriber(id_, ::pubsub::DMPREGISTER, ctx);

    struct epoll_event event;
    event.data.fd = sockfd;
    event.events = DefaultEvents;

    return ::epoll_ctl(dmpfd_, EPOLL_CTL_ADD, event.data.fd, &event);
}

int Demultiplex::demultiplexModify(std::shared_ptr<::pubsub::Context> ctx, bool notify)
{
    ctx->event_type_ = ::pubsub::DMPMODIFY;
    auto ptr = std::dynamic_pointer_cast<context::DmpModifyContext>(ctx);

    if ((!notify_ && notify) || (notify_ && notify))
        center_->notifySubscriber(id_, ::pubsub::DMPMODIFY, ctx);

    struct epoll_event event;
    event.data.fd = ptr->fd_;
    event.events = ptr->events_;

    return ::epoll_ctl(dmpfd_, EPOLL_CTL_MOD, ptr->fd_, &event);
}

int Demultiplex::demultiplexRemove(std::shared_ptr<::pubsub::Context> ctx, bool notify)
{
    ctx->event_type_ = ::pubsub::DMPDELETE;
    auto ptr = std::dynamic_pointer_cast<context::DmpDeleteContext>(ctx);

    if ((!notify_ && notify) || (notify_ && notify))
        center_->notifySubscriber(id_, ::pubsub::DMPDELETE, ctx);

    return ::epoll_ctl(dmpfd_, EPOLL_CTL_DEL, ptr->fd_, nullptr);
}

void Demultiplex::demultiplexWait(std::shared_ptr<::pubsub::Context> ctx, bool notify)
{
    auto dmpCtx = std::dynamic_pointer_cast<context::DmpWaitContext>(ctx);

    static const int EVENT_SIZE = 1024;
    struct epoll_event eps[EVENT_SIZE];

    auto resp_count = ::epoll_wait(dmpfd_, eps, EVENT_SIZE, -1);
    if (resp_count <= 0)
        return;

    std::vector<context::DmpWaitContext::resp> resp(resp_count);
    for (int i = 0; i < resp_count; i++)
    {
        resp[i].fd = eps[i].data.fd;
        resp[i].events = eps[i].events;
    }
    
    dmpCtx->resp_events_ = std::move(resp);
    if ((!notify_ && notify) || (notify_ && notify))
        center_->notifySubscriber(id_, ::pubsub::DMPWAIT, dmpCtx);
}

} // namespace reactor