#pragma once
#include "IO.hpp"
#include <vector>
#include <chrono>

namespace H {
class MacroSystem {
public:
    struct MacroEvent {
        std::chrono::milliseconds timestamp;
        std::variant<std::string, std::pair<int, int>> data;
    };

    MacroSystem(IO& io) : io(io), recording(false) {}

    void StartRecording() {
        recording = true;
        macro.clear();
        startTime = std::chrono::steady_clock::now();
    }

    void StopAndSave(const std::string& name) {
        recording = false;
        macros[name] = macro;
    }

    void Play(const std::string& name) {
        if(macros.find(name) == macros.end()) return;
        
        const auto& events = macros[name];
        auto start = std::chrono::steady_clock::now();
        
        for(const auto& event : events) {
            std::this_thread::sleep_until(start + event.timestamp);
            if(auto key = std::get_if<std::string>(&event.data)) {
                io.Send(*key);
            }
            else if(auto pos = std::get_if<std::pair<int, int>>(&event.data)) {
                io.MouseMove(pos->first, pos->second);
            }
        }
    }

private:
    IO& io;
    bool recording;
    std::vector<MacroEvent> macro;
    std::unordered_map<std::string, std::vector<MacroEvent>> macros;
    std::chrono::steady_clock::time_point startTime;
};
} 