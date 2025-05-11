#include "RedisClient.h"

#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <string.h>

const int BUFFER_SIZE = 1024;
const size_t MAX_MSG_SIZE = 4096;

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
    std::cout << "[INFO] Type exit, or press Ctrl+C to exit gracefully" << std::endl;

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
    return send_message_lenprefixed(client_socket, msg);
}

std::string RedisClient::receive_message() {
    return recv_message_lenprefixed(client_socket);
}

void RedisClient::disconnect() {
    if (client_socket != -1) {
        std::cout << "[INFO] Gracefully shutting down CLI..." << std::endl;
        close(client_socket);
        client_socket = -1;
        std::cout << "[INFO] Disconnected from server" << std::endl;
    }
}

bool RedisClient::send_message_lenprefixed(int fd, const std::string &msg) {
    if (msg.size() > MAX_MSG_SIZE) return false;

    uint32_t len = msg.size();
    char buf[4 + MAX_MSG_SIZE];
    memcpy(buf, &len, 4);               // assumes little-endian
    memcpy(buf + 4, msg.data(), len);

    size_t to_send = 4 + len;
    size_t sent = 0;
    while (sent < to_send) {
        ssize_t n = send(fd, buf + sent, to_send - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

std::string RedisClient::recv_message_lenprefixed(int fd) {
    char header[4];
    size_t received = 0;
    while (received < 4) {
        ssize_t n = recv(fd, header + received, 4 - received, 0);
        if (n <= 0) return "[ERROR] Failed to read length prefix";
        received += n;
    }

    uint32_t len;
    memcpy(&len, header, 4);
    if (len > MAX_MSG_SIZE) return "[ERROR] Message too long";

    char buf[MAX_MSG_SIZE + 1];
    received = 0;
    while (received < len) {
        ssize_t n = recv(fd, buf + received, len - received, 0);
        if (n <= 0) return "[ERROR] Failed to read body";
        received += n;
    }

    buf[len] = '\0';
    return std::string(buf);
}
