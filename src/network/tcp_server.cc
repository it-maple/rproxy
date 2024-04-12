#include <iostream>
#include <memory>

#include "eventloop.hpp"
#include "nw_ctx.hpp"
#include "tcp_server.hpp"
#include "stream_socket.hpp"

namespace reactor {

TcpServer::TcpServer(std::string ip, std::string port, int subloop)
    : id_(::pubsub::ID::sub_id())
    , server_(ip, port)
    , acceptor_(std::make_shared<Acceptor>(server_()))
    , conn_map_()
    , center_(::pubsub::PubSubCenter::instance())
    , loop_(acceptor_, subloop)
    // , loops_()
    // , workers_(subloop)
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

uint16_t TcpServer::subID() const
{
    return id_;
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
            // workers_.submit(std::bind(&TcpServer::updateForRegister, this, ctx));
            break;
        }
        case ::pubsub::DMPMODIFY:
        {
            updateForModify(ctx);
            // workers_.submit(std::bind(&TcpServer::updateForModify, this, ctx));
            break;
        }
        case ::pubsub::DMPDELETE:
        {
            updateForDelete(ctx);
            // workers_.submit(std::bind(&TcpServer::updateForDelete, this, ctx));
            break;
        }
        case ::pubsub::DMPWAIT:
        {
            // workers_.submit(std::bind(&TcpServer::updateForWait, this, ctx));
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
    auto cnt = dmp_ptr.use_count();
    TcpConnection::ChannelPtr channel = std::make_shared<Channel>(dmp_ctx->fd_, dmp_ptr);

    std::lock_guard<std::mutex> lk(mx_);
    std::unique_ptr<TcpConnection> conn(new TcpConnection(std::move(channel), std::move(ctx)));
    conn_map_.emplace(dmp_ctx->fd_, std::move(conn));
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
    for (auto & resp : dmp_ctx->resp_events_)
    {
        auto & tcp_conn = conn_map_.at(resp.fd);
        if ((resp.events & Demultiplex::READABLE)
                && !(resp.events & Demultiplex::CLOSABLE)
            )
        {
            tcp_conn->read();
            // TODO: comment the code. it just used to test the recv and send ability.
            auto str = tcp_conn->recvBuffer().retrieveAllAsString();
            tcp_conn->send(str.c_str(), static_cast<size_t>(str.size()));
        }
        if (resp.events & Demultiplex::WRITABLE)
        {
            tcp_conn->sendInLoop(*tcp_conn.get());
        }
        if (resp.events & Demultiplex::CLOSABLE)
        {
            // removeConnection(resp.fd);
            tcp_conn->disconnect();
        }
    }
}

void TcpServer::removeConnection(int sockfd)
{
    auto const & iter = conn_map_.find(sockfd);
    if (iter == conn_map_.end())
        return;

    if (sockfd != iter->first)
        return;

    std::lock_guard<std::mutex> lk(mx_);
    conn_map_.erase(sockfd);
    std::cout << "remove fd: " << sockfd << "\n";
}

} // namespace reactor