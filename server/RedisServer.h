#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <atomic>
#include <unordered_map>

class RedisServer {
public:
    RedisServer(int port);

    void run_server();
    void shutdown();
    void handle_client(int client_socket, int client_id);

    // Helper functions
    bool send_message_lenprefixed(int fd, const std::string& msg);
    std::string recv_message_lenprefixed(int fd);
private:
    // Network stuff
    int port;
    int server_socket;
    std::atomic<bool> running;

    // For users
    // Data structure is { id, { client_ip, client_port } }
    std::unordered_map<int, std::pair<std::string, int>> clients;
    int next_id;
    std::mutex clients_mutex;
};

#endif
