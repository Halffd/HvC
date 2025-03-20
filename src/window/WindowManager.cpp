#include "WindowManager.hpp"
#include "types.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

// Define global variables
group groups;

#ifdef _WIN32
str defaultTerminal = "Cmd";
#else
Display* display = H::DisplayManager::GetDisplay();
Window root = H::DisplayManager::GetRootWindow();
str defaultTerminal = "konsole"; //gnome-terminal
cstr globalShell = "fish";
#endif

namespace H {

WindowManager::WindowManager() {
#ifdef _WIN32
    pid = GetCurrentProcessId();
#elif defined(__linux__)
    WindowManagerDetector detector;
    wmName = detector.GetWMName();
    wmType = detector.Detect();
    wmSupported = true;
    
    if (IsX11()) {
        InitializeX11();
    }
#endif
}

bool WindowManager::InitializeX11() {
#ifdef __linux__
    if (!display) {
        display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Failed to open X11 display" << std::endl;
            return false;
        }
        root = DefaultRootWindow(display);
    }
    return true;
#else
    return false;
#endif
}

// Function to add a group
void WindowManager::AddGroup(cstr groupName, cstr identifier) {
    if (groups.find(groupName) == groups.end()) {
        groups[groupName] = std::vector<std::string>();
    }
    groups[groupName].push_back(identifier);
}

str WindowManager::GetIdentifierType(cstr identifier) {
    std::istringstream iss(identifier);
    std::string type;
    std::getline(iss, type, ' ');
    return type;
}

str WindowManager::GetIdentifierValue(cstr identifier) {
    std::istringstream iss(identifier);
    std::string type;
    std::getline(iss, type, ' ');
    std::string value;
    std::getline(iss, value);
    return value;
}

wID WindowManager::GetActiveWindow() {
#ifdef _WIN32
    return reinterpret_cast<wID>(GetForegroundWindow());
#elif defined(__linux__)
    if (!display) {
        if (!WindowManager::InitializeX11()) {
            return 0;
        }
    }
    
    Atom activeAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
    if (activeAtom == None) {
        return 0;
    }
    
    Atom actualType;
    int actualFormat;
    unsigned long nItems, bytesAfter;
    unsigned char* prop = nullptr;
    
    if (XGetWindowProperty(display, root, activeAtom, 0, (~0L), False, AnyPropertyType,
                         &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == Success) {
        if (nItems > 0) {
            wID activeWindow = *reinterpret_cast<Window*>(prop);
            XFree(prop);
            return activeWindow;
        }
        if (prop) XFree(prop);
    }
    return 0;
#else
    return 0;
#endif
}

// Method to find a window based on various identifiers
wID WindowManager::Find(cstr identifier) {
    std::string type = GetIdentifierType(identifier);
    std::string value = GetIdentifierValue(identifier);
    
    if (type == "group") {
        return FindWindowInGroup(value);
    } else if (type == "class") {
#ifdef _WIN32
        return reinterpret_cast<wID>(FindWindowA(value.c_str(), NULL));
#elif defined(__linux__)
        return FindByClass(value);
#endif
    } else if (type == "pid") {
        pID pid = std::stoul(value);
        return GetwIDByPID(pid);
    } else if (type == "exe") {
        return GetwIDByProcessName(value);
    } else if (type == "title") {
        return FindByTitle(value);
    } else if (type == "id") {
        return std::stoul(value);
    } else {
        // Default to title search if no type specified
        return FindByTitle(identifier);
    }
    return 0; // Unsupported platform
}

void WindowManager::AltTab() {
#ifdef __linux__
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;
    
    // Get the root window
    Window root = DefaultRootWindow(display);
    
    // Get the list of windows
    Atom clientListAtom = XInternAtom(display, "_NET_CLIENT_LIST_STACKING", False);
    if (clientListAtom == None) {
        XCloseDisplay(display);
        return;
    }
    
    Atom actualType;
    int actualFormat;
    unsigned long numWindows, bytesAfter;
    unsigned char* data = nullptr;
    
    Status status = XGetWindowProperty(display, root, clientListAtom,
                                   0, ~0L, False, XA_WINDOW,
                                   &actualType, &actualFormat,
                                   &numWindows, &bytesAfter, &data);
    
    if (status != Success || numWindows < 2) {
        if (data) XFree(data);
        return;
    }
    
    Window* windows = reinterpret_cast<Window*>(data);
    
    // Get the currently active window
    Atom activeWindowAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    unsigned char* activeData = nullptr;
    unsigned long activeItems;
    
    status = XGetWindowProperty(display, root, activeWindowAtom,
                               0, 1, False, XA_WINDOW,
                               &actualType, &actualFormat,
                               &activeItems, &bytesAfter, &activeData);
                               
    Window activeWindow = None;
    if (status == Success && activeItems > 0) {
        activeWindow = *reinterpret_cast<Window*>(activeData);
    }
    if (activeData) XFree(activeData);
    
    // Find the next window to activate
    Window windowToActivate = None;
    
    // Iterate through windows from top to bottom
    for (int i = numWindows - 1; i >= 0; i--) {
        Window currentWindow = windows[i];
        
        if (currentWindow == activeWindow) {
            // Found the active window, get the next one
            for (int j = i - 1; j >= 0; j--) {
                // Check if the window is visible
                XWindowAttributes attrs;
                if (XGetWindowAttributes(display, windows[j], &attrs) && 
                    attrs.map_state == IsViewable) {
                    // Check if the window has a title (skip desktop, panels, etc.)
                    char* windowName = nullptr;
                    if (XFetchName(display, windows[j], &windowName) && windowName) {
                        XFree(windowName);
                        windowToActivate = windows[j];
                        break;
                    }
                }
            }
            
            // If no suitable window found before the active one, wrap around
            if (windowToActivate == None) {
                for (int j = numWindows - 1; j > i; j--) {
                    XWindowAttributes attrs;
                    if (XGetWindowAttributes(display, windows[j], &attrs) && 
                        attrs.map_state == IsViewable) {
                        char* windowName = nullptr;
                        if (XFetchName(display, windows[j], &windowName) && windowName) {
                            XFree(windowName);
                            windowToActivate = windows[j];
                            break;
                        }
                    }
                }
            }
            
            break;
        }
    }
    
    // If no window to activate was found, use the top-most window
    if (windowToActivate == None && numWindows > 0) {
        for (int i = numWindows - 1; i >= 0; i--) {
            XWindowAttributes attrs;
            if (XGetWindowAttributes(display, windows[i], &attrs) && 
                attrs.map_state == IsViewable && 
                windows[i] != activeWindow) {
                char* windowName = nullptr;
                if (XFetchName(display, windows[i], &windowName) && windowName) {
                    XFree(windowName);
                    windowToActivate = windows[i];
                    break;
                }
            }
        }
    }
    
    // Activate the selected window
    if (windowToActivate != None) {
        // Create activation event
        XEvent event = {};
        event.type = ClientMessage;
        event.xclient.type = ClientMessage;
        event.xclient.window = windowToActivate;
        event.xclient.message_type = activeWindowAtom;
        event.xclient.format = 32;
        event.xclient.data.l[0] = 2; // Source indication: pager
        event.xclient.data.l[1] = CurrentTime;
        event.xclient.data.l[2] = 0;
        
        // Send the event
        XSendEvent(display, root, False,
                   SubstructureRedirectMask | SubstructureNotifyMask,
                   &event);
                   
        // Raise the window
        XRaiseWindow(display, windowToActivate);
        
        // Focus the window
        XSetInputFocus(display, windowToActivate, RevertToParent, CurrentTime);
    }
    
    if (data) XFree(data);
    XSync(display, False);
#endif
}

wID WindowManager::GetwIDByPID(pID pid) {
#ifdef _WIN32
    wID hwnd = NULL;
    // Windows implementation would go here
    return hwnd;
#elif defined(__linux__)
    if (!display) {
        if (!WindowManager::InitializeX11()) {
            return 0;
        }
    }
    
    Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);
    if (pidAtom == None) {
        std::cerr << "X11 does not support _NET_WM_PID." << std::endl;
        return 0;
    }
    
    Window root = DefaultRootWindow(display);
    Window parent, *children;
    unsigned int nchildren;
    
    if (XQueryTree(display, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            Atom actualType;
            int actualFormat;
            unsigned long nItems, bytesAfter;
            unsigned char *propPID = nullptr;
            
            if (XGetWindowProperty(display, children[i], pidAtom, 0, 1, False, XA_CARDINAL,
                                 &actualType, &actualFormat, &nItems, &bytesAfter, &propPID) == Success) {
                if (nItems > 0) {
                    pid_t windowPID = *reinterpret_cast<pid_t*>(propPID);
                    if (windowPID == pid) {
                        XFree(propPID);
                        XFree(children);
                        return reinterpret_cast<wID>(children[i]);
                    }
                }
                if (propPID) XFree(propPID);
            }
        }
        XFree(children);
    }
    return 0;
#else
    return 0;
#endif
}

wID WindowManager::GetwIDByProcessName(cstr processName) {
#ifdef _WIN32
    // Windows implementation would go here
    return 0;
#elif defined(__linux__)
    if (!display) {
        if (!WindowManager::InitializeX11()) {
            return 0;
        }
    }
    
    Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);
    if (pidAtom == None) {
        std::cerr << "X11 does not support _NET_WM_PID." << std::endl;
        return 0;
    }
    
    Window root = DefaultRootWindow(display);
    Window parent, *children;
    unsigned int nchildren;
    
    if (XQueryTree(display, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            Atom actualType;
            int actualFormat;
            unsigned long nItems, bytesAfter;
            unsigned char *propPID = nullptr;
            
            if (XGetWindowProperty(display, children[i], pidAtom, 0, 1, False, XA_CARDINAL,
                                 &actualType, &actualFormat, &nItems, &bytesAfter, &propPID) == Success) {
                if (nItems > 0) {
                    pid_t windowPID = *reinterpret_cast<pid_t*>(propPID);
                    std::string procName = getProcessName(windowPID);
                    if (procName == processName) {
                        XFree(children);
                        return reinterpret_cast<wID>(children[i]);
                    }
                }
                if (propPID) XFree(propPID);
            }
        }
        XFree(children);
    }
    return 0;
#else
    return 0;
#endif
}

// Find a window by class
wID WindowManager::FindByClass(cstr className) {
#ifdef _WIN32
    // Windows implementation would go here
    return 0;
#elif defined(__linux__)
    if (!display) {
        if (!WindowManager::InitializeX11()) {
            return 0;
        }
    }
    
    Window root = DefaultRootWindow(display);
    Window parent, *children;
    unsigned int nchildren;
    
    if (XQueryTree(display, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            XClassHint classHint;
            if (XGetClassHint(display, children[i], &classHint)) {
                bool match = (className == classHint.res_name || className == classHint.res_class);
                XFree(classHint.res_name);
                XFree(classHint.res_class);
                
                if (match) {
                    XFree(children);
                    return reinterpret_cast<wID>(children[i]);
                }
            }
        }
        XFree(children);
    }
    return 0;
#else
    return 0;
#endif
}

wID WindowManager::FindByTitle(cstr title) {
#ifdef _WIN32
    // Windows implementation would go here
    return 0;
#elif defined(__linux__)
    if (!display) {
        if (!WindowManager::InitializeX11()) {
            return 0;
        }
    }
    
    Window root = DefaultRootWindow(display);
    Window parent, *children;
    unsigned int nchildren;
    
    if (XQueryTree(display, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            char* windowName = nullptr;
            if (XFetchName(display, children[i], &windowName) && windowName) {
                bool match = (title == windowName);
                XFree(windowName);
                
                if (match) {
                    XFree(children);
                    return reinterpret_cast<wID>(children[i]);
                }
            }
        }
        XFree(children);
    }
    return 0;
#endif
    return 0;
}

std::string WindowManager::getProcessName(pid_t windowPID) {
#ifdef __linux__
    std::ostringstream procPath;
    procPath << "/proc/" << windowPID << "/comm";
    
    std::string path = procPath.str();
    FILE* procFile = fopen(path.c_str(), "r");
    
    if (procFile) {
        char procName[256];
        size_t len = fread(procName, 1, sizeof(procName) - 1, procFile);
        procName[len] = '\0';
        
        // Remove trailing newline if present
        if (len > 0 && procName[len - 1] == '\n') {
            procName[len - 1] = '\0'; // Replace newline with null terminator
        }
        
        fclose(procFile);
        return std::string(procName); // Return the process name as a string
    } else {
        std::cerr << "Error: Could not read from file " << path << ": " << std::strerror(errno) << "\n";
        return ""; // Handle the error as needed
    }
#else
    return "";
#endif
}

wID WindowManager::FindWindowInGroup(cstr groupName) {
    auto it = groups.find(groupName);
    if (it != groups.end()) {
        for (const auto& identifier : it->second) {
            wID hwnd = Find(identifier);
            if (hwnd) {
                return hwnd;
            }
        }
    }
    return 0;
}

wID WindowManager::NewWindow(cstr name, std::vector<int>* dimensions, bool hide) {
#ifdef _WIN32
    // Windows implementation would go here
    return 0;
#elif defined(__linux__)
    if (!display) {
        if (!WindowManager::InitializeX11()) {
            return 0;
        }
    }
    
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    int x = 0, y = 0, width = 800, height = 600;
    
    if (dimensions && dimensions->size() == 4) {
        x = (*dimensions)[0];
        y = (*dimensions)[1];
        width = (*dimensions)[2];
        height = (*dimensions)[3];
    }
    
    Window newWindow = XCreateSimpleWindow(display, root, x, y, width, height, 1,
                                            BlackPixel(display, screen), WhitePixel(display, screen));
    
    XStoreName(display, newWindow, name.c_str());
    if (!hide) {
        XMapWindow(display, newWindow);
    }
    XFlush(display);
    
    return reinterpret_cast<wID>(newWindow);
#else
    std::cerr << "NewWindow not supported on this platform." << std::endl;
    return 0;
#endif
}

ProcessMethod WindowManager::toMethod(cstr method) {
    std::string lowerMethod = ToLower(method);
    
    if (lowerMethod == "continue" || lowerMethod == "continueexecution") {
        return ProcessMethod::ContinueExecution;
    } else if (lowerMethod == "wait" || lowerMethod == "waitforterminate") {
        return ProcessMethod::WaitForTerminate;
    } else if (lowerMethod == "waituntilstarts") {
        return ProcessMethod::WaitUntilStarts;
    } else if (lowerMethod == "system" || lowerMethod == "systemcall") {
        return ProcessMethod::SystemCall;
    } else if (lowerMethod == "async" || lowerMethod == "asyncprocesscreate") {
        return ProcessMethod::AsyncProcessCreate;
    } else if (lowerMethod == "fork" || lowerMethod == "forkprocess") {
        return ProcessMethod::ForkProcess;
    } else if (lowerMethod == "new" || lowerMethod == "createnewwindow") {
        return ProcessMethod::CreateNewWindow;
    } else if (lowerMethod == "same" || lowerMethod == "samewindow") {
        return ProcessMethod::SameWindow;
    } else if (lowerMethod == "shell") {
        return ProcessMethod::Shell;
    } else {
        return ProcessMethod::Invalid;
    }
}

#ifdef _WIN32
// Function to convert error code to a human-readable message
str WindowManager::GetErrorMessage(pID errorCode) {
    // Windows implementation would go here
    return "Unknown error";
}

// Function to create a process and handle common logic
bool WindowManager::CreateProcessWrapper(cstr path, cstr command, pID creationFlags, STARTUPINFO& si, PROCESS_INFORMATION& pi) {
    // Windows implementation would go here
    return false;
}
#endif

std::string WindowManager::GetCurrentWMName() const {
    return wmName;
}

bool WindowManager::IsWMSupported() const {
    return wmSupported;
}

bool WindowManager::IsX11() const {
    return WindowManagerDetector().IsX11();
}

bool WindowManager::IsWayland() const {
    return WindowManagerDetector().IsWayland();
}

void WindowManager::All() {
    // Implementation for All method
}

std::string WindowManager::DetectWindowManager() const {
#ifdef __linux__
    // Try to get window manager name from _NET_SUPPORTING_WM_CHECK
    if (!display) {
        const_cast<WindowManager*>(this)->InitializeX11();
        if (!display) return "Unknown";
    }
    
    Atom netSupportingWmCheck = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
    Atom netWmName = XInternAtom(display, "_NET_WM_NAME", False);
    
    if (netSupportingWmCheck != None && netWmName != None) {
        Atom actualType;
        int actualFormat;
        unsigned long nItems, bytesAfter;
        unsigned char* data = nullptr;
        
        if (XGetWindowProperty(display, DefaultRootWindow(display), netSupportingWmCheck,
                             0, 1, False, XA_WINDOW, &actualType, &actualFormat,
                             &nItems, &bytesAfter, &data) == Success && data) {
            
            Window wmWindow = *(reinterpret_cast<Window*>(data));
            XFree(data);
            
            if (XGetWindowProperty(display, wmWindow, netWmName, 0, 1024, False,
                                 XInternAtom(display, "UTF8_STRING", False), &actualType, &actualFormat,
                                 &nItems, &bytesAfter, &data) == Success && data) {
                
                std::string name(reinterpret_cast<char*>(data));
                XFree(data);
                return name;
            }
        }
    }
    
    return "Unknown";
#else
    return "Unknown";
#endif
}

bool WindowManager::CheckWMProtocols() const {
#ifdef __linux__
    if (!display) return false;
    
    Atom wmProtocols = XInternAtom(display, "WM_PROTOCOLS", False);
    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    Atom wmTakeFocus = XInternAtom(display, "WM_TAKE_FOCUS", False);
    
    if (wmProtocols != None && wmDeleteWindow != None && wmTakeFocus != None) {
        Window dummyWindow = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                               0, 0, 1, 1, 0, 0, 0);
        
        Atom* protocols = nullptr;
        int numProtocols = 0;
        bool hasRequiredProtocols = false;
        
        if (XGetWMProtocols(display, dummyWindow, &protocols, &numProtocols)) {
            // Check if the required protocols are supported
            hasRequiredProtocols = true;
            XFree(protocols);
        }
        
        XDestroyWindow(display, dummyWindow);
        return hasRequiredProtocols;
    }
    
    return false;
#else
    return true;
#endif
}

int64_t WindowManager::Terminal(cstr command, bool canPause, str windowState, bool continueExecution, cstr terminal) {
#ifdef __linux__
    pid_t pid = fork();
    if (pid == 0) {
        const char* term = terminal.empty() ? defaultTerminal.c_str() : terminal.c_str();
        execlp(term, term, "-e", "sh", "-c", command.c_str(), NULL);
        exit(1);
    }
    return pid;
#else
    (void)command;
    (void)canPause;
    (void)windowState;
    (void)continueExecution;
    (void)terminal;
    return -1;
#endif
}

void WindowManager::SetPriority(int priority, pID procID) {
#ifdef _WIN32
    // Windows implementation
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
    if (!hProcess) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return;
    }

    DWORD priorityClass;
    switch (priority) {
        case 0: priorityClass = IDLE_PRIORITY_CLASS; break;
        case 1: priorityClass = BELOW_NORMAL_PRIORITY_CLASS; break;
        case 2: priorityClass = NORMAL_PRIORITY_CLASS; break;
        case 3: priorityClass = ABOVE_NORMAL_PRIORITY_CLASS; break;
        case 4: priorityClass = HIGH_PRIORITY_CLASS; break;
        case 5: priorityClass = REALTIME_PRIORITY_CLASS; break;
        default: priorityClass = (DWORD)priority; break;
    }

    if (!SetPriorityClass(hProcess, priorityClass)) {
        std::cerr << "Failed to set priority class. Error: " << GetLastError() << std::endl;
    }
    CloseHandle(hProcess);
#else
    // Linux implementation
    if (procID == 0) procID = getpid();
    
    int niceValue;
    switch (priority) {
        case 0: niceValue = 19; break;
        case 1: niceValue = 10; break;
        case 2: niceValue = 0; break;
        case 3: niceValue = -10; break;
        case 4: niceValue = -20; break;
        case 5: niceValue = -20; break;
        default: niceValue = 0; break;
    }

    if (setpriority(PRIO_PROCESS, procID, niceValue) != 0) {
        std::cerr << "Failed to set priority. Error: " << errno << std::endl;
    }
#endif
}

// Explicit template instantiations
template int64_t WindowManager::Run<int>(str, int, str, str, int);
template int64_t WindowManager::Run<str>(str, str, str, str, int);
template int64_t WindowManager::Run<ProcessMethod>(str, ProcessMethod, str, str, int);

// Implementation of AHK-like features
void WindowManager::MoveWindow(int direction, int distance) {
    try {
        if(distance < 1 || distance > 1000) {
            throw std::range_error("Invalid move distance");
        }

#ifdef __linux__
        Display* display = H::DisplayManager::GetDisplay();
        if(!display) throw std::runtime_error("No X11 display");

        Window win = GetActiveWindow();
        if(win == 0) return;

        XWindowAttributes attrs;
        if(!XGetWindowAttributes(display, win, &attrs)) {
            throw std::runtime_error("Failed to get window attributes");
        }

        int newX = attrs.x;
        int newY = attrs.y;
        
        switch(direction) {
            case 1: newY -= distance; break; // Up
            case 2: newY += distance; break; // Down
            case 3: newX -= distance; break; // Left
            case 4: newX += distance; break; // Right
        }
        
        XMoveWindow(display, win, newX, newY);
        XFlush(display);
#elif _WIN32
        HWND hwnd = GetForegroundWindow();
        RECT rect;
        GetWindowRect(hwnd, &rect);
        
        switch(direction) {
            case 1: rect.top -= distance; rect.bottom -= distance; break;
            case 2: rect.top += distance; rect.bottom += distance; break;
            case 3: rect.left -= distance; rect.right -= distance; break;
            case 4: rect.left += distance; rect.right += distance; break;
        }
        
        MoveWindow(hwnd, rect.left, rect.top, 
                  rect.right - rect.left, rect.bottom - rect.top, TRUE);
#endif
    } catch(const std::exception& e) {
        std::cerr << "Window move failed: " << e.what() << "\n";
    }
}

void WindowManager::ResizeWindow(int direction, int distance) {
#ifdef __linux__
    Display* display = H::DisplayManager::GetDisplay();
    Window win = GetActiveWindow();
    
    XWindowAttributes attrs;
    XGetWindowAttributes(display, win, &attrs);
    
    int newWidth = attrs.width;
    int newHeight = attrs.height;
    
    switch(direction) {
        case 1: newHeight -= distance; break; // Up
        case 2: newHeight += distance; break; // Down
        case 3: newWidth -= distance; break; // Left
        case 4: newWidth += distance; break; // Right
    }
    
    XResizeWindow(display, win, newWidth, newHeight);
    XFlush(display);
#elif _WIN32
    HWND hwnd = GetForegroundWindow();
    RECT rect;
    GetWindowRect(hwnd, &rect);
    
    switch(direction) {
        case 1: rect.bottom -= distance; break;
        case 2: rect.bottom += distance; break;
        case 3: rect.right -= distance; break;
        case 4: rect.right += distance; break;
    }
    
    MoveWindow(hwnd, rect.left, rect.top, 
              rect.right - rect.left, rect.bottom - rect.top, TRUE);
#endif
}

void WindowManager::SnapWindow(int position) {
    // 1=Left, 2=Right, 3=Top, 4=Bottom, 5=TopLeft, 6=TopRight, 7=BottomLeft, 8=BottomRight
#ifdef __linux__
    auto* display = H::DisplayManager::GetDisplay();
    if(!display) return;
    
    Window root = H::DisplayManager::GetRootWindow();
    Window win = GetActiveWindow();
    
    XWindowAttributes root_attrs;
    if(!XGetWindowAttributes(display, root, &root_attrs)) {
        std::cerr << "Failed to get root window attributes\n";
        return;
    }
    
    XWindowAttributes win_attrs;
    if(!XGetWindowAttributes(display, win, &win_attrs)) {
        std::cerr << "Failed to get window attributes\n";
        return;
    }
    
    int screenWidth = root_attrs.width;
    int screenHeight = root_attrs.height;
    
    int newX = win_attrs.x;
    int newY = win_attrs.y;
    int newWidth = win_attrs.width;
    int newHeight = win_attrs.height;
    
    switch(position) {
        case 1: // Left half
            newWidth = screenWidth/2;
            newHeight = screenHeight;
            newX = 0;
            newY = 0;
            break;
        case 2: // Right half
            newWidth = screenWidth/2;
            newHeight = screenHeight;
            newX = screenWidth/2;
            newY = 0;
            break;
        // Add other positions...
    }
    
    XMoveResizeWindow(display, win, newX, newY, newWidth, newHeight);
    XFlush(display);
#endif
}

void WindowManager::ManageVirtualDesktops(int action) {
#ifdef __linux__
    auto* display = H::DisplayManager::GetDisplay();
    if(!display) {
        std::cerr << "Cannot manage desktops - no X11 display\n";
        return;
    }
    
    Window root = H::DisplayManager::GetRootWindow();
    Atom desktopAtom = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    Atom desktopCountAtom = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
    
    unsigned long nitems, bytes;
    unsigned char *data = NULL;
    int format;
    Atom type;
    
    // Get current desktop
    XGetWindowProperty(display, root, desktopAtom,
                      0, 1, False, XA_CARDINAL, &type, &format, &nitems, &bytes, &data);
    
    int currentDesktop = *(int*)data;
    XFree(data);
    
    // Get total desktops
    XGetWindowProperty(display, root, desktopCountAtom,
                      0, 1, False, XA_CARDINAL, &type, &format, &nitems, &bytes, &data);
    
    int totalDesktops = *(int*)data;
    XFree(data);
    
    int newDesktop = currentDesktop;
    switch(action) {
        case 1: newDesktop = (currentDesktop + 1) % totalDesktops; break;
        case 2: newDesktop = (currentDesktop - 1 + totalDesktops) % totalDesktops; break;
    }
    
    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.message_type = desktopAtom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = newDesktop;
    event.xclient.data.l[1] = CurrentTime;
    
    XSendEvent(display, root, False,
              SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(display);
#endif
}

// Add similar implementations for other AHK functions...

void WindowManager::SnapWindowWithPadding(int position, int padding) {
#ifdef __linux__
    auto* display = H::DisplayManager::GetDisplay();
    Window win = GetActiveWindow();
    
    XWindowAttributes root_attrs;
    XGetWindowAttributes(display, root, &root_attrs);
    
    XWindowAttributes win_attrs;
    XGetWindowAttributes(display, win, &win_attrs);

    const int screenWidth = root_attrs.width - padding*2;
    const int screenHeight = root_attrs.height - padding*2;
    
    switch(position) {
        case 1: // Left with padding
            XMoveResizeWindow(display, win, 
                padding, padding, 
                screenWidth/2, screenHeight);
            break;
        case 2: // Right with padding
            XMoveResizeWindow(display, win, 
                screenWidth/2 + padding, padding, 
                screenWidth/2, screenHeight);
            break;
        // Add other positions...
    }
    XFlush(display);
#endif
}

} // namespace H