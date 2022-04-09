#include "simple.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <cstring>

#include <iostream>
#include <string>

using namespace std;

/*
编程方面，自己不够统一，要么就都用C++，要么都用C，混用实在太拉了
比如，字符串的读写，缓冲区末尾是否需要\0？  自己是否需要额外添加\0？

服务端似乎并不需要字符串结尾，因为并不会对c_str使用printf等c风格IO操作

还有一个问题，writen封装函数里面处理了系统调用被中断的情况，
但另外一些函数，例如accept、connect又没有封装并处理此情况，这不够统一
能否直接让系统调用被打断后自动重启？

采用IO复用（epoll），是为了解决服务器终止时（例如服务器线程被kill），客户端能收到fin或者rst
如果不用IO复用，客户端的串行化导致客户端可能阻塞于标准输入，收不到fin或rst
*/
int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) ErrSys("socket");

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, SIMPLE_SERVER_ADDR.c_str(), &server_addr.sin_addr.s_addr) != 1)
        ErrSys("inet_pton");
    server_addr.sin_port = htons(SIMPLE_SERVER_PORT);
    if (connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        ErrSys("connect");

    epoll_event events[2];
    int epoll_fd = epoll_create(4), stdin_fd = fileno(stdin);
    if (epoll_fd < 0) ErrSys("epoll");
    if (AddEpollFd(epoll_fd, stdin_fd, EPOLLIN) < 0) ErrSys("epoll_ctl");
    if (AddEpollFd(epoll_fd, client_fd, EPOLLIN) < 0) ErrSys("epoll_ctl");

    bool stdin_eof = false;
    string line;
    while (true) {
        int ret = epoll_wait(epoll_fd, events, 2, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            ErrSys("epoll_wait");
        }
        for (int i = 0; i < ret; i++) {
            int efd = events[i].data.fd;
            if (efd == stdin_fd) {
                int n = ReadLine(stdin_fd, line);
                if (n < 0) ErrSys("read");
                if(n==0) {
                    stdin_eof = true;
                    shutdown(client_fd, SHUT_WR);
                    if(DelEpollFd(epoll_fd, stdin_fd, EPOLLIN)<0) ErrSys("epoll_ctl del");
                    continue;
                }
                n = line.size();
                if (WriteN(client_fd, line.c_str(), n) != n) ErrSys("write");
            } else if (efd == client_fd) {
                int n = ReadLine(client_fd, line);
                if (n < 0) ErrSys("read");
                if (n == 0) {
                    if(stdin_eof==1) return 0;
                    ErrQuit("server terminated prematurely");
                }
                cout << line;
            }
        }
    }
    exit(0);
}