#include "Window.hpp"

#ifdef __linux__
DisplayServer H::Window::displayServer = DisplayServer::Unknown;

DisplayServer H::Window::DetectDisplayServer() {
    // Check if Wayland is active
    if (getenv("WAYLAND_DISPLAY")) {
        return DisplayServer::Wayland;
    }
    // Check if X11 is active
    if (getenv("DISPLAY")) {
        return DisplayServer::X11;
    }
    return DisplayServer::X11;
    //return DisplayServer::Unknown;
}
#endif

// Constructor
H::Window::Window(cstr identifier, const int method) {
    #ifdef __linux__
    displayServer = DetectDisplayServer();
    #endif

    switch (method) {
        case 0:
            id = FindT(identifier);
            break;
        case 1:
            id = Find(identifier);
            break;
        case 2:
            id = Find2(identifier);
            break;
    }
}

// Get the position of a window
H::Rect H::Window::Pos(wID win) {
    if (!win) win = id;

#if defined(WINDOWS)
    return GetPositionWindows(win);
#elif defined(__linux__)
    switch (displayServer) {
        case DisplayServer::X11:
            return GetPositionX11(win);
        case DisplayServer::Wayland:
            return GetPositionWayland(win);
        default:
            return H::Rect(0, 0, 0, 0);
    }
#else
    return H::Rect(0, 0, 0, 0);
#endif
}

#if defined(WINDOWS)
// Windows implementation of GetPosition
H::Rect H::Window::GetPositionWindows(wID win) {
    RECT rect;
    if (GetWindowRect(reinterpret_cast<HWND>(win), &rect)) {
        return H::Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    }
    return H::Rect(0, 0, 0, 0);
}
#endif

// X11 implementation of GetPosition
H::Rect H::Window::GetPositionX11(wID win) {
    Display* display = XOpenDisplay(nullptr);
    if (!display) return H::Rect(0, 0, 0, 0);  

    XWindowAttributes attr;
    if (XGetWindowAttributes(display, win, &attr)) {
        return H::Rect(attr.x, attr.y, attr.width, attr.height);
    }
    return H::Rect(0, 0, 0, 0);
}

// Wayland implementation of GetPosition
H::Rect H::Window::GetPositionWayland(wID win) {
    // Placeholder for Wayland logic. Requires implementation using Wayland libraries.
    std::cerr << "Wayland support is a work in progress." << std::endl;
    return H::Rect(0, 0, 0, 0);
}

// Find window using method 2
wID H::Window::Find2(cstr identifier, cstr type) {
    wID win = 0;

    if (type == "title") {
        win = FindByTitle(identifier.c_str());
    } else if (type == "class") {
        win = FindByClass(identifier.c_str());
    } else if (type == "pid") {
        pID pid = std::stoi(identifier);
        win = GetwIDByPID(pid);
    }
    if (win) {
        lo << "Found window ID: " + win;
    } else {
        std::cerr << "Window not found!";
    }
    return win;
}

// Template function to find a window based on different identifiers
template<typename T>
wID H::Window::FindT(const T& identifier) {
    if constexpr (std::is_same<T, pID>::value) {
        return GetwIDByPID(identifier);
    } else if constexpr (std::is_same<T, wID>::value) {
        return identifier; // Already a wID
    } else {
        return Find(identifier);
    }
    return 0;
}
#include "Window.hpp"
#include <iostream>
#include <sstream>

// Title retrieval
std::string H::Window::Title(wID win) {
    if (!win) win = id;

#ifdef WINDOWS
    char title[256];
    if (GetWindowTextA(reinterpret_cast<HWND>(win), title, sizeof(title))) {
        return std::string(title);
    }
    return "";
#elif defined(__linux__) && defined(__X11__)
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X11 display." << std::endl;
        return "";
    }

    Atom wmName = XInternAtom(display, "_NET_WM_NAME", True);
    if (wmName == None) {
        return "";
    }

    Atom actualType;
    int actualFormat;
    unsigned long nitems, bytesAfter;
    unsigned char* prop = nullptr;

    if (XGetWindowProperty(display, win, wmName, 0, (~0L), False, AnyPropertyType,
                           &actualType, &actualFormat, &nitems, &bytesAfter, &prop) == Success) {
        if (prop) {
            std::string title(reinterpret_cast<char*>(prop));
            XFree(prop);
            
            return title;
        }
    }
    return "";
#elif defined(__linux__) && defined(__WAYLAND__)
    // Placeholder for Wayland: Use wmctrl as a fallback
    std::ostringstream command;
    command << "wmctrl -l | grep " << reinterpret_cast<uintptr_t>(win);

    FILE* pipe = popen(command.str().c_str(), "r");
    if (!pipe) return "";

    char buffer[256];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output += buffer;
    }
    pclose(pipe);

    return output; // Parse the output as needed
#else
    return "";
#endif
}

// Check if a window is active
bool H::Window::Active(wID win) {
    if (!win) win = id;

#if defined(WINDOWS)
    return GetForegroundWindow() == reinterpret_cast<HWND>(win);
#elif defined(__linux__)
    Display* display = XOpenDisplay(nullptr);
    if (!display) return false;

    wID active;
    Atom activeAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
    if (activeAtom == None) return false;

    Atom actualType;
    int actualFormat;
    unsigned long nitems, bytesAfter;
    unsigned char* prop = nullptr;

    if (XGetWindowProperty(display, DefaultRootWindow(display), activeAtom, 0, (~0L), False, AnyPropertyType,
                           &actualType, &actualFormat, &nitems, &bytesAfter, &prop) == Success) {
        if (prop) {
            active = *reinterpret_cast<wID*>(prop);
            XFree(prop);
            return active == win;
        }
    }
    return false;
#else
    return false;
#endif
}

// Function to check if a window exists
bool H::Window::Exists(wID win) {
    if (!win) win = id;

#ifdef WINDOWS
    return IsWindow(reinterpret_cast<HWND>(win)) != 0;
#elif defined(__linux__)
    Display* display = XOpenDisplay(nullptr);
    if (!display) return false;

    XWindowAttributes attr;
    bool exists = XGetWindowAttributes(display, win, &attr) != 0;
    return exists;
#elif defined(__linux__) && defined(__WAYLAND__)
    // Wayland does not provide a direct API to check window existence.
    std::cerr << "Window existence check in Wayland is not implemented." << std::endl;
    return false;
#else
    return false;
#endif
}
void H::Window::Activate(wID win) {
    if (!win) win = id;

#ifdef WINDOWS
    // Windows implementation
    if (win) {
        SetForegroundWindow(reinterpret_cast<HWND>(win));
        std::cout << "Activated: " << win << std::endl;
    }
#elif defined(__linux__)
    // X11 implementation
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    Atom activeAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
    if (activeAtom != None) {
        XEvent event = {};
        event.xclient.type = ClientMessage;
        event.xclient.window = win;
        event.xclient.message_type = activeAtom;
        event.xclient.format = 32;
        event.xclient.data.l[0] = 1; // Source indication: 1 (application)
        event.xclient.data.l[1] = CurrentTime;

        XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XFlush(display);
    } else {
        std::cerr << "Failed to find _NET_ACTIVE_WINDOW atom." << std::endl;
    }
#elif defined(__linux__) && defined(__WAYLAND__)
    // Wayland implementation using `wmctrl`
    if (win) {
        std::ostringstream command;
        command << "wmctrl -i -a " << std::hex << reinterpret_cast<uintptr_t>(win);
        int ret = system(command.str().c_str());
        if (ret == -1) {
            std::cerr << "Failed to execute wmctrl command to activate window." << std::endl;
        } else {
            std::cout << "Activated window via wmctrl: " << win << std::endl;
        }
    } else {
        std::cerr << "Invalid window ID for Wayland activation." << std::endl;
    }
#else
    std::cerr << "Platform not supported for Activate function." << std::endl;
#endif
}

// Close a window
void H::Window::Close(wID win) {
    if (!win) win = id;

#ifdef WINDOWS
    if (win) {
        SendMessage(reinterpret_cast<HWND>(win), WM_CLOSE, 0, 0);
        std::cout << "Closed: " << win << std::endl;
    }
#elif defined(__linux__)
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", True);
    if (wmDelete != None) {
        XEvent event = {};
        event.xclient.type = ClientMessage;
        event.xclient.message_type = wmDelete;
        event.xclient.format = 32;
        event.xclient.data.l[0] = CurrentTime;

        XSendEvent(display, win, False, NoEventMask, &event);
        XFlush(display);
    }
#elif defined(__linux__) && defined(__WAYLAND__)
    // Wayland does not provide a universal API for closing windows.
    std::cerr << "Window closing in Wayland is not implemented." << std::endl;
#endif
}
void H::Window::Min(wID win) {
    if (!win) win = id;
    if (!win) return; // Early return if no valid window ID

#ifdef WINDOWS
    if (win) {
        ShowWindow(reinterpret_cast<HWND>(win), SW_MINIMIZE);
        std::cout << "Minimized: " << win << std::endl;
    }
#elif defined(__linux__) 
    if (!display) {
        std::cerr << "Failed to open X display" << std::endl;
        return;
    }
   // XIconifyWindow takes (Display*, Window, int screen_number)
    XIconifyWindow(display, win, DefaultScreen(display));
    XFlush(display);  // Ensure the command is sent to the server
#elif defined(__linux__) && defined(__WAYLAND__)
    std::cerr << "Window minimization in Wayland is not implemented." << std::endl;
#endif
}
// Maximize a window
void H::Window::Max(wID win) {
    if (!win) win = id;

#ifdef WINDOWS
    if (win) {
        ShowWindow(reinterpret_cast<HWND>(win), SW_MAXIMIZE);
        std::cout << "Maximized: " << win << std::endl;
    }
#elif defined(__linux__)
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    Atom wmState = XInternAtom(display, "_NET_WM_STATE", True);
    Atom wmMaxVert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", True);
    Atom wmMaxHorz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", True);
    if (wmState != None && wmMaxVert != None && wmMaxHorz != None) {
        XEvent event = {};
        event.xclient.type = ClientMessage;
        event.xclient.window = win;
        event.xclient.message_type = wmState;
        event.xclient.format = 32;
        event.xclient.data.l[0] = 1; // Add
        event.xclient.data.l[1] = wmMaxVert;
        event.xclient.data.l[2] = wmMaxHorz;

        XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XFlush(display);
    }
#elif defined(__linux__) && defined(__WAYLAND__)
    // Wayland does not provide a universal API for maximizing windows.
    std::cerr << "Window maximization in Wayland is not implemented." << std::endl;
#endif
}

// Set the transparency of a window
void H::Window::Transparency(wID win, int alpha) {
    if (!win) win = id;

#ifdef WINDOWS
    if (win && alpha >= 0 && alpha <= 255) {
        SetWindowLong(reinterpret_cast<HWND>(win), GWL_EXSTYLE, GetWindowLong(reinterpret_cast<HWND>(win), GWL_EXSTYLE) | WS_EX_LAYERED);
        SetLayeredWindowAttributes(reinterpret_cast<HWND>(win), 0, alpha, LWA_ALPHA);
    }
#elif defined(__linux__)
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    Atom opacityAtom = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
    if (opacityAtom != None) {
        unsigned long opacity = static_cast<unsigned long>((alpha / 255.0) * 0xFFFFFFFF);
        XChangeProperty(display, win, opacityAtom, XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<unsigned char*>(&opacity), 1);
    }
    XFlush(display);
#elif defined(__linux__) && defined(__WAYLAND__)
    // Wayland does not provide a universal API for setting transparency.
    std::cerr << "Transparency control in Wayland is not implemented." << std::endl;
#endif
}

// Set a window to always be on top
void H::Window::AlwaysOnTop(wID win, bool top) {
    if (!win) win = id;

#ifdef WINDOWS
    SetWindowPos(reinterpret_cast<HWND>(win), top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#elif defined(__linux__)
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    Atom wmState = XInternAtom(display, "_NET_WM_STATE", True);
    Atom wmAbove = XInternAtom(display, "_NET_WM_STATE_ABOVE", True);
    if (wmState != None && wmAbove != None) {
        XEvent event = {};
        event.xclient.type = ClientMessage;
        event.xclient.window = win;
        event.xclient.message_type = wmState;
        event.xclient.format = 32;
        event.xclient.data.l[0] = top ? 1 : 0; // Add or Remove
        event.xclient.data.l[1] = wmAbove;

        XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XFlush(display);
    }
#elif defined(__linux__) && defined(__WAYLAND__)
    // Wayland does not provide a universal API for setting windows on top.
    std::cerr << "AlwaysOnTop in Wayland is not implemented." << std::endl;
#endif
}