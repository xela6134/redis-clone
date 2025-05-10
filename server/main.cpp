#include <iostream>
#include <csignal>
#include "RedisServer.h"

const int DEFAULT_PORT = 6379;

RedisServer* global_server = nullptr;

// Handles Ctrl+C
void handle_sigint(int signum) {
    std::cout << "\n[INFO] SIGINT received. Gracefully shutting down server..." << std::endl;
    if (global_server) {
        global_server->shutdown();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " <port-number>" << std::endl;
        return 1;
    }

    int port = DEFAULT_PORT;
    if (argc == 2) port = std::stoi(argv[1]);

    RedisServer server(port);
    global_server = &server;

    signal(SIGINT, handle_sigint);

    server.run();

    return 0;
}
