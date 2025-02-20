#include "tcp_server.hh"
#include "logger.hh"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>


extern bool start_run();
extern void stop_run();
extern bool pause_run();
extern bool resume_run();
extern int daq_exit();

TCPServer::TCPServer(int port) : port(port), server_fd(-1), running(false) {}

TCPServer::~TCPServer() {
    stop();
}

void TCPServer::start() {
    running = true;
    serverThread = std::thread(&TCPServer::run, this);
}

void TCPServer::stop() {
    running = false;
    if (serverThread.joinable()) {
        serverThread.join();
    }
    if (server_fd != -1) {
        close(server_fd);
    }
}

void trim(std::string &str) {
    str.erase(str.find_last_not_of(" \t\r\n") + 1);
    str.erase(0, str.find_first_not_of(" \t\r\n"));
}

void TCPServer::run() {

    auto log = Logger::getLogger();
    log->info("Starting TCP server on port {}", port);

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        log->error("Failed to create socket");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log->error("Bind failed");
        return;
    }

    if (listen(server_fd, 5) < 0) {
        log->error("Listen failed");
        return;
    }

    log->info("TCP server listening on port {}", port);

    while (running) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            log->warn("Client connection failed");
            continue;
        }

        char buffer[1024] = {0};
        ssize_t bytesRead = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string command(buffer);
            trim(command);
            log->info("Received command: {}", command);

            if (command == "start") {
                start_run();
                if (write(client_fd, "Started\n", 8) == -1) {
                    log->error("Error writing to client");
                }
            } else if (command == "stop") {
                stop_run();
                if (write(client_fd, "Stopped\n", 8) == -1) {
                    log->error("Error writing to client");
                }
            } else if (command == "pause") {
                pause_run();
                if (write(client_fd, "Paused\n", 7) == -1) {
                    log->error("Error writing to client");
                }
            } else if (command == "resume") {
                resume_run();
                if (write(client_fd, "Resumed\n", 8) == -1) {
                    log->error("Error writing to client");
                }
            } else if (command == "!") {
                daq_exit();
                if (write(client_fd, "Stopping DAQ\n", 18) == -1) {
                    log->error("Error writing to client");
                }
            } else {
                if (write(client_fd, "Unknown command\n", 16) == -1) {
                    log->error("Error writing to client");
                }
            }
        }
        close(client_fd);
    }

    log->info("TCP server stopped");
}
