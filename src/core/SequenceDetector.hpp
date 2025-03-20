#pragma once
#include <vector>
#include <chrono>
#include <functional>

namespace H {
class SequenceDetector {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    
    SequenceDetector(std::vector<std::string> sequence, 
                    std::function<void()> action,
                    std::chrono::milliseconds timeout = 1000ms)
        : targetSequence(sequence), action(action), timeout(timeout) {}

    void ProcessEvent(const std::string& key) {
        auto now = std::chrono::steady_clock::now();
        
        if (!currentSequence.empty() && 
            (now - lastEventTime) > timeout) {
            Reset();
        }
        
        currentSequence.push_back(key);
        lastEventTime = now;
        
        if (currentSequence.size() > targetSequence.size()) {
            currentSequence.erase(currentSequence.begin());
        }
        
        if (currentSequence == targetSequence) {
            action();
            Reset();
        }
    }

private:
    std::vector<std::string> targetSequence;
    std::vector<std::string> currentSequence;
    std::function<void()> action;
    std::chrono::milliseconds timeout;
    TimePoint lastEventTime;
    
    void Reset() {
        currentSequence.clear();
        lastEventTime = TimePoint{};
    }
};
} 