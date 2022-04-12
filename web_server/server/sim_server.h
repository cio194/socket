#ifndef SIM_SERVER_H_
#define SIM_SERVER_H_

#include "sim_utils.h"

#include <pthread.h>
#include <unistd.h>

#include <string>

/*
要考虑哪些边界？
SIGCHLD信号处理
客户端、服务端崩溃

连接管理怎么做？
持久连接：浏览器关闭还是服务器关闭？
非持久连接直接不支持
*/
class SimServer {
private:
    static int servfd;
    static pthread_mutex_t mlock;
    static void* thread_main(void* arg);
private:
    SimServer(const SimServer&) = delete;
    SimServer& operator=(const SimServer&) = delete;
    SimServer() {}
public:
    static SimServer& GetInstance() {
        static SimServer sv;
        return sv;
    }
    void start(const std::string& addr_str, int port, int nthread);
};

#endif