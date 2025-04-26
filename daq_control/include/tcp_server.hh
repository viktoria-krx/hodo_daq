#ifndef TCP_SERVER_HH
#define TCP_SERVER_HH

#include <string>
#include <thread>
#include <atomic>

class TCPServer {
public:
    TCPServer(int port);
    ~TCPServer();

    void start();
    void stop();

    bool isRunning() const {
        return running;
    }
    

private:
    void run();  // Main loop for handling client connections

    int server_fd;
    int port;
    std::thread serverThread;
    std::atomic<bool> running;
};

#endif  // TCP_SERVER_HH