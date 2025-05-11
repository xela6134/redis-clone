#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <atomic>
#include <unordered_set>

class RedisServer {
public:
    RedisServer(int port);

    void run_server();
    void shutdown();
    static void handle_client(int client_socket, char client_ip[16], int client_port);
private:
    // Network stuff
    int port;
    int server_socket;
    std::atomic<bool> running;

    // For users
    std::unordered_set<std::string, int> clients;
    int total_clients;
};

#endif
