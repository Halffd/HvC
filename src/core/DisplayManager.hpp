#pragma once
#include <X11/Xlib.h>

namespace H {

class DisplayManager {
public:
    static Display* GetDisplay() {
        Initialize();
        return display;
    }

    static Window GetRootWindow() {
        Initialize();
        return root;
    }

    static void Initialize() {
        if (!display) {
            display = XOpenDisplay(nullptr);
            if (display) {
                root = DefaultRootWindow(display);
                // Register cleanup on exit
                static Cleanup cleanup;
            }
        }
    }

    static void Close() {
        if (display) {
            XCloseDisplay(display);
            display = nullptr;
            root = 0;
        }
    }

    static bool IsInitialized() { return display != nullptr; }

private:
    static Display* display;
    static Window root;

    struct Cleanup {
        ~Cleanup() { DisplayManager::Close(); }
    };
};

} // namespace H 