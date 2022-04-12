#include "sim_server.h"
#include "sim_utils.h"
#include "sim_http.h"

using namespace std;

int SimServer::servfd = 0;

pthread_mutex_t SimServer::mlock = PTHREAD_MUTEX_INITIALIZER;

void* SimServer::thread_main(void* arg) {
    auto tid = pthread_self();
    pthread_detach(tid);

    int connfd;
    sockaddr_in cliaddr;
    socklen_t clilen;
    HTTP_CODE hcode;
    HttpRequest request;
    while (true) {
        pthread_mutex_lock(&mlock);
        connfd = Accept(servfd, &cliaddr, &clilen);
        pthread_mutex_unlock(&mlock);
        if (connfd < 0) {
            ErrMsg("thread " + to_string(tid) + " exits");
            int ret;
            pthread_exit(&ret);
        }
        hcode = request.ParseRequest(connfd);
        if (hcode == HTTP_CODE::REQUEST_OK) {
            cout << "=====================" << endl;
            cout << "thread: " << to_string(tid) << endl;
            cout << "method: " << request.method << endl;
            cout << "uri: " << request.uri << endl;
            cout << "version: " << request.version << endl;
            cout << "headers: " << request.headers.size() << endl;
            for (auto it = request.headers.begin(); it != request.headers.end(); it++) {
                cout << it->first << ": " << it->second << endl;
            }
            cout << "=====================" << endl << endl;
        } else {
            ErrMsg("thread " + to_string(tid) + " 解析错误");
        }
        close(connfd);
    }
}

void SimServer::start(const std::string& addr_str, int port, int nthread) {
    if (nthread < 1 || nthread>50) {
        ErrQuit("线程数量错误（1-50）");
    }

    servfd = TCPSocket();
    if (servfd < 0) {
        ErrQuit("服务器启动失败");
    }
    int opt = 1;
    setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (TCPListen(servfd, 10, addr_str, port) < 0) {
        ErrQuit("服务器启动失败");
    }

    // 创建线程池，各自accept
    pthread_t tid;
    for (int i = 0; i < nthread; i++) {
        if (pthread_create(&tid, NULL, thread_main, NULL) != 0) {
            ErrQuit("线程创建失败");
        }
    }
    while (true) {
        pause();
    }
}