#include "RedisServer.h"

#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include <unistd.h>

const int BUFFER_SIZE = 1024;

RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true), total_clients(0) {}

void RedisServer::run_server() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
        perror("[ERROR] Socket creation failed");
        return;
    }
    
    // Create the socket itself
    // Server listens to specified port and address 0.0.0.0 
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);              // Sending is htons (host-to-network short), receiving is ntohs (for ports)
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Sending is htonl (host-to-network long),  receiving is ntohl (for address)

    // Linux doesn't allow reusing the same address too quickly after shutting down
    // Without this option, the server won’t able to bind to the same address if restarted.
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Binding
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("[ERROR] Socket binding failed");
        close(server_socket);
        return;
    }

    // Listening
    if (listen(server_socket, 10) < 0) {
        perror("[ERROR] Socket listening failed");
    }

    std::cout << "[INFO] Redis Server listening on port " << port << std::endl;

    struct sockaddr_in client_address;
    socklen_t address_length = sizeof(client_address);
    int client_socket;

    while (running) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &address_length);

        if (client_socket < 0) {
            perror("[ERROR] Accepting client failed");
            return;
        }

        // Display client info
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_address.sin_port);

        std::cout << "[INFO] New client connected: " << client_ip << ":" << client_port << std::endl;

        std::thread t(RedisServer::handle_client, client_socket, client_ip, client_port);
        t.detach();
    }

    return;
}

void RedisServer::shutdown() {
    running = false;
    if (server_socket != -1) {
        close(server_socket);
    }
    std::cout << "[INFO] Server shutdown complete" << std::endl;
}

void RedisServer::handle_client(int client_socket, char client_ip[16], int client_port) {
    char buffer[BUFFER_SIZE];

    while (true) {
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received == 0) {
            std::cout << "[INFO] Client disconnected: " << client_ip << ":" << client_port << std::endl;
            break;
        } else if (bytes_received < 0) {
            perror("[ERROR] Receiving from client failed");
            break;
        }

        buffer[bytes_received] = '\0';
        std::cout << "[CLIENT] " << buffer << std::endl;

        ssize_t bytes_sent = send(client_socket, buffer, bytes_received, 0);
        if (bytes_sent < 0) {
            perror("[ERROR] Sending data back to client failed");
            break;
        }
    }

    close(client_socket);
}
