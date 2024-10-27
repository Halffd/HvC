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
HWND Window::Find2(cstr identifier, cstr type) {
    HWND hwnd = NULL;

    if (type == "title") {
        hwnd = FindWindowA(NULL, identifier.c_str());
    } else if (type == "class") {
        hwnd = FindWindowA(identifier.c_str(), NULL);
    } else if (type == "pid") {
        DWORD pid = std::stoi(identifier);
        hwnd = GetHWNDByPID(pid);
    }
    if (hwnd) {
        lo << "Found window ID: " << hwnd;
    } else {
        std::cerr << "Window not found!";
    }
    return hwnd;
}

// Template function to find a window based on different identifiers
template<typename T>
HWND Window::FindT(const T& identifier) {
    /*if constexpr (std::is_same<T, WindowTitle>::value) {
        return FindWindowA(NULL, identifier.c_str());
    } else if constexpr (std::is_same<T, WindowClass>::value) {
        return FindWindowA(identifier.c_str(), NULL);
    } else if constexpr (std::is_same<T, ProcessID>::value) {
        return GetHWNDByPID(identifier);
    } else if constexpr (std::is_same<T, WindowID>::value) {
        return identifier; // Already a HWND
    } else if constexpr (std::is_same<T, ProcessName>::value) {
        return GetHWNDByProcessName(identifier);
    }*/
    return NULL;
}

// Function to get the title of a window by ID
str Window::Title(HWND hwnd) {
    if (!hwnd) hwnd = id; // Use object's id if not provided
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    return str(title);
}

// Check if a window is active
bool Window::Active(HWND hwnd) {
    if (!hwnd) hwnd = id; // Use object's id if not provided
    return GetForegroundWindow() == hwnd;
}

// Activate a window
void Window::Activate(HWND hwnd) {
    if (!hwnd) hwnd = id; // Use object's id if not provided
    if (hwnd) {
        SetForegroundWindow(hwnd);
        lo << "Activated: " << hwnd;
    }
}

// Close a window
void Window::Close(HWND hwnd) {
    if (!hwnd) hwnd = id; // Use object's id if not provided
    if (hwnd) {
        SendMessage(hwnd, WM_CLOSE, 0, 0);
        lo << "Closed: " << hwnd;
    }
}

// Minimize a window
void Window::Min(HWND hwnd) {
    if (!hwnd) hwnd = id; // Use object's id if not provided
    if (hwnd) {
        ShowWindow(hwnd, SW_MINIMIZE);
        lo << "Minimized: " << hwnd;
    }
}

// Maximize a window
void Window::Max(HWND hwnd) {
    if (!hwnd) hwnd = id; // Use object's id if not provided
    if (hwnd) {
        ShowWindow(hwnd, SW_MAXIMIZE);
        lo << "Maximized: " << hwnd;
    }
}

// Get the position of a window
RECT Window::Pos(HWND hwnd) {
    if (!hwnd) hwnd = id; // Use object's id if not provided
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        lo << "Position: (" << rect.left << ", " << rect.top << ", "
                  << rect.right << ", " << rect.bottom << ")";
    }
    return rect;
}

// Set a window to always be on top
void Window::AlwaysOnTop(HWND hwnd, bool top) {
    if (!hwnd) hwnd = id; // Use object's id if not provided
    if (hwnd) {
        SetWindowPos(hwnd, top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE);
        lo << (top ? "Set always on top." : "Removed always on top.");
    }
}

// Set the transparency of a window
void Window::Transparency(HWND hwnd, BYTE alpha) {
    if (!hwnd) hwnd = id; // Use object's id if not provided
    if (hwnd) {
        SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
        lo << "Set transparency to: " << (int)alpha;
    }
}
