#include "RedisClient.h"
#include <iostream>
#include <csignal>

const int DEFAULT_PORT = 6379;
RedisClient* global_client = nullptr;

void handle_sigint(int) {
    std::cout << "\n[INFO] SIGINT received" << std::endl;
    if (global_client) {
        global_client->disconnect();
    }
    std::_Exit(0);
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = DEFAULT_PORT;
    if (argc == 2) port = std::stoi(argv[1]);

    RedisClient client(port);
    global_client = &client;

    signal(SIGINT, handle_sigint);

    client.run_client();
    return 0;
}
