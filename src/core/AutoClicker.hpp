#pragma once
#include "IO.hpp"
#include <atomic>
#include <thread>

namespace H {
class AutoClicker {
public:
    AutoClicker(IO& io) : io(io) {}
    
    void Toggle(int intervalMs = 100) {
        if(running) {
            Stop();
        } else {
            Start(intervalMs);
        }
    }

private:
    IO& io;
    std::atomic<bool> running{false};
    std::thread clickThread;

    void Start(int interval) {
        running = true;
        clickThread = std::thread([this, interval]{
            while(running) {
                io.MouseClick(1); // Left click
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(interval)
                );
            }
        });
    }

    void Stop() {
        running = false;
        if(clickThread.joinable()) {
            clickThread.join();
        }
    }
};
} 