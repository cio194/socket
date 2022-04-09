#ifndef SIMPLE_H_
#define SIMPLE_H_

#include <string>

const std::string SIMPLE_SERVER_ADDR = "127.0.0.1";

const int SIMPLE_SERVER_PORT = 9090;

const size_t MAX_EVENT_NUM = 1024;

void ErrSys(const std::string& func_name);

void ErrQuit(const std::string& msg);

ssize_t WriteN(int fd, const char* buf, size_t n);

ssize_t ReadLine(int fd, std::string& read_line);

int AddEpollFd(int epoll_fd, int fd, uint32_t events);

int DelEpollFd(int epoll_fd, int fd, uint32_t events);

#endif