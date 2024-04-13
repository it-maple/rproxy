#pragma once

#include <cstdint>
#include <string>

namespace reactor {

class InetAddr
{
public:
    InetAddr(std::string ip, uint16_t port);
    InetAddr(const InetAddr &) = default;
    InetAddr(InetAddr &&) = default;
    InetAddr & operator=(const InetAddr &) = default;
    InetAddr & operator=(InetAddr &&) = default;
    ~InetAddr() = default;

    // [ip:port]
    std::string toString();

    std::string operator()();

    std::string ip();

    std::string port();

private:
    std::string ip_;
    uint16_t    port_;
};

class StreamSocket
{
    public:
        explicit StreamSocket(int sockfd);
        StreamSocket(std::string & ip, std::string & port);
        StreamSocket(const StreamSocket&) = default;
        StreamSocket(StreamSocket&&) = default;
        StreamSocket& operator=(const StreamSocket&) = default;
        StreamSocket& operator=(StreamSocket&&) = default;
        ~StreamSocket();

        const int & operator()() noexcept;

        int bindAddr();

        int bindAddr(std::string const & ip, std::string const & port);

        static int bindAddr(int sockfd, std::string const & ip, std::string const & port);

        int listen();

        static int listen(int fd, int backlog = 50);

        int accept();

        void reuseAddrPort();

        void close();

        static void close(int fd);

        static int connectTo(InetAddr addr);

        static ssize_t read(int sockfd, void * const buf, size_t count);

        static ssize_t write(int sockfd, void const * buf, size_t count);

        static void setNonBlocking(int fd);

        static InetAddr getPeerAddr(int fd);

    private:
        int         sockfd_;
        std::string ip_;
        std::string port_;
};

} // namespace reactor