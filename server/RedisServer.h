#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <atomic>
#include <string>
#include <unordered_map>

class RedisServer {
public:
    RedisServer(int port);

    void run_server();
    void shutdown();

    // Length-prefixed message helpers
    bool send_message_lenprefixed(int fd, const std::string& msg);
    std::string recv_message_lenprefixed(int fd);
    std::string handle_command(const std::string& msg);
private:
    // Managing server execution
    int port;
    int server_socket;
    std::atomic<bool> running;

    // Managing users
    int next_client_id;
    std::unordered_map<int, int> fd_to_client_id;

    // Key-Value Store {string: string}
    std::unordered_map<std::string, std::string> kv_store;
};

#endif
