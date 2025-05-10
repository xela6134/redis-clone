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

private:
    int port;
    int client_socket;
};

#endif