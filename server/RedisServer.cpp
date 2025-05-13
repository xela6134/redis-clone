#include "RedisServer.h"

#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <unordered_map>
#include <vector>
#include <fcntl.h>

const int BUFFER_SIZE = 1024;
const size_t MAX_MSG_SIZE = 4096;

RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true), next_client_id(0) {}

void RedisServer::run_server() {
    // Server socket creation
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("[ERROR] Socket creation failed");
        return;
    }

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
        return;
    }

    // Set server socket to 'non-blocking'
    // Blocking: Function waits until data arrives (e.g. read())
    // Non-blocking: Just do the next thing, doesn't matter if data was received or not
    fcntl(server_socket, F_SETFL, O_NONBLOCK);

    std::cout << "[INFO] Redis Server listening on port " << port << std::endl;

    // Store all the sockets in fds
    std::vector<pollfd> fds;
    std::unordered_map<int, std::string> client_buffers;
    fds.push_back({server_socket, POLLIN, 0});

    while (running) {
        // poll changes each pollfd.events to check if events are ready
        int ready = poll(fds.data(), fds.size(), -1);
        if (ready < 0) {
            perror("[ERROR] poll() failed");
            break;
        }

        for (size_t i = 0; i < fds.size(); ++i) {
            // Only works if there actually is something to read
            if (fds[i].revents & POLLIN) {
                int fd = fds[i].fd;

                // Accepting new client
                if (fd == server_socket) {                    
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int client_fd = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
                    if (client_fd < 0) {
                        perror("[ERROR] Accept failed");
                        continue;
                    }

                    int client_id = next_client_id++;
                    fd_to_client_id[client_fd] = client_id;

                    fcntl(client_fd, F_SETFL, O_NONBLOCK);

                    fds.push_back({client_fd, POLLIN, 0});
                    client_buffers[client_fd] = "";

                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
                    int port = ntohs(client_addr.sin_port);
                    std::cout << "[INFO] Client " << client_id << " connected: " << ip << ":" << port << std::endl;
                } 
                
                // Handling client message
                else {
                    std::string msg = recv_message_lenprefixed(fd);

                    if (msg.starts_with("[DISCONNECT]")) {
                        int client_id = fd_to_client_id[fd];
                        std::cout << "[INFO] Client " << client_id << " disconnected gracefully" << std::endl;
                        
                        close(fd);
                        client_buffers.erase(fd);
                        fd_to_client_id.erase(fd);
                        fds.erase(fds.begin() + i);
                        --i;
                        continue;
                    }

                    if (msg.starts_with("[ERROR]")) {
                        int client_id = fd_to_client_id[fd];
                        std::cerr << "[ERROR] Client " << client_id << ": " << msg << std::endl;
                        close(fd);
                        client_buffers.erase(fd);
                        fds.erase(fds.begin() + i);
                        --i;
                        continue;
                    }

                    int client_id = fd_to_client_id[fd];
                    std::cout << "[CLIENT " << client_id << "]: " << msg << std::endl;

                    if (!send_message_lenprefixed(fd, msg)) {
                        int client_id = fd_to_client_id[fd];
                        std::cerr << "[ERROR] Failed to send response to client " << client_id << std::endl;
                        close(fd);
                        client_buffers.erase(fd);
                        fds.erase(fds.begin() + i);
                        --i;
                        continue;
                    }
                }
            }
        }
    }

    close(server_socket);
}

void RedisServer::shutdown() {
    running = false;
    if (server_socket != -1) {
        close(server_socket);
    }
    std::cout << "[INFO] Server shutdown complete" << std::endl;
}

/**
 * Sends a single length-prefixed message to a client over TCP.
 * 
 * The message is sent using the following protocol format:
 * +------+--------+
 * | len  | msg    |
 * +------+--------+
 * 
 * The message consists of:
 * - A 4-byte little-endian unsigned integer indicating the length of the message
 * - A message body of that length
 * 
 * This function ensures that the entire message (length header and body) is sent
 * using repeated calls to send() if necessary.
 */
bool RedisServer::send_message_lenprefixed(int fd, const std::string &msg) {
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

/**
 * Reads a single length-prefixed message from a client over TCP.
 * 
 * The protocol format:
 * +------+--------+------+--------+--------
 * | len  | msg1   | len  | msg2   | ...
 * +------+--------+------+--------+--------
 * 
 * Each message consists of:
 * - A 4-byte little-endian unsigned integer indicating the length of the message body
 * - A variable-length message body of that length
 * 
 * - This function first reads exactly 4 bytes to determine the length of the incoming message,
 *   then reads exactly that many bytes to get the full message body. 
 * - If the client disconnects, or an error occurs during reading, a special string is returned.
 */
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
