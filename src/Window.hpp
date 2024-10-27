#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "WindowManager.hpp"

class Window : public WindowManager {
public:
    wID id;

    Window(cstr identifier, const int method = 1);

    // Find window using method 2
    HWND Find2(cstr identifier, cstr type = "title");

    // Template function to find a window based on different identifiers
    template<typename T>
    HWND FindT(const T& identifier);

    // Function to get the title of a window by ID
    str Title(HWND hwnd = NULL);

    // Check if a window is active
    bool Active(HWND hwnd = NULL);

    // Activate a window
    void Activate(HWND hwnd = NULL);

    // Close a window
    void Close(HWND hwnd = NULL);

    // Minimize a window
    void Min(HWND hwnd = NULL);

    // Maximize a window
    void Max(HWND hwnd = NULL);

    // Get the position of a window
    RECT Pos(HWND hwnd = NULL);

    // Set a window to always be on top
    void AlwaysOnTop(HWND hwnd = NULL, bool top = true);

    // Set the transparency of a window
    void Transparency(HWND hwnd = NULL, BYTE alpha = 255);
};

#endif // WINDOW_HPP