#pragma once

namespace H {

// Simple rectangle structure for window positions
struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    
    Rect() = default;
    
    Rect(int x, int y, int width, int height)
        : x(x), y(y), width(width), height(height) {}
};

// Display server enum for Linux
enum class DisplayServer {
    X11,
    Wayland,
    Unknown
};

} // namespace H 