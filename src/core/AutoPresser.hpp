#pragma once

#include "IO.hpp"
#include <atomic>
#include <thread>
#include <string>
#include <chrono>
#include <vector>
#include <mutex>

namespace havel {

class AutoPresser {
public:
    AutoPresser(std::shared_ptr<IO> io);
    ~AutoPresser();

    // Start auto-pressing with the specified interval in milliseconds
    void Start(const std::vector<std::string>& keys, int intervalMs);
    
    // Stop auto-pressing
    void Stop();
    
    // Check if auto-presser is running
    bool IsRunning() const;
    
    // Set the press duration in milliseconds
    void SetPressDuration(int durationMs);

private:
    std::shared_ptr<IO> io;
    std::atomic<bool> running{false};
    std::thread presserThread;
    std::chrono::milliseconds interval{100};
    std::chrono::milliseconds pressDuration{50};
    std::vector<std::string> keysToPress;
    mutable std::mutex keysMutex;
    
    void PresserLoop();
    void PerformKeyPress(const std::string& key);
};

} // namespace havel
