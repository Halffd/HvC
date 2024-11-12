#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "WindowManager.hpp"

class Window : public WindowManager {
public:
    wID id;

    Window(cstr identifier, const int method = 1);

    // Find window using method 2
    wID Find2(cstr identifier, cstr type = "title");

    // Template function to find a window based on different identifiers
    template<typename T>
    wID FindT(const T& identifier);

    // Function to get the title of a window by ID
    str Title(wID win = NULL);

    // Check if a window is active
    bool Active(wID win = NULL);
    bool Exists(wID win);

    // Activate a window
    void Activate(wID win = NULL);

    // Close a window
    void Close(wID win = NULL);

    // Minimize a window
    void Min(wID win = NULL);

    // Maximize a window
    void Max(wID win = NULL);

    // Get the position of a window
    RECT Pos(wID win = NULL);

    // Set a window to always be on top
    void AlwaysOnTop(wID win = NULL, bool top = true);

    // Set the transparency of a window
    void Transparency(wID win = NULL, int alpha = 255);
};

#endif // WINDOW_HPP