#include "RedisClient.h"
#include <iostream>

const int DEFAULT_PORT = 6379;

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: ./redis-cli <port>" << std::endl;
        return 1;
    }

    int port = DEFAULT_PORT;
    if (argc == 2) port = std::stoi(argv[1]);

    RedisClient client(port);
    client.run_client();

    return 0;
}
