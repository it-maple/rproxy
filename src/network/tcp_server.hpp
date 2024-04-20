#pragma once

#include <sys/types.h>

#include <atomic>

#include "eventloop.hpp"
#include "publisher.hpp"
#include "tcp_connection.hpp"
// #include "worker_group.hpp"

namespace reactor {

class TcpServer : public ::pubsub::Subscriber,
                  public pubsub::Publisher
{
public:
    using TcpConnectionPtr = std::unique_ptr<TcpConnection>;

public:
    TcpServer(std::string ip, std::string port, int subloop = 1);

    ~TcpServer();

    void start();

public:
    virtual uint16_t pubID() const override;

    virtual void notify(::pubsub::PubType type, std::shared_ptr<::pubsub::Context> ctx) override;

    virtual uint16_t subID() const override;

    virtual void subscribe(uint16_t pubID, ::pubsub::PubType type) override;

    virtual void update(std::shared_ptr<::pubsub::Context> ctx) override;

private:
    void updateForAccept(std::shared_ptr<::pubsub::Context> ctx);

    void updateForRegister(std::shared_ptr<::pubsub::Context> ctx);

    void updateForModify(std::shared_ptr<::pubsub::Context> ctx);

    void updateForDelete(std::shared_ptr<::pubsub::Context> ctx);

    void updateForWait(std::shared_ptr<::pubsub::Context> ctx);

    void removeConnection(int sockfd);

    friend void TcpConnection::sendInLoop(TcpConnection & conn);

private:
    uint16_t                                    sub_id_;
    uint16_t                                    pub_id_;
    StreamSocket                                server_;
    std::shared_ptr<Acceptor>                   acceptor_;
    std::unordered_map<int, TcpConnectionPtr>   conn_map_;
    std::shared_ptr<::pubsub::PubSubCenter>       center_;

    std::mutex                                  mx_;
    EventLoop                                   loop_;
    // std::vector<EventLoop>                      loops_;
    // WorkerGroup                                 workers_;

    std::atomic<ssize_t>                        total_conn_;
};

} // namespace reactor