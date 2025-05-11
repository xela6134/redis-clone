#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include <string>

class RedisClient {
public:
    RedisClient(int port);

    void run_client();
    void disconnect();
    bool send_message(const std::string& msg);
    std::string receive_message();

    // Helper functions
    bool send_message_lenprefixed(int fd, const std::string& msg);
    std::string recv_message_lenprefixed(int fd);
private:
    // Network stuff
    int port;
    int client_socket;
};

#endif