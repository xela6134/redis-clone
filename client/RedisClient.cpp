#include "RedisClient.h"

#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <string>

const int BUFFER_SIZE = 1024;

RedisClient::RedisClient(int port) : port(port), client_socket(-1) {}

void RedisClient::run_client() {
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket < 0) {
        perror("[ERROR] Socket creation failed");
        return;
    }

    // Connects to server @ 127.0.0.1
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);    // INADDR_LOOPBACK is 127.0.0.1

    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("[ERROR] Failed to connect with server");
        close(client_socket);
        return;
    }

    std::cout << "[INFO] Connected to Redis server on port " << port << "." << std::endl;
    std::cout << "Type exit, or press Ctrl+C to exit gracefully" << std::endl;

    std::string input;
    char buffer[BUFFER_SIZE];

    while (true) {
        std::cout << "redis> ";
        std::getline(std::cin, input);

        // Edge cases
        if (input.empty()) continue;
        if (input == "exit") break;

        if (not send_message(input)) {
            std::cerr << "[ERROR] Failed to send command" << std::endl;
            break;
        }

        std::string response = receive_message();
        std::cout << "Received: " << response << std::endl;
    }

    disconnect();
}

bool RedisClient::send_message(const std::string& msg) {
    ssize_t sent = send(client_socket, msg.c_str(), msg.length(), 0);

    // sent: bytes actually sent
    // msg.length(): bytes you wanted to send
    // Defensive programming
    return sent == (ssize_t) msg.length();
}

std::string RedisClient::receive_message() {
    char buffer[BUFFER_SIZE];
    ssize_t bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        return "[ERROR] No response from server";
    }

    buffer[bytes] = '\0';
    return std::string(buffer);
}

void RedisClient::disconnect() {
    if (client_socket != -1) {
        close(client_socket);
        client_socket = -1;
        std::cout << "[INFO] Disconnected from server" << std::endl;
    }
}