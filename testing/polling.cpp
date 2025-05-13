#include <iostream>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 12345
#define MAX_CLIENTS 1024
#define BUFFER_SIZE 1024

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket()");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind()");
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen()");
        return 1;
    }

    std::vector<pollfd> poll_fds;
    poll_fds.push_back({server_fd, POLLIN, 0});

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        int ready = poll(poll_fds.data(), poll_fds.size(), -1);
        if (ready < 0) {
            perror("poll()");
            break;
        }

        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (!(poll_fds[i].revents & POLLIN)) continue;

            if (poll_fds[i].fd == server_fd) {
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                if (client_fd >= 0) {
                    poll_fds.push_back({client_fd, POLLIN, 0});
                    std::cout << "Client connected: fd = " << client_fd << std::endl;
                }
            } else {
                char buffer[BUFFER_SIZE];
                ssize_t n = read(poll_fds[i].fd, buffer, sizeof(buffer) - 1);
                if (n <= 0) {
                    std::cout << "Client disconnected: fd = " << poll_fds[i].fd << std::endl;
                    close(poll_fds[i].fd);
                    poll_fds.erase(poll_fds.begin() + i);
                    --i;
                } else {
                    buffer[n] = '\0';
                    std::cout << "Received: " << buffer;
                    send(poll_fds[i].fd, buffer, n, 0);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
