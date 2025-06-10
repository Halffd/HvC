#include "AutoPresser.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>

namespace havel {

AutoPresser::AutoPresser(std::shared_ptr<IO> io) : io(io) {
    // Initialize with default values
}

AutoPresser::~AutoPresser() {
    // Make sure to stop the presser thread when the object is destroyed
    Stop();
}

void AutoPresser::Start(const std::vector<std::string>& keys, int intervalMs) {
    // Don't start if already running
    if (running.load()) {
        Stop();
    }
    
    // Set the interval and keys
    interval = std::chrono::milliseconds(intervalMs);
    
    {
        std::lock_guard<std::mutex> lock(keysMutex);
        keysToPress = keys;
    }
    
    // Set the running flag
    running.store(true);
    
    // Start the presser thread
    presserThread = std::thread(&AutoPresser::PresserLoop, this);
    
    Logger::getInstance().info("AutoPresser started with interval " + std::to_string(intervalMs) + " ms");
}

void AutoPresser::Stop() {
    // Set the running flag to false
    bool wasRunning = running.exchange(false);
    
    // Join the thread if it's joinable
    if (wasRunning && presserThread.joinable()) {
        presserThread.join();
        Logger::getInstance().info("AutoPresser stopped");
    }
}

bool AutoPresser::IsRunning() const {
    return running.load();
}

void AutoPresser::SetPressDuration(int durationMs) {
    pressDuration = std::chrono::milliseconds(durationMs);
}

void AutoPresser::PresserLoop() {
    while (running.load()) {
        std::vector<std::string> currentKeys;
        
        // Make a local copy of keys to minimize lock time
        {
            std::lock_guard<std::mutex> lock(keysMutex);
            currentKeys = keysToPress;
        }
        
        // Press each key in sequence
        for (const auto& key : currentKeys) {
            if (!running.load()) break;
            PerformKeyPress(key);
            
            // Small delay between key presses
            if (running.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        
        // Sleep for the specified interval
        if (running.load()) {
            std::this_thread::sleep_for(interval);
        }
    }
}

void AutoPresser::PerformKeyPress(const std::string& key) {
    try {
        // Press the key
        io->SendX11Key(key, true);  // Key down
        
        // Hold for the specified duration
        std::this_thread::sleep_for(pressDuration);
        
        // Release the key
        io->SendX11Key(key, false); // Key up
        
    } catch (const std::exception& e) {
        Logger::getInstance().error("Error in AutoPresser: " + std::string(e.what()));
    }
}

} // namespace havel
