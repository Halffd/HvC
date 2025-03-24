#pragma once
#include "types.hpp"
#include "WindowManager.hpp"
#include <string>
#include <chrono>
#include <memory>

namespace H {

enum class DisplayServer {
    Unknown,
    X11,
    Wayland
};

class Window {
public:
    explicit Window(cstr identifier = "", const int method = 0);
    ~Window() = default;

    // Window properties
    wID id{0};
    
    // Static methods
    static DisplayServer DetectDisplayServer();
    
    // Window operations
    void Activate(wID win = 0);
    void Close(wID win = 0);
    void Min(wID win = 0);
    void Max(wID win = 0);
    void Transparency(wID win = 0, int alpha = 255);
    void AlwaysOnTop(wID win = 0, bool top = true);
    
    // Window info
    std::string Title(wID win = 0);
    Rect Pos(wID win = 0);
    bool Active(wID win = 0);
    bool Exists(wID win = 0);

    // Window finding methods
    wID Find(cstr identifier);
    wID Find2(cstr identifier, cstr type = "title");
    
    template<typename T>
    wID FindT(T identifier) {
        if constexpr (std::is_same_v<T, std::string>) {
            return Find(identifier);
        } else if constexpr (std::is_same_v<T, const char*>) {
            return Find(std::string(identifier));
        } else if constexpr (std::is_same_v<T, wID>) {
            return identifier;
        } else if constexpr (std::is_same_v<T, int>) {
            return static_cast<wID>(identifier);
        }
        return 0;
    }

private:
    // Platform-specific implementations
    Rect GetPositionX11(wID win);
    Rect GetPositionWayland(wID win);
    #ifdef _WIN32
    Rect GetPositionWindows(wID win);
    #endif

    // Static members
    #ifdef __linux__
    static DisplayServer displayServer;
    #endif

    // Helper methods
    wID FindByTitle(cstr title);
    wID FindByClass(cstr className);
    wID GetwIDByPID(pID pid);
    wID GetwIDByProcessName(cstr processName);
    
    // X11 helper methods
    void SetAlwaysOnTopX11(wID win, bool top);
};

} // namespace H
