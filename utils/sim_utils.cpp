#include "sim_utils.h"

#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

#include <cstdlib>
#include <cstring>

#include <iostream>

using namespace std;

// 标准socket的包装函数用来干什么？
// 针对EINTER，重启系统调用；打印错误消息

void ErrSys(const string& func) {
    cout << "call " << func << " failed, ";
    cout << "errno: " << errno << ' ' << strerror(errno) << endl;
}

void ErrQuit(const string& msg) {
    cout << msg << endl;
    exit(-1);
}

void ErrMsg(const std::string& msg) {
    cout << msg << endl;
}

int TCPSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) ErrSys("socket");
    return fd;
}

int TCPConnect(int clifd, const string& addr_str, int port) {
    int ret = 1;
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    ret = inet_pton(AF_INET, addr_str.c_str(), &addr.sin_addr);
    if (ret != 1) {
        ErrSys("inet_pton");
        return ret;
    }
    addr.sin_port = htons(port);
    ret = connect(clifd, (sockaddr*) &addr, sizeof(addr));
    if (ret < 0) {
        ErrSys("connect");
        return ret;
    }
    return ret;
}

int TCPListen(int sockfd, int backlog, const std::string& addr_str, int port) {
    int ret = 1;
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if ((ret = inet_pton(AF_INET, addr_str.c_str(), &addr.sin_addr)) != 1) {
        ErrSys("inet_pton");
        return ret;
    }
    addr.sin_port = htons(port);
    if ((ret = bind(sockfd, (sockaddr*) (&addr), sizeof(addr))) < 0) {
        ErrSys("bind");
        return ret;
    }
    if ((ret = listen(sockfd, backlog)) < 0) {
        ErrSys("listen");
        return ret;
    }
    return ret;
}

int Accept(int sockfd, sockaddr_in* addr, socklen_t* len) {
    int connfd;
    while (true) {
        *len = sizeof(*addr);
        connfd = accept(sockfd, (sockaddr*) addr, len);
        if (connfd < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                ErrSys("accept");
                return connfd;
            }
        } else {
            break;
        }
    }
    return connfd;
}

ssize_t Read(int sockfd, char* buf, size_t nbytes) {
    ssize_t nread;
    while (true) {
        nread = read(sockfd, buf, nbytes);
        if (nread < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                ErrSys("read");
                return nread;
            }
        } else {
            break;
        }
    }
    return nread;
}

ssize_t WriteN(int fd, const char* buf, size_t n) {
    size_t nleft;
    ssize_t nwrite;
    nleft = n;
    while (nleft > 0) {
        nwrite = write(fd, buf, nleft);
        if (nwrite < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                ErrSys("write");
                return nwrite;
            }
        } else {
            nleft -= nwrite;
            buf += nwrite;
        }
    }
    return n;
}