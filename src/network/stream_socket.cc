#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
#include <string.h>

#include <string>

#include "stream_socket.hpp"

namespace reactor {

InetAddr::InetAddr(std::string ip, uint16_t port)
    : ip_(ip)
    , port_(port) {}

std::string InetAddr::toString()
{
    // [ip:port]
    std::string info;
    auto port = std::to_string(port_);

    info.append("[");
    info.assign(ip_.c_str());
    info.append(":");

    info.append(port.c_str());
    info.append("]");

    return info;
}

std::string InetAddr::operator()()
{
    return toString();
}

std::string InetAddr::ip() { return ip_; }

std::string InetAddr::port() { return std::to_string(port_); }


StreamSocket::StreamSocket(int sockfd)
    : sockfd_(sockfd)
    , ip_()
    , port_()
{
    assert(sockfd >= 0);
}

StreamSocket::StreamSocket(std::string & ip, std::string & port)
    : sockfd_(::socket(AF_INET, SOCK_STREAM, 0))
    , ip_(ip)
    , port_(port)
{
    assert(sockfd_ >= 0);

    bindAddr();
}

StreamSocket::~StreamSocket()
{
    ::close(sockfd_);
}

const int & StreamSocket::operator()() noexcept { return sockfd_; }

int StreamSocket::bindAddr()
{
    return bindAddr(ip_, port_);
}

int StreamSocket::bindAddr(std::string const & ip, std::string const & port)
{
    return bindAddr(sockfd_, ip, port);
}

int StreamSocket::bindAddr(int sockfd, std::string const & ip, std::string const & port)
{
    struct sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &addr.sin_addr);
    addr.sin_port = ::htons(::atoi(port.c_str()));

    auto ret = ::bind(sockfd, (struct sockaddr*) &addr, sizeof(addr));
    assert(ret != -1);

    return ret;
}

int StreamSocket::listen()
{
    auto ret = ::listen(sockfd_, 50);
    assert(ret != -1);

    return ret;
}

int StreamSocket::listen(int fd, int backlog)
{
    auto ret = ::listen(fd, 50);
    assert(ret != -1);

    return ret;
}

int StreamSocket::accept()
{
    socklen_t addrLen;
    struct sockaddr_in clientAddr;
    auto ret = ::accept(sockfd_, (struct sockaddr *) &clientAddr, &addrLen);
    
    // the socket is marked nonblocking and no conncetions are present to accepted.
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return ret;

    assert(ret != -1);

    return ret;
}

void StreamSocket::close()
{
    ::close(sockfd_);
}

void StreamSocket::close(int fd)
{
    ::close(fd);
}

void StreamSocket::reuseAddrPort()
{
    int reuse = 1;
    auto ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    assert(ret != -1);
    ret = setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    assert(ret != -1);
}

int StreamSocket::connectTo(InetAddr addr)
{
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = ::inet_addr(addr.ip().c_str());
    serverAddr.sin_port = ::htons(::atoi(addr.port().c_str()));

    auto sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        throw std::runtime_error("can't create new socket");

    auto ret = ::connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (ret < 0)
        throw std::runtime_error("can not establish connection");

    return sock;
}

ssize_t StreamSocket::read(int sockfd, void * const buf, size_t count)
{
    return ::read(sockfd, buf, count);
}

ssize_t StreamSocket::write(int sockfd, const void *buf, size_t count)
{
    return ::write(sockfd, buf, count);
}

void StreamSocket::setNonBlocking(int fd)
{
    auto flag = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, flag | O_NONBLOCK);
}

InetAddr StreamSocket::getPeerAddr(int fd)
{
    struct sockaddr_in client_addr; 
    socklen_t len = sizeof(client_addr);
    ::getpeername(fd, (struct sockaddr *) (&client_addr), &len);

    return InetAddr(
            ::inet_ntoa(client_addr.sin_addr), 
            ::ntohs(client_addr.sin_port));
}

} // namespace reactor
