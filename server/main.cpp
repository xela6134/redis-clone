#include <iostream>
#include "RedisServer.h"

const int DEFAULT_PORT = 6379;

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: ./server <port-number>" << std::endl;
        return 1;
    }

    int port = DEFAULT_PORT;
    if (argc == 2) port = std::stoi(argv[1]);

    RedisServer server = RedisServer(port);
    server.run();

    
}
