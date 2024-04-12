#pragma once

#include "demultiplex.hpp"

namespace reactor {

class Channel
{
public:
    // Channel(int fd, Demultiplex * const dmp);
    Channel(int fd, std::shared_ptr<Demultiplex> dmp) noexcept;
    Channel(const Channel &) = default;
    Channel(Channel &&) = default;
    Channel & operator=(const Channel &) = default;
    Channel & operator=(Channel &&) = default;
    ~Channel();

    int socket() noexcept;

    std::shared_ptr<Demultiplex> dmp() noexcept;

    void leave();

    bool isLeave() noexcept;

    void enableRead(std::shared_ptr<::pubsub::Context> ctx);

    void enableWrite(std::shared_ptr<::pubsub::Context> ctx);

    void disableRead(std::shared_ptr<::pubsub::Context> ctx);

    void disableWrite(std::shared_ptr<::pubsub::Context> ctx);

private:
    bool                        leave_;
    int                         socket_;
    // Demultiplex *               dmp_;
    std::shared_ptr<Demultiplex>    dmp_ptr_;
};

} // namespace reactor