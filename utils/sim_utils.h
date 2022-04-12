#ifndef SIM_UTILS_H_
#define SIM_UTILS_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstring>

#include <string>

const std::string SIM_SERV_ADDR = "127.0.0.1";

const int SIM_SERV_PORT = 9090;

const size_t MAX_EVENT_NUM = 1024;

void ErrSys(const std::string& func);

void ErrQuit(const std::string& msg);

void ErrMsg(const std::string& msg);

int TCPSocket();

// 根据unp v1，p108，connect不应该由程序员自动重启
// 要么使用select，要么让设置信号为自动重启系统调用
int TCPConnect(int clifd, const std::string& addr_str, int port);

int TCPListen(int sockfd, int backlog, const std::string& addr_str, int port);

int Accept(int sockfd, sockaddr_in* addr, socklen_t* len);

ssize_t Read(int sockfd, char* buf, size_t nbytes);

ssize_t WriteN(int fd, const char* buf, size_t n);

#endif