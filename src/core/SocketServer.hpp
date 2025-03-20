#pragma once
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <atomic>

namespace H {
class SocketServer {
public:
    void Start() {
        running = true;
        serverThread = std::thread([this]{ RunServer(); });
    }

    void Stop() {
        running = false;
        if (serverThread.joinable()) serverThread.join();
    }

    virtual ~SocketServer() {
        Stop();
    }

protected:
    virtual void HandleCommand(const std::string& cmd) = 0;

private:
    std::thread serverThread;
    std::atomic<bool> running{false};

    void RunServer() {
        int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd < 0) return;

        struct sockaddr_un addr = {};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/hv2.sock", sizeof(addr.sun_path)-1);

        unlink(addr.sun_path);
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sockfd);
            return;
        }

        listen(sockfd, 5);

        while (running) {
            int client = accept(sockfd, nullptr, nullptr);
            if (client < 0) continue;

            char buffer[256];
            ssize_t count = read(client, buffer, sizeof(buffer)-1);
            if (count > 0) {
                buffer[count] = '\0';
                HandleCommand(buffer);
            }
            close(client);
        }
        close(sockfd);
    }
};
} 