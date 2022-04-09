#include "simple.h"

#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

#include <cstdlib>

#include <iostream>

using namespace std;

void ErrSys(const std::string& func_name) {
    cout << "call " << func_name << " failed" << endl;
    exit(-1);
}

void ErrQuit(const std::string& msg) {
    cout << msg << endl;
    exit(-1);
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
            if(c=='\n') return read_line.size();
        }
        if(n<0) {
            if(errno==EINTR) continue;
            return -1;
        } else if(n==0) {
            // EOF
            return read_line.size();
        }
    }
    return read_line.size();
}

int AddEpollFd(int epoll_fd, int fd, uint32_t events) {
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

int DelEpollFd(int epoll_fd, int fd, uint32_t events) {
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
}