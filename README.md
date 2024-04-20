# RProxy

## Feature

1.reactor模型：采用epoll监听IO事件，主reactor线程负责与对端建立，封装读写socket的对象，并将建立连接后的socket注册到子reactor线程，子reactor线程负责监听对端IO事件并交给负载均衡模块

2.负载均衡：RoundRobin算法将对端请求分发给健康状态的被代理服务器，使各个服务器平均分担压力

3.健康检测：采用定时任务形式，周期性向后端服务器发送带指定文本的检测报文，三次尝试后任无响应将其从可用列表中移除

4.“零拷贝”：在内核空间中开启一个管道，splice()系统调用将客户端 socket  缓冲区的地址指针及偏移量发送到管道中，再调用一次 splice()将数据从管道中转发到与被代理服务器建立连接的socket套接字，在内核空间内完成数据移动，全程中cpu不参与操作，节省cpu，提高程序效率。将被代理服务器对客户端的响应转发到客户端亦同理。

5.benchmark：模拟http请求，发送带http头部信息数据包，平均每秒10万次响应，iperf工具测试结果平均9Mbit/s

## Reverse Proxy

A reverse proxy is a type of proxy server that retrieves resources on behalf of a client from one or more servers. Unlike a traditional forward proxy, which sits between the client and the origin server, a reverse proxy sits in front of one or more servers and acts as a gateway for incoming client requests.

Key features of a reverse proxy include:

1. Load Balancing: A reverse proxy can distribute client requests across multiple backend servers, helping to evenly distribute the workload and improve overall performance and reliability.

2. SSL Termination: It can handle SSL/TLS encryption and decryption, offloading this process from the backend servers to improve performance and simplify server configuration.

3. Caching: A reverse proxy can cache static content and serve it directly to clients, reducing the load on backend servers and improving response times for frequently accessed resources.

4. Security: It can provide an additional layer of security by hiding the backend servers from direct access and filtering incoming requests to prevent malicious traffic.

To resolve the data transmission between the backend server and the client, the reverse proxy typically acts as an intermediary. When a client sends a request to the reverse proxy, the reverse proxy establishes a separate connection to the backend server and forwards the request. Once the backend server processes the request and generates a response, the reverse proxy receives the response and forwards it back to the original client.

This process involves managing and maintaining connections between the reverse proxy, backend servers, and clients, as well as handling data transmission in both directions. The reverse proxy may also need to handle issues such as request routing, content compression, and protocol translation to ensure seamless communication between the client and the backend server.
