#include "RedisServer.h"

#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

RedisServer::RedisServer(int port) : port_(port), server_socket_(-1) {}

void RedisServer::run() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket_ < 0) {
        perror("Socket creation failed");
        return;
    }
    
    // Create the socket itself
    struct sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Linux doesn't allow reusing the same address too quickly after shutting down
    // Forces server to be able to reuse the address again
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Binding
    if (bind(server_socket_, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Socket binding failed");
        close(server_socket_);
        return;
    }

    // Listening
    if (listen(server_socket_, 10) < 0) {
        perror("Socket listening failed");
    }

    std::cout << "Redis Server listening on port " << port_ << std::endl;
    return;
}

void RedisServer::shutdown() {
    if (server_socket_ != -1) {
        close(server_socket_);
    }
}
