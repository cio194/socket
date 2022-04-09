#include "simple.h"

#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>

#include <iostream>

using namespace std;

static void HandleEcho(int client_fd) {
    ssize_t n;
    string read_line;
    for(;;) {
        while((n=ReadLine(client_fd, read_line))>0) {
            // 这里没有较好考虑客户端关闭连接的情况
            if(WriteN(client_fd, read_line.c_str(), n)!=n) ErrSys("write");
        }
        if(n<0) ErrSys("read");
        if(n==0) return;
    }
}

static void HandleSigchld(int sig) {
    pid_t pid;
    int state;
    while((pid=waitpid(-1, &state, WNOHANG))>0) {
        cout << "child " << pid << " terminated" << endl;
    }
    return;
}

/*
服务器端需要考虑蛮多情况：
1、系统调用被信号打断，需要根据情况重启系统调用（主要针对慢系统调用，可能永久阻塞，例如accept、网络read
2、SIGCHLD信号的处理：使用多进程时，需要考虑到僵死进程，且必须小心信号本身并不排队（waitpid + while代替wait
3、各种边界条件，客户端崩溃、服务端崩溃等等。

针对服务端崩溃，可以理解到TCP的运作机制。TCP的写实际上是写入缓冲区，实际网络传输是由内核（即协议栈+网络驱动）在管理，
当服务端宕机或者突然不可达时，客户端写入后会一直等待，此时客户TCP持续重传分节，直到一定时间后（例如9分钟），才会收到
ETIMEDOUT、EHOSTUNREACH、ENETUNREACH等错误码（由read收到？）
*/
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd<0) {
        ErrSys("socket");
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SIMPLE_SERVER_PORT);
    if(bind(server_fd, (sockaddr *)(&server_addr), sizeof(server_addr))<0) 
        ErrSys("bind");
    if(listen(server_fd, 5)<0) ErrSys("listen");
    
    // 处理SIGCHLD信号（僵死进程）
    pid_t child_pid;
    struct sigaction act, oact;
    act.sa_handler = HandleSigchld;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_flags |= SA_RESTART;
    if(sigaction(SIGCHLD, &act, &oact)<0) ErrSys("sigaction");

    int client_fd = 0;
    sockaddr_in client_addr;
    socklen_t client_addr_len;
    for(;;) {
        client_addr_len = sizeof(client_addr);
        client_fd=accept(server_fd, (sockaddr *)&client_addr, &client_addr_len);
        if(client_fd < 0) ErrSys("accept");
        child_pid = fork();
        if(child_pid==0) {
            // 子进程处理Echo服务
            close(server_fd);
            HandleEcho(client_fd);
            exit(0);
        } else if(child_pid<0) ErrSys("fork");
        // 父进程继续监听
        close(client_fd);
    }
}