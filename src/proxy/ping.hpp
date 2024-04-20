#pragma once

#include <arpa/inet.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>

#include <stdexcept>
#include <string>

namespace proxy {

class Ping
{
public:
    Ping() {}
    ~Ping() {}

    static bool ping(std::string host)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(host.c_str());
        addr.sin_port = htons(5050);
        // auto ret = inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
        // if (ret < 0)
        //     throw std::runtime_error("inet_pton: can't transfer ip");

        int sockfd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        ::perror("raw_socket: ");
        if (sockfd == -1)
            throw std::runtime_error("can't create new raw socket");

        struct icmphdr hdr;
        ::memset(&hdr, 0, sizeof(hdr));

        hdr.type = ICMP_ECHO;
        hdr.code = 0;
        hdr.checksum = 0;
        hdr.un.echo.id = 0;
        hdr.un.echo.sequence = 0;

        char buf[1024];
        hdr.checksum = checksum((unsigned short * )&hdr, sizeof(hdr));
        auto ret = ::sendto(sockfd, (char const *) &hdr, sizeof(hdr), 0, (const struct sockaddr *) &addr, sizeof(addr));
        ::perror("sendto(): ");
        if (ret < 1)
            throw std::runtime_error("sendto() error");

        ::memset(buf, 0, sizeof(buf));

        ret = ::recv(sockfd, buf, sizeof(buf), 0);
        if (ret < 1)
            throw std::runtime_error("recv() error");

        struct icmphdr * icmphdr_ptr;
        struct iphdr * iphdr_ptr;

        // iphdr_ptr = (struct iphdr *) buf;
        // icmphdr_ptr = (struct icmphdr *) buf + (iphdr_ptr->ihl) * 4;
        iphdr_ptr = reinterpret_cast<struct iphdr *>(buf);
        icmphdr_ptr = reinterpret_cast<struct icmphdr *>(buf)
                            + (iphdr_ptr->ihl) * 4;

        std::cout << (icmphdr_ptr->type == 0 || icmphdr_ptr->type == 0) << "\n";
        return icmphdr_ptr->type == 0 || icmphdr_ptr->type == 0;
    }

private:
    static unsigned short checksum(unsigned short * buf, int bufsz)
    {
        unsigned long sum = 0xffff;

        while (bufsz > 1)
        {
            sum += *buf;
            buf++;
            bufsz -=2;
        }

        if (bufsz == 1)
            sum += *(unsigned char *) buf;

        sum = (sum & 0xffff) + (sum >> 16);
        sum = (sum & 0xffff) + (sum >> 16);

        return ~sum;

    }
};

} // namespace