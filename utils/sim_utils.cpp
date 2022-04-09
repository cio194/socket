#include "sim_utils.h"

#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

#include <cstdlib>

#include <iostream>

using namespace std;

void ErrSys(const string& func_name) {
    cout << "call " << func_name << " failed" << endl;
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
    if (fd < 0) ErrSys("socket");
    return fd;
}

void Connect(int cli_fd, const string& serv_addr_str,
    int serv_port) {
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, serv_addr_str.c_str(), &serv_addr.sin_addr) != 1)
        ErrSys("inet_pton");
    serv_addr.sin_port = htons(serv_port);
    if (connect(cli_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        ErrSys("connect");
}

void Listen(int serv_fd, int backlog,
    const std::string& serv_addr_str, int serv_port) {
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, serv_addr_str.c_str(), &serv_addr.sin_addr) != 1)
        ErrSys("inet_pton");
    serv_addr.sin_port = htons(serv_port);
    if (bind(serv_fd, (sockaddr*)(&serv_addr), sizeof(serv_addr)) < 0)
        ErrSys("bind");
    if (listen(serv_fd, backlog) < 0) ErrSys("listen");
}

int Accept(int sockfd, sockaddr_in* addr, socklen_t* len) {
    // 静态局部变量是否有线程安全问题呢？
    int connfd;
    while (true) {
        *len = sizeof(*addr);
        connfd = accept(sockfd, (sockaddr*)addr, len);
        if(connfd<0) {
            if(errno==EINTR) continue;
            ErrSys("accept");
        }
        break;
    }
    return connfd;
}

ssize_t Read(int sockfd, char* buf, size_t nbytes) {
    ssize_t nread;
    while(true) {
        nread = read(sockfd, buf, nbytes);
        if(nread<0) {
            if(errno==EINTR) continue;
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

ssize_t ReadLine(int fd, string& read_line) {
    read_line.clear();
    char c;
    int n;
    while (true) {
        while ((n = read(fd, &c, 1)) == 1) {
            read_line.push_back(c);
            if (c == '\n') return read_line.size();
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        } else if (n == 0) {
            // EOF
            return read_line.size();
        }
    }
    return read_line.size();
}