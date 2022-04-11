#include "sim_utils.h"

#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

#include <cstdlib>

#include <iostream>

using namespace std;

// 标准socket的包装函数用来干什么？
// 针对EINTER，重启系统调用；
// 哪些需要杀死程序，哪些不需要呢？统一都不杀死吧

void ErrSys(const string& func) {
    cout << "call " << func << " failed" << endl;
    exit(-1);
}

void ErrQuit(const string& msg) {
    cout << msg << endl;
    exit(-1);
}

void ErrMsg(const std::string& msg) {
    cout << msg << endl;
}

int Socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) ErrMsg("socket");
    return fd;
}

int Connect(int clifd, const string& addr_str, int port) {
    int ret = 1;
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    ret = inet_pton(AF_INET, addr_str.c_str(), &serv_addr.sin_addr);
    if (ret != 1) {
        ErrMsg("inet_pton");
        return ret;
    }
    serv_addr.sin_port = htons(port);
    ret = connect(clifd, (sockaddr*) &serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        ErrMsg("connect");
        return ret;
    }
    return ret;
}

int Listen(int serv_fd, int backlog,
           const std::string& addr, int port) {
    int ret = 1;
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, addr.c_str(), &serv_addr.sin_addr) != 1)
        ErrSys("inet_pton");
    serv_addr.sin_port = htons(port);
    if (bind(serv_fd, (sockaddr*) (&serv_addr), sizeof(serv_addr)) < 0)
        ErrSys("bind");
    if (listen(serv_fd, backlog) < 0) ErrSys("listen");
}

int Accept(int sockfd, sockaddr_in* addr, socklen_t* len) {
    // 静态局部变量是否有线程安全问题呢？
    int connfd;
    while (true) {
        *len = sizeof(*addr);
        connfd = accept(sockfd, (sockaddr*) addr, len);
        if (connfd < 0) {
            if (errno == EINTR) continue;
            ErrSys("accept");
        }
        break;
    }
    return connfd;
}

ssize_t Read(int sockfd, char* buf, size_t nbytes) {
    ssize_t nread;
    while (true) {
        nread = read(sockfd, buf, nbytes);
        if (nread < 0) {
            if (errno == EINTR) continue;
            ErrSys("read");
        }
        break;
    }
    return nread;
}

ssize_t WriteN(int fd, const char* buf, size_t n) {
    size_t left_n;
    ssize_t write_n;
    left_n = n;
    while (left_n > 0) {
        write_n = write(fd, buf, left_n);
        if (write_n <= 0 && errno == EINTR) continue;
        else if (write_n <= 0) return -1;
        left_n -= write_n;
        buf += write_n;
    }
    return n;
}