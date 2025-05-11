#include "RedisServer.h"

#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <string.h>

const int BUFFER_SIZE = 1024;
const size_t MAX_MSG_SIZE = 4096;

RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true), next_id(0) {}

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

        // Adds each client data in data structure, safeguarded by mutex
        int curr_id;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);    // protect write
            curr_id = next_id++;
            clients[curr_id] = {client_ip, client_port};
        }

        std::cout << "[INFO] New client connected: " << client_ip << ":" << client_port << ", ID " << curr_id << std::endl;

        std::thread t([this, client_socket, curr_id]() {
            this->handle_client(client_socket, curr_id);
        });
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

void RedisServer::handle_client(int client_socket, int client_id) {
    char buffer[BUFFER_SIZE];

    while (true) {
        std::string msg = recv_message_lenprefixed(client_socket);
        if (msg.starts_with("[DISCONNECT]")) {
            std::cout << "[INFO] Client " << client_id << " disconnected gracefully" << std::endl;
            break;
        }
        if (msg.starts_with("[ERROR]")) {
            std::cerr << msg << std::endl;
            break;
        }

        std::cout << "[CLIENT] Client " << client_id << ": " << msg << std::endl;

        if (!send_message_lenprefixed(client_socket, msg)) {
            std::cerr << "[ERROR] Sending response failed" << std::endl;
            break;
        }
    }

    close(client_socket);
}

bool RedisServer::send_message_lenprefixed(int fd, const std::string &msg) {
    if (msg.size() > MAX_MSG_SIZE) return false;

    uint32_t len = msg.size();
    char buf[4 + MAX_MSG_SIZE];
    memcpy(buf, &len, 4);  // assumes little-endian
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

std::string RedisServer::recv_message_lenprefixed(int fd) {
    char header[4];
    size_t received = 0;
    while (received < 4) {
        ssize_t n = recv(fd, header + received, 4 - received, 0);
        if (n == 0) return "[DISCONNECT] Client closed connection";
        if (n < 0) return "[ERROR] Failed to read length prefix";
        received += n;
    }

    uint32_t len;
    memcpy(&len, header, 4);
    if (len > MAX_MSG_SIZE) return "[ERROR] Message too long";

    char buf[MAX_MSG_SIZE + 1];
    received = 0;
    while (received < len) {
        ssize_t n = recv(fd, buf + received, len - received, 0);
        if (n == 0) return "[DISCONNECT] Client closed connection";
        if (n < 0) return "[ERROR] Failed to read body";
        received += n;
    }

    buf[len] = '\0';
    return std::string(buf);
}
