#include "Window.hpp"

// Constructor
Window::Window(cstr identifier, const int method) {
    switch (method) {
        case 0: {
            id = FindT(identifier);
            break;
        }
        case 1: {
            id = Find(identifier);
            break;
        }
        case 2: {
            id = Find2(identifier);
            break;
        }
    }
}

// Find window using method 2
wID Window::Find2(cstr identifier, cstr type) {
    wID win = NULL;

    if (type == "title") {
        win = FindWindowA(NULL, identifier.c_str());
    } else if (type == "class") {
        win = FindWindowA(identifier.c_str(), NULL);
    } else if (type == "pid") {
        DWORD pid = std::stoi(identifier);
        win = GetwIDByPID(pid);
    }
    if (win) {
        lo << "Found window ID: " << win;
    } else {
        std::cerr << "Window not found!";
    }
    return win;
}

// Template function to find a window based on different identifiers
template<typename T>
wID Window::FindT(const T& identifier) {
    if constexpr (std::is_same<T, pID>::value) {
        return GetwIDByPID(identifier);
    } else if constexpr (std::is_same<T, wID>::value) {
        return identifier; // Already a wID
    } else {
        return Find(identifier);
    }
    return NULL;
}

// Function to get the title of a window by ID
str Window::Title(wID win) {
    if (!win) win = id; // Use object's id if not provided
    char title[256];
    GetWindowTextA(win, title, sizeof(title));
    return str(title);
}

// Check if a window is active
bool Window::Active(wID win) {
    if (!win) win = id; // Use object's id if not provided
    return GetForegroundWindow() == win;
}

// Activate a window
void Window::Activate(wID win) {
    if (!win) win = id; // Use object's id if not provided
    if (win) {
        SetForegroundWindow(win);
        lo << "Activated: " << win;
    }
}

// Close a window
void Window::Close(wID win) {
    if (!win) win = id; // Use object's id if not provided
    if (win) {
        SendMessage(win, WM_CLOSE, 0, 0);
        lo << "Closed: " << win;
    }
}

// Minimize a window
void Window::Min(wID win) {
    if (!win) win = id; // Use object's id if not provided
    if (win) {
        ShowWindow(win, SW_MINIMIZE);
        lo << "Minimized: " << win;
    }
}

// Maximize a window
void Window::Max(wID win) {
    if (!win) win = id; // Use object's id if not provided
    if (win) {
        ShowWindow(win, SW_MAXIMIZE);
        lo << "Maximized: " << win;
    }
}

// Get the position of a window
RECT Window::Pos(wID win) {
    if (!win) win = id; // Use object's id if not provided
    RECT rect;
    if (GetWindowRect(win, &rect)) {
        lo << "Position: (" << rect.left << ", " << rect.top << ", "
                  << rect.right << ", " << rect.bottom << ")";
    }
    return rect;
}

// Set a window to always be on top
void Window::AlwaysOnTop(wID win, bool top) {
    if (!win) win = id; // Use object's id if not provided
    if (win) {
        SetWindowPos(win, top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE);
        lo << (top ? "Set always on top." : "Removed always on top.");
    }
}
bool Window::Exists(wID win){
    return IsWindow(win);
}
// Set the transparency of a window
void Window::Transparency(wID win, int alpha) {
    if (!win) win = id; // Use object's id if not provided
    // Assuming win and alpha are already defined
    if (!win) {
        win = id; // Use object's id if not provided
    }
    if (win && Exists(win)) {
        // Ensure the window is layered
        SetWindowLong(win, GWL_EXSTYLE, GetWindowLong(win, GWL_EXSTYLE) | WS_EX_LAYERED);

        // Check alpha value
        if (alpha < 0 || alpha > 255) {
            lo << "Invalid alpha value: " << (int)alpha;
            return; // Handle error
        }

        // Set transparency
        if (!SetLayeredWindowAttributes(win, 0, alpha, LWA_ALPHA)) {
            std::cout << "Failed to set transparency. Error: " << GetLastError();
        } else {
            lo << "Set transparency to: " << (int)alpha;
        }

        // Force a redraw if necessary
        RedrawWindow(win, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    } else {
        lo << "Invalid window handle: " << win;
    }
}
