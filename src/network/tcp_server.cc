#include "context.hpp"
#include "stream_socket.hpp"
#include <memory>
#include <mutex>
#include <vector>

#ifdef Debug
#   include <iostream>
#endif

#include "proxy_ctx.hpp"
#include "pub_type.hpp"
#include "nw_ctx.hpp"
#include "tcp_server.hpp"

namespace reactor {

TcpServer::TcpServer(std::string ip, std::string port, int subloop)
    : sub_id_(::pubsub::ID::sub_id())
    , pub_id_(::pubsub::ID::pub_id())
    , server_(ip, port)
    , acceptor_(std::make_shared<Acceptor>(server_()))
    , conn_map_()
    , center_(::pubsub::PubSubCenter::instance())
    , loop_(acceptor_, subloop)
    , total_conn_(0)
{
    server_.listen();
    server_.reuseAddrPort();
    StreamSocket::setNonBlocking(server_());

    center_->subscribe(acceptor_->pubID(), ::pubsub::ACCEPTED, this);

    auto const & ptrVec = loop_.dmpForLoopGroup();
    for (auto const & ptr : ptrVec)
    {
        center_->subscribe(ptr->pubID(), ::pubsub::DMPREGISTER, this);
        center_->subscribe(ptr->pubID(), ::pubsub::DMPMODIFY, this);
        center_->subscribe(ptr->pubID(), ::pubsub::DMPDELETE, this);
        center_->subscribe(ptr->pubID(), ::pubsub::DMPWAIT, this);
    }
}

TcpServer::~TcpServer()
{
    center_->unSubscribe(acceptor_->pubID(), ::pubsub::ACCEPTED, this);

    auto const & loop_threads = loop_.dmpForLoopGroup();
    for (auto const & ptr : loop_threads)
    {
        center_->unSubscribe(ptr->pubID(), ::pubsub::DMPREGISTER, this);
        center_->unSubscribe(ptr->pubID(), ::pubsub::DMPMODIFY, this);
        center_->unSubscribe(ptr->pubID(), ::pubsub::DMPDELETE, this);
        center_->unSubscribe(ptr->pubID(), ::pubsub::DMPWAIT, this);
    }

    loop_.stop();
    server_.close();
}

void TcpServer::start()
{
    loop_.loop();
}

uint16_t TcpServer::pubID() const
{
    return pub_id_;
}

void TcpServer::notify(::pubsub::PubType type, std::shared_ptr<::pubsub::Context> ctx)
{
    center_->notifySubscriber(pub_id_, pubsub::BALANCE, ctx);
}

uint16_t TcpServer::subID() const
{
    return sub_id_;
}

void TcpServer::subscribe(uint16_t pubID, ::pubsub::PubType type)
{
    center_->subscribe(pubID, type, this);
}

void TcpServer::update(std::shared_ptr<::pubsub::Context> ctx)
{
    switch (ctx->event_type_) {
        case ::pubsub::ACCEPTED:
        {
            updateForAccept(ctx);
            break;
        }
        case ::pubsub::DMPREGISTER:
        {
            updateForRegister(ctx);
            break;
        }
        case ::pubsub::DMPMODIFY:
        {
            updateForModify(ctx);
            break;
        }
        case ::pubsub::DMPDELETE:
        {
            updateForDelete(ctx);
            break;
        }
        case ::pubsub::DMPWAIT:
        {
            updateForWait(ctx);
            break;
        }
        default:
            break;
    }
}

void TcpServer::updateForAccept(std::shared_ptr<::pubsub::Context> ctx)
{
}

void TcpServer::updateForRegister(std::shared_ptr<::pubsub::Context> ctx)
{
    auto dmp_ctx = std::dynamic_pointer_cast<context::DmpRegisterContext>(ctx);

    auto dmp_ptr = dmp_ctx->dmp_ptr_;
    TcpConnection::ChannelPtr channel = std::make_shared<Channel>(dmp_ctx->fd_, dmp_ptr);

    // the pointer to Demultiplex in DmpRegisterContext
    // is not necessary. It just used to construct Channel object
    // in the layer.
    dmp_ctx->dmp_ptr_.reset();
    dmp_ptr.reset();

    std::unique_ptr<TcpConnection> conn(new TcpConnection(std::move(channel), ctx));
    std::lock_guard<std::mutex> lk(mx_);
    auto emplaced = conn_map_.emplace(dmp_ctx->fd_, std::move(conn));
    center_->account(StreamSocket::getPeerAddr(dmp_ctx->fd_).toString(), 0);
#ifdef Debug
    auto fd = dmp_ctx->fd_;
    if (emplaced.second)
    {
        total_conn_.fetch_add(1, std::memory_order_release);
        std::cout << "Incoming client: " << total_conn_.load(std::memory_order_acquire)
                  << ", fd: " << fd
                  << ", " << StreamSocket::getPeerAddr(fd).toString() << "\n";
    }
#endif
}

void TcpServer::updateForModify(std::shared_ptr<::pubsub::Context> ctx)
{
    auto dmp_ctx = std::dynamic_pointer_cast<context::DmpModifyContext>(ctx);

    auto const & iter = conn_map_.find(dmp_ctx->fd_);
    if (iter == conn_map_.end())
        return;

    auto & tcp_conn = iter->second;
    auto conn_ctx = std::dynamic_pointer_cast<context::DmpRegisterContext>(tcp_conn->context());

    if (dmp_ctx->fd_ != conn_ctx->fd_)
        return;

    conn_ctx->events_ = dmp_ctx->events_;
}

void TcpServer::updateForDelete(std::shared_ptr<::pubsub::Context> ctx)
{
    auto dmp_ctx = std::dynamic_pointer_cast<context::DmpDeleteContext>(ctx);

    removeConnection(dmp_ctx->fd_);
}

void TcpServer::updateForWait(std::shared_ptr<::pubsub::Context> ctx)
{
    auto dmp_ctx = std::dynamic_pointer_cast<context::DmpWaitContext>(ctx);

    auto lb_ptr = new proxy::context::LoadBalanceCtx(pubsub::BALANCE);
    std::shared_ptr<pubsub::Context> lb_ctx(lb_ptr);
    std::vector<int> sockes;

    auto iter = conn_map_.begin();
    for (auto & resp : dmp_ctx->resp_events_)
    {
        {
            std::lock_guard<std::mutex> guard(mx_);
            iter = conn_map_.find(resp.fd);
            if (iter == conn_map_.end())
                continue;
        }

        auto & tcp_conn = iter->second;
        if ((resp.events & Demultiplex::READABLE)
                && !(resp.events & Demultiplex::CLOSABLE)
            )
        {
            // TODO: forward incoming request to proxy
            sockes.emplace_back(resp.fd);

            // TODO: comment the code. it just used to test the recv and send ability.
            // tcp_conn->read();
            // auto str = tcp_conn->recvBuffer().retrieveAllAsString();
            // tcp_conn->send(str.c_str(), static_cast<size_t>(str.size()));
        }
        else if (resp.events & Demultiplex::WRITABLE)
        {
            tcp_conn->sendInLoop(*tcp_conn.get());
        }
        else if (resp.events & Demultiplex::CLOSABLE)
        {
            // FIXME: This call will remove monitored socket from epoll
            // and then notify TcpServer to remove TcpConnection object
            // mapped to the socket. But this will generating unnecessary
            // function call overhead.
            tcp_conn->disconnect();
            removeConnection(resp.fd);
        }
    }

    lb_ptr->sockes_ = std::move(sockes);
    notify(pubsub::BALANCE, std::move(lb_ctx));
}

void TcpServer::removeConnection(int sockfd)
{
    auto const & iter = conn_map_.find(sockfd);
    if (iter == conn_map_.end())
        return;

    if (sockfd != iter->first)
        return;

    total_conn_.fetch_sub(1, std::memory_order_release);
#ifdef Debug
    std::cout << "remove fd: " << sockfd << ", "
              << "Alive: " << total_conn_.load(std::memory_order_acquire)
              << ", " << StreamSocket::getPeerAddr(sockfd).toString() << "\n";
#endif
    std::lock_guard<std::mutex> guard(mx_);
    conn_map_.erase(sockfd);
}

} // namespace reactor