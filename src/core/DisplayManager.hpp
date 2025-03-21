#pragma once
#include <X11/Xlib.h>
#include <iostream>

namespace H {

class DisplayManager {
public:
    static Display* GetDisplay() {
        Initialize();
        if(!display) {
            throw std::runtime_error("Failed to initialize X11 display");
        }
        return display;
    }

    static Window GetRootWindow() {
        Initialize();
        if(!display) {
            throw std::runtime_error("No X11 display available");
        }
        return root;
    }

    static void Initialize() {
        if (!display) {
            display = XOpenDisplay(nullptr);
            if (display) {
                root = DefaultRootWindow(display);
                // Register cleanup on exit
                static Cleanup cleanup;
                XSetErrorHandler(X11ErrorHandler);
                XSetIOErrorHandler([](Display* display) -> int {
                    std::cerr << "X11 I/O Error - Display connection lost\n";
                    std::exit(EXIT_FAILURE);
                });
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

    static int X11ErrorHandler(Display* display, XErrorEvent* error) {
        char errorText[256];
        XGetErrorText(display, error->error_code, errorText, sizeof(errorText));
        std::cerr << "X11 Error: " << errorText 
                  << " (Request: " << (int)error->request_code
                  << ", ResourceID: 0x" << std::hex << error->resourceid << ")\n";
        return 0;
    }

    // RAII wrapper for X11 resources
    template<typename T, auto Deleter>
    class X11Resource {
    public:
        X11Resource(T resource = nullptr) : resource(resource) {}
        ~X11Resource() { if(resource) Deleter(resource); }
        
        operator T() const { return resource; }
        T* operator&() { return &resource; }
        
    private:
        T resource;
    };

    // Usage example
    using XWindow = X11Resource<Window, XDestroyWindow>;
    using XColormap = X11Resource<Colormap, XFreeColormap>;
};

// Static member initialization
Display* DisplayManager::display = nullptr;
Window DisplayManager::root = 0;

} // namespace H 