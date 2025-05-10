#include "RedisServer.h"

#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

RedisServer::RedisServer(int port) : port_(port), server_socket_(-1), running_(true) {}

void RedisServer::run() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket_ < 0) {
        perror("[ERROR] Socket creation failed");
        return;
    }
    
    // Create the socket itself
    struct sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_);         // Sending is htons, receiving is ntohs
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Linux doesn't allow reusing the same address too quickly after shutting down
    // Without this option, the server won’t able to bind to the same address if restarted.
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Binding
    if (bind(server_socket_, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("[ERROR] Socket binding failed");
        close(server_socket_);
        return;
    }

    // Listening
    if (listen(server_socket_, 10) < 0) {
        perror("[ERROR] Socket listening failed");
    }

    std::cout << "[INFO] Redis Server listening on port " << port_ << std::endl;

    struct sockaddr_in client_address;
    socklen_t address_length = sizeof(client_address);
    int client_socket;

    while (running_) {
        client_socket = accept(server_socket_, (struct sockaddr*)&client_address, &address_length);

        if (client_socket < 0) {
            perror("[ERROR] Accepting client failed");
            return;
        }

        // Display client info
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_address.sin_port);

        std::cout << "[INFO] New client connected: " << client_port << ":" << client_ip << std::endl;
    }

    return;
}

void RedisServer::shutdown() {
    running_ = false;
    if (server_socket_ != -1) {
        close(server_socket_);
    }
    std::cout << "[INFO] Server shutdown complete" << std::endl;
}
