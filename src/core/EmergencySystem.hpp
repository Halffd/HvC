#pragma once
#include "IO.hpp"
#include <set>

namespace H {
class EmergencySystem {
public:
    static void RegisterResetHandler(IO& io) {
        io.Hotkey("ctrl+shift+z", []{
            // Release all keys
            for(int key : pressedKeys) {
                io.ReleaseVirtualKey(key);
            }
            pressedKeys.clear();
            
            // Reset mouse state
            io.MouseUp(1); // Left
            io.MouseUp(2); // Right
            io.MouseUp(3); // Middle
            
            Notifier::Show("Emergency Reset", "Input state cleared");
        });
    }

    static void TrackKeyPress(int key) {
        pressedKeys.insert(key);
    }

private:
    static std::set<int> pressedKeys;
};

std::set<int> EmergencySystem::pressedKeys;
} 