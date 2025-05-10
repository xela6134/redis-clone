#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <atomic>

class RedisServer {
public:
    RedisServer(int port);

    void run_server();
    void shutdown();
private:
    int port;
    int server_socket;
    std::atomic<bool> running;
};

#endif
