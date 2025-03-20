#pragma once
#include <string>

namespace H {
class Notifier {
public:
    static void Show(const std::string& message, int timeout=3000) {
#ifdef __linux__
        std::string cmd = "notify-send -t " + std::to_string(timeout) 
                        + " \"Hv2\" \"" + message + "\"";
        std::system(cmd.c_str());
#elif _WIN32
        // Windows toast notification implementation
#endif
    }

    static void ConfigReloaded() {
        Show("Configuration reloaded successfully");
    }

    static void Error(const std::string& message) {
#ifdef __linux__
        std::string cmd = "notify-send -u critical -t 5000 \"Hv2 Error\" \"" 
                        + message + "\"";
        std::system(cmd.c_str());
#endif
    }
};
}; 