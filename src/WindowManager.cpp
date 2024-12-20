#include "WindowManager.hpp"
#include "WindowManager.hpp"

group groups;

#ifdef WINDOWS
str defaultTerminal = "Cmd";
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
Display* display = nullptr; // Global X11 display handle
Window root = 0;
str defaultTerminal = "konsole"; //gnome-terminal
cstr globalShell = "fish";
#endif

WindowManager::WindowManager() {
#ifdef WINDOWS
    pid = GetCurrentProcessId();
#elif defined(__linux__)
    WindowManager::InitializeX11();
#endif
}
bool WindowManager::InitializeX11() {
    if (!display) {
        display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Failed to open X11 display." << std::endl;
            return false;
        }
        return true;
    }
    return true;
}
// List all windows
void WindowManager::All() {
#ifdef WINDOWS
    wID win = GetTopWindow(NULL);
    while (win) {
        char title[256];
        GetWindowTextA(win, title, sizeof(title));
        if (IsWindowVisible(win) && strlen(title) > 0) {
            lo << "ID: " << win << " | Title: " << title;
        }
        win = GetNextWindow(win, GW_HWNDNEXT);
    }
#elif defined(__linux__)
    if (!display) {
        std::cerr << "X11 display not initialized." << std::endl;
        return;
    }
    Window root = DefaultRootWindow(display);
    Window parent;
    Window* children;
    unsigned int nchildren;

    if (XQueryTree(display, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; ++i) {
            char* windowName = nullptr;
            XFetchName(display, children[i], &windowName);
            if (windowName) {
                std::cout << "ID: " << children[i] << " | Title: " << windowName << std::endl;
                XFree(windowName);
            }
        }
        XFree(children);
    }
#endif
}
// Function to add a group
void WindowManager::AddGroup(cstr groupName, cstr identifier) {
    if (groups.find(groupName) == groups.end()) {
        groups[groupName] = std::vector<str>();
        groups[groupName].reserve(4);
    }
    groups[groupName].push_back(identifier);
}
// Helper to parse the identifier string
str WindowManager::GetIdentifierType(cstr identifier)
{
    std::istringstream iss(identifier);
    str type;
    iss >> type; // Get the first word
    return type;
}

// Helper to get the value part of the identifier string
str WindowManager::GetIdentifierValue(cstr identifier)
{
    size_t pos = identifier.find(' ');
    if (pos != str::npos)
    {
        return identifier.substr(pos + 1); // Return everything after the first space
    }
    return ""; // Return empty if no space found
}
// Windows implementation
wID WindowManager::GetActiveWindow() {

    #if defined(_WIN32)
        wID hwnd = GetForegroundWindow();
        return (hwnd);
    #elif defined(__linux__)
        Display* display = XOpenDisplay(nullptr);
        if (!display) {
            return 0;
        }

        wID root = DefaultRootWindow(display);
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
    #endif
}


// Method to find a window based on various identifiers
wID WindowManager::Find(cstr identifier) {
    if (identifier == "A" || identifier.empty()) {
        return GetActiveWindow();
    }
    str type = GetIdentifierType(identifier);
    str value = GetIdentifierValue(identifier);
    #ifdef WINDOWS
        str extension = ".exe";
        if(type == "exe"){
            // Check if the filename ends with .exe
            if (!filename.empty() && 
                (filename.size() < extension.size() || 
                filename.compare(filename.size() - extension.size(), extension.size(), extension) != 0)) {
                value += extension; // Append .exe if not present
            }
        }
    #endif
    if(type == "process" || type == "pname"){
        type = "exe";
    }
    if (type == "class") {
#ifdef WINDOWS
        return FindWindowA(value.c_str(), NULL);
#elif defined(__linux__)
        return FindByClass(value);
#endif
    } else if (type == "pid") {
        pID pid = std::stoul(value);
        return GetwIDByPID(pid);
    } else if (type == "exe") {
        return GetwIDByProcessName(value);
    } else if (type == "id") {
        return reinterpret_cast<wID>(std::stoul(value));
    } else if (type == "group") {
        return FindWindowInGroup(value);
    } else {
        return FindByTitle(value);
    }
    return 0; // Unsupported platform
}
// Example usage:
void WindowManager::AltTab() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    Window root = DefaultRootWindow(display);
    Atom clientListAtom = XInternAtom(display, "_NET_CLIENT_LIST_STACKING", False);
    Atom activeWindowAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

    if (clientListAtom == None || activeWindowAtom == None) {
        std::cerr << "Required atoms not found" << std::endl;
        return;
    }

    // Get the list of windows
    Atom actualType;
    int actualFormat;
    unsigned long numWindows, bytesAfter;
    unsigned char* data = nullptr;

    int status = XGetWindowProperty(display, root, clientListAtom,
                                  0, (~0L), False, XA_WINDOW,
                                  &actualType, &actualFormat,
                                  &numWindows, &bytesAfter, &data);

    if (status != Success || numWindows < 2) {
        if (data) XFree(data);
        return;
    }

    Window* windows = reinterpret_cast<Window*>(data);
    
    // Get current active window
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

    // Find the window to activate (first visible window below current active window)
    Window windowToActivate = None;
    bool foundActive = false;

    // Iterate through windows from top to bottom
    for (int i = numWindows - 1; i >= 0; i--) {
        Window currentWindow = windows[i];
        
        if (currentWindow == activeWindow) {
            foundActive = true;
            continue;
        }

        if (foundActive) {
            // Check if window is visible and not minimized
            XWindowAttributes attrs;
            if (XGetWindowAttributes(display, currentWindow, &attrs)) {
                if (attrs.map_state == IsViewable) {
                    windowToActivate = currentWindow;
                    break;
                }
            }
        }
    }

    // If no window found after active window, take first visible window from top
    if (windowToActivate == None) {
        for (unsigned long i = numWindows - 1; i >= 0; i--) {
            if (windows[i] == activeWindow) continue;
            
            XWindowAttributes attrs;
            if (XGetWindowAttributes(display, windows[i], &attrs)) {
                if (attrs.map_state == IsViewable) {
                    windowToActivate = windows[i];
                    break;
                }
            }
        }
    }

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
}
wID WindowManager::GetwIDByPID(pID pid) {
#ifdef WINDOWS
    wID hwnd = NULL;
    EnumWindows([](wID hwnd, LPARAM lParam) -> BOOL {
        pID windowPID;
        GetWindowThreadProcessId(hwnd, &windowPID);
        if (windowPID == static_cast<pID>(lParam)) {
            *((wID*)lParam) = hwnd;
            return FALSE; // Stop enumeration
        }
        return TRUE; // Continue enumeration
    },
                (LPARAM)&pid);
    return hwnd;
#elif defined(__linux__)
    if (!InitializeX11()) return 0;

    Window root = DefaultRootWindow(display);
    Window parent;
    Window* children = nullptr;
    unsigned int nchildren = 0;

    Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);
    if (pidAtom == None) {
        std::cerr << "X11 does not support _NET_WM_PID." << std::endl;
        return 0;
    }

    if (XQueryTree(display, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; ++i) {
            XWindowAttributes attrs;
            if (XGetWindowAttributes(display, children[i], &attrs) == 0) {
                continue; // Skip invalid windows
            }

            Atom actualType;
            int actualFormat;
            unsigned long nitems, bytesAfter;
            unsigned char* propPID = nullptr;

            if (XGetWindowProperty(display, children[i], pidAtom, 0, 1, False, XA_CARDINAL,
                                   &actualType, &actualFormat, &nitems, &bytesAfter, &propPID) == Success) {
                if (propPID) {
                    pID windowPID = *(reinterpret_cast<pID*>(propPID));
                    XFree(propPID);
                    if (windowPID == pid) {
                        XFree(children);
                        return reinterpret_cast<wID>(children[i]);
                    }
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

wID WindowManager::GetwIDByProcessName(cstr processName) {
#ifdef WINDOWS
    EnumWindowsData data(processName);
    EnumWindows([](wID hwnd, LPARAM lParam) -> BOOL {
        EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);
        DWORD windowPID;
        GetWindowThreadProcessId(hwnd, &windowPID);

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, windowPID);
        if (hProcess) {
            char exeName[MAX_PATH];
            if (GetModuleFileNameExA(hProcess, NULL, exeName, MAX_PATH)) {
                std::string basename = std::string(exeName).substr(std::string(exeName).find_last_of("\\") + 1);
                if (basename == data->targetProcessName) {
                    data->id = hwnd;
                    CloseHandle(hProcess);
                    return FALSE; // Match found
                }
            }
            CloseHandle(hProcess);
        }
        return TRUE; // Continue enumeration
    },
                reinterpret_cast<LPARAM>(&data));
    return data.id;
#elif defined(__linux__)
    if (!InitializeX11()) return 0;

    Window root = DefaultRootWindow(display);
    Window parent;
    Window* children = nullptr;
    unsigned int nchildren = 0;

    Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);
    if (pidAtom == None) {
        std::cerr << "X11 does not support _NET_WM_PID." << std::endl;
        return 0;
    }

    if (XQueryTree(display, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; ++i) {
            XWindowAttributes attrs;
            if (XGetWindowAttributes(display, children[i], &attrs) == 0) {
                continue; // Skip invalid windows
            }

            Atom actualType;
            int actualFormat;
            unsigned long nitems, bytesAfter;
            unsigned char* propPID = nullptr;

            if (XGetWindowProperty(display, children[i], pidAtom, 0, 1, False, XA_CARDINAL,
                                   &actualType, &actualFormat, &nitems, &bytesAfter, &propPID) == Success) {
                if (propPID) {
                    pID windowPID = *(reinterpret_cast<pID*>(propPID));
                    XFree(propPID);

                    std::string procName = getProcessName(windowPID);
                    
                    if (procName == processName) {
                        XFree(children);
                        return reinterpret_cast<wID>(children[i]);
                    }
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

// Find a window by class
wID WindowManager::FindByClass(cstr className) {
#if defined(__linux__)
    if (!InitializeX11()) return 0;

    Window root = DefaultRootWindow(display);
    Window parent;
    Window* children;
    unsigned int nchildren;

    if (XQueryTree(display, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; ++i) {
            XClassHint classHint;
            if (XGetClassHint(display, children[i], &classHint)) {
                if (classHint.res_class && className == classHint.res_class) {
                    XFree(classHint.res_name);
                    XFree(classHint.res_class);
                    return reinterpret_cast<wID>(children[i]);
                }
                if (classHint.res_name) XFree(classHint.res_name);
                if (classHint.res_class) XFree(classHint.res_class);
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
    #ifdef WINDOWS
    return FindWindowA(NULL, title.c_str());
    #else
    if (!display) return 0;

    Window root = DefaultRootWindow(display);
    Window parent;
    Window* children;
    unsigned int nchildren;

    if (XQueryTree(display, root, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; ++i) {
            char* windowName = nullptr;
            XFetchName(display, children[i], &windowName);
            if (windowName && title == windowName) {
                XFree(windowName);
                return reinterpret_cast<wID>(children[i]);
            }
        }
        XFree(children);
    }
    #endif
    return 0;
}
std::string WindowManager::getProcessName(pid_t windowPID) {
    std::ostringstream procPath;
    procPath << "/proc/" << windowPID << "/comm";
    std::string path = procPath.str();
    
    // Open the file using fopen
    FILE* procFile = fopen(path.c_str(), "r");
    std::cout << "Attempting to open: " << path << "\n";
    
    if (!procFile) {
        std::cerr << "Error: Could not open file " << path << ": " << std::strerror(errno) << "\n";
        return ""; // Handle the error as needed
    }

    char procName[256]; // Buffer to hold the process name
    if (fgets(procName, sizeof(procName), procFile) != nullptr) {
        fclose(procFile); // Close the file

        // Trim newline character, if present
        size_t len = std::strlen(procName);
        if (len > 0 && procName[len - 1] == '\n') {
            procName[len - 1] = '\0'; // Replace newline with null terminator
        }

        return std::string(procName); // Return the process name as a string
    } else {
        std::cerr << "Error: Could not read from file " << path << ": " << std::strerror(errno) << "\n";
        fclose(procFile); // Close the file
        return ""; // Handle the error as needed
    }
}
// Helper to find a window in a specified group
wID WindowManager::FindWindowInGroup(cstr groupName) {
    auto it = groups.find(groupName);
    if (it != groups.end()) {
        for (cstr window : it->second) {
            wID foundWindow = Find(window.c_str());
            if (foundWindow) {
                return foundWindow;
            }
        }
    }
    return 0;
}

// Create a new window
wID WindowManager::NewWindow(cstr name, std::vector<int>* dimensions, bool hide) {
#ifdef WINDOWS
    std::vector<int> dim = dimensions ? *dimensions : std::vector<int>{0, 0, 800, 600};
    if (dim.size() != 4) {
        std::cerr << "Dimensions vector must contain exactly 4 elements: x, y, width, height." << std::endl;
        return nullptr;
    }

    WNDCLASS wc = {0};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = TEXT(name.c_str());

    if (!RegisterClass(&wc)) {
        std::cerr << "Window registration failed!" << std::endl;
        return nullptr;
    }

    wID hwnd = CreateWindowEx(0, wc.lpszClassName, name.c_str(),
                              WS_OVERLAPPEDWINDOW, dim[0], dim[1], dim[2], dim[3],
                              HWND_MESSAGE, nullptr, wc.hInstance, nullptr);

    if (hide) {
        ShowWindow(hwnd, SW_HIDE);
    }

    return hwnd;
#elif defined(__linux__)
    if (!display) {
        std::cerr << "X11 display not initialized." << std::endl;
        return 0;
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

ProcessMethod WindowManager::toMethod(cstr method)
{
    static const std::unordered_map<str, ProcessMethod> methodMap = {
        {"WaitForTerminate", ProcessMethod::WaitForTerminate},
        {"ForkProcess", ProcessMethod::ForkProcess},
        {"ContinueExecution", ProcessMethod::ContinueExecution},
        {"WaitUntilStarts", ProcessMethod::WaitUntilStarts},
        {"CreateNewWindow", ProcessMethod::CreateNewWindow},
        {"AsyncProcessCreate", ProcessMethod::AsyncProcessCreate},
        {"SystemCall", ProcessMethod::SystemCall}};

    auto it = methodMap.find(method);
    return (it != methodMap.end()) ? it->second : ProcessMethod::Invalid;
}
#ifdef WINDOWS
// Function to convert error code to a human-readable message
str WindowManager::GetErrorMessage(pID errorCode)
{
    LPWSTR lpMsgBuf;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&lpMsgBuf,
        0, nullptr);

    // Convert wide string to narrow string
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, lpMsgBuf, -1, nullptr, 0, nullptr, nullptr);
    std::string errorMessage(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, lpMsgBuf, -1, &errorMessage[0], sizeNeeded, nullptr, nullptr);

    LocalFree(lpMsgBuf);
    return errorMessage;
}

// Function to create a process and handle common logic
bool WindowManager::CreateProcessWrapper(cstr path, cstr command, pID creationFlags, STARTUPINFO &si, PROCESS_INFORMATION &pi)
{
    if (path.empty())
    {
        return CreateProcess(
            NULL,
            const_cast<char *>(command.c_str()),
            nullptr, nullptr, FALSE,
            creationFlags, nullptr, nullptr,
            &si, &pi);
    }
    else
    {
        return CreateProcess(
            path.c_str(),
            const_cast<char *>(command.c_str()),
            nullptr, nullptr, FALSE,
            creationFlags, nullptr, nullptr,
            &si, &pi);
    }
}
#endif
template <typename T>
int64_t WindowManager::Run(str path, T method, str windowState, str command, int priority)
{
    ProcessMethod processMethod;
    windowState = ToLower(windowState);
    // Convert method to ProcessMethod if it's a string
    if constexpr (std::is_same_v<T, str>)
    {
        processMethod = toMethod(method);
    }
    else if constexpr (std::is_same_v<T, int>)
    {
        processMethod = static_cast<ProcessMethod>(method);
    }
    else if constexpr (std::is_same_v<T, ProcessMethod>)
    {
        processMethod = method;
    }
    else
    {
        processMethod = ProcessMethod::Invalid;
    }

#ifdef WINDOWS
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    pID creationFlags = windowState == "call" ? CREATE_NEW_PROCESS_GROUP : CREATE_NEW_CONSOLE; // Start with the new process group flag
    // Add the no window flag if you want to suppress the command window
    if (windowState == "hide")
    {
        creationFlags |= CREATE_NO_WINDOW; // Suppress the console window
    }
    if (!command.empty() && !path.empty() && processMethod != ProcessMethod::Shell)
    {
        command = "\"" + path + "\" " + command;
        path = "";
        lo << "Path " << path << "\nCommand " << command;
    }
    // Set window state based on the `windowState` parameter
    if (!windowState.empty())
    {
        si.dwFlags |= STARTF_USESHOWWINDOW; // Enable `wShowWindow` setting

        if (windowState == "max")
        {
            si.wShowWindow = SW_SHOWMAXIMIZED;
        }
        else if (windowState == "min")
        {
            si.wShowWindow = SW_SHOWMINIMIZED;
        }
        else if (windowState == "hide")
        {
            si.wShowWindow = SW_HIDE;
        }
        else if (windowState == "unfocused")
        {
            si.wShowWindow = SW_SHOWNOACTIVATE;
        }
        else if (windowState == "focused")
        {
            si.wShowWindow = SW_SHOW;
        }
        else
        {
            si.wShowWindow = SW_SHOWNORMAL; // Default
        }
    }

    int processFlags = 0;
    if (priority != -1)
    {
        switch (priority)
        {
        case 0:
            processFlags = IDLE_PRIORITY_CLASS;
            break;
        case 1:
            processFlags = BELOW_NORMAL_PRIORITY_CLASS;
            break;
        case 2:
            processFlags = NORMAL_PRIORITY_CLASS;
            break;
        case 3:
            processFlags = ABOVE_NORMAL_PRIORITY_CLASS;
            break;
        case 4:
            processFlags = HIGH_PRIORITY_CLASS;
            break;
        case 5:
            processFlags = REALTIME_PRIORITY_CLASS;
            break;
        case 6:
            processFlags = 0x00000200;
            break;
        default:
            processFlags = NORMAL_PRIORITY_CLASS;
            break;
        }
        creationFlags |= processFlags;
    }
    switch (processMethod)
    {
    case ProcessMethod::AsyncProcessCreate:
        std::thread([path]()
                    { system(path.c_str()); })
            .detach();
        return 0; // Return immediately for async
        break;

    case ProcessMethod::SystemCall:
        return system(path.c_str()); // Synchronous call, returns exit code
    case ProcessMethod::ForkProcess:
    {
        int forkedProcessID = _spawnl(_P_WAIT, path.c_str(), path.c_str(), NULL);
        if (forkedProcessID == -1)
        {
            std::cerr << "Failed to fork process." << std::endl;
        }
        else
        {
            std::cout << "Forked process ID: " << forkedProcessID << std::endl;
            return forkedProcessID;
        }
        break;
    }
    case ProcessMethod::CreateNewWindow:
    {
        creationFlags &= ~CREATE_NO_WINDOW;
        creationFlags &= ~CREATE_NEW_PROCESS_GROUP;
        creationFlags |= CREATE_NEW_CONSOLE;
        processMethod = ProcessMethod::ContinueExecution;
        break;
    }
    case ProcessMethod::SameWindow:
    {
        creationFlags &= ~CREATE_NEW_CONSOLE;
        creationFlags &= ~CREATE_NEW_PROCESS_GROUP;
        processMethod = ProcessMethod::WaitForTerminate;
        break;
    }
    case ProcessMethod::Shell:
    {
        // Use ShellExecute to open the Steam URL
        HINSTANCE result = ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, si.wShowWindow);
        // Check if the operation was successful
        if ((intptr_t)result <= 32)
        {
            // Handle the error
            std::cerr << "ShellExecuteA failed with error code: " << GetLastError() << std::endl;
            return -1; // Indicate failure, adjust as necessary for your application
        }

        // Successfully opened, return the result as int64_t
        return reinterpret_cast<int64_t>(result); // Safe cast to int64_t
    }
    case ProcessMethod::Invalid:
        std::cerr << "Invalid process method." << std::endl;
        break;

    case ProcessMethod::WaitUntilStarts:
        break;
    case ProcessMethod::WaitForTerminate:
        break;
    case ProcessMethod::ContinueExecution:
        break;
    }
    // Attempt to create the process once
    if (!CreateProcessWrapper(path, command, creationFlags, si, pi))
    {
        pID error = GetLastError();
        std::cerr << "Failed to create process. Error: " << error << " - " << GetErrorMessage(error) << std::endl;
        return -1; // Return error code
    }
    if (priority != -1)
    {
        WindowManager *wm = new WindowManager();
        wm->SetPriority((int)processFlags, GetProcessId(pi.hProcess));
        delete wm;
    }
    switch (processMethod)
    {
    case ProcessMethod::WaitForTerminate:
        WaitForSingleObject(pi.hProcess, INFINITE);
        pID exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exitCode;
    case ProcessMethod::ContinueExecution:
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return pi.dwProcessId; // Return process ID for asynchronous
    case ProcessMethod::WaitUntilStarts:
        WaitForInputIdle(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return pi.dwProcessId; // Return process ID after process starts
    default:
        std::cerr << "Invalid process method." << std::endl;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return -1;
    }

#else // Linux/Unix implementation
    if(!command.empty()){
        path += " " + command;
    }
    lo << path;
    pid_t pid;
    int status;

    // Determine priority class based on priority input
    int priorityClass = 0; // Default priority
    if (priority != -1)
    {
        switch (priority)
        {
        case 1:
            priorityClass = -10;
            break; // High priority
        case 2:
            priorityClass = -20;
            break; // Real-time priority
        case 3:
            priorityClass = 10;
            break; // Below normal
        case 4:
            priorityClass = 19;
            break; // Idle
        default:
            priorityClass = 0;
            break; // Normal
        }
    }

    switch (processMethod)
    {
    case ProcessMethod::WaitForTerminate:
    {
        pid = fork();
        if (pid == 0)
        {
            // Child process
            if (setpriority(PRIO_PROCESS, 0, priorityClass) < 0)
            {
                perror("setpriority failed");
            }
            execl(path.c_str(), path.c_str(), command.c_str(), (char *)NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            // Parent process waits for the child to terminate
            if (waitpid(pid, &status, 0) < 0)
            {
                perror("waitpid failed");
            }
            if (WIFEXITED(status))
            {
                return WEXITSTATUS(status); // Return child's exit code
            }
        }
        else
        {
            perror("fork failed");
        }
        break;
    }

    case ProcessMethod::ForkProcess:
    case ProcessMethod::ContinueExecution:
    {
        pid = fork();
        if (pid == 0)
        {
            // Child process
            if (setpriority(PRIO_PROCESS, 0, priorityClass) < 0)
            {
                perror("setpriority failed");
            }
            execl(path.c_str(), path.c_str(), command.c_str(), (char *)NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            return pid; // Return PID for asynchronous behavior
        }
        else
        {
            perror("fork failed");
        }
        break;
    }

    case ProcessMethod::WaitUntilStarts:
    {
        pid = fork();
        if (pid == 0)
        {
            // Child process
            if (setpriority(PRIO_PROCESS, 0, priorityClass) < 0)
            {
                perror("setpriority failed");
            }
            execl(path.c_str(), path.c_str(), command.c_str(), (char *)NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            usleep(100000); // Sleep for 100ms to allow the process to start
            return pid;     // Return PID
        }
        else
        {
            perror("fork failed");
        }
        break;
    }

    case ProcessMethod::AsyncProcessCreate:
    {
        try
        {
            std::thread([path]()
                        {
                    if (system(path.c_str()) == -1) {
                        perror("system call failed");
                    } })
                .detach();
            return 0; // Return immediately for async process
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error creating async thread: " << e.what() << std::endl;
        }
        break;
    }

    case ProcessMethod::Shell:
    case ProcessMethod::SystemCall:
    {
        status = system(path.c_str());
        if (status == -1)
        {
            perror("system call failed");
        }
        return WEXITSTATUS(status); // Return system call exit code
    }

    case ProcessMethod::Invalid:
    default:
    {
        std::cerr << "Invalid process method specified." << std::endl;
        break;
    }
    }

#endif

    return -1; // Return -1 if something fails
}
// Terminal function to open a terminal in a new window
int64_t WindowManager::Terminal(cstr command, bool canPause, str windowState, bool continueExecution, cstr terminal)
{
    str fullCommand;

#ifdef WINDOWS
    // Prepare the command based on the window state
    if (ToLower(terminal) == "powershell")
    {
        fullCommand = continueExecution ? "-NoExit " : ""; // Keep PowerShell open if continueExecution is true
        fullCommand += command;                            // Add the command to execute
        // Launch PowerShell
        return Run("C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", 0, windowState, fullCommand, -1);
    }
    else
    {
        // Prepare the command for cmd.exe
        fullCommand = continueExecution ? "/k" : "/c";
        fullCommand += " \"" + command + "\"";
        if (canPause)
        {
            fullCommand += " && pause"; // Add pause if required
        }
        // Use cmd.exe for Windows
        return Run("C:\\Windows\\System32\\cmd.exe", 0, windowState, fullCommand, -1);
    }
#else
    // Prepare the command for Linux
    fullCommand = command;
    if (canPause)
    {
        fullCommand += "; read"; // Pause on Linux
    }
    ProcessMethod method = ProcessMethod::Shell;

    if (ToLower(terminal) == "gnome-terminal") {
        fullCommand = continueExecution ? "-e '" + fullCommand + "' --wait" : "-e '" + fullCommand + "'";
        return Run("gnome-terminal", method, windowState, fullCommand, -1);
    } else if (ToLower(terminal) == "konsole") {
        fullCommand = continueExecution ? "-e " + globalShell + " -c '" + fullCommand + "; exec " + globalShell + "'" : "-e " + globalShell + " -c '" + fullCommand + "'";
        return Run("konsole", method, windowState, fullCommand, -1);
    } else if (ToLower(terminal) == "xfce4-terminal") {
        fullCommand = continueExecution ? "-e " + globalShell + " -c '" + fullCommand + "; exec " + globalShell + "'" : "-e " + globalShell + " -c '" + fullCommand + "'";
        return Run("xfce4-terminal", method, windowState, fullCommand, -1);
    } else if (ToLower(terminal) == "xterm") {
        fullCommand = continueExecution ? "-e " + globalShell + " -c '" + fullCommand + "; exec " + globalShell + "'" : "-e " + globalShell + " -c '" + fullCommand + "'";
        return Run("xterm", method, windowState, fullCommand, -1);
    } else if (ToLower(terminal) == "lxterminal") {
        fullCommand = continueExecution ? "-e " + globalShell + " -c '" + fullCommand + "; exec " + globalShell + "'" : "-e " + globalShell + " -c '" + fullCommand + "'";
        return Run("lxterminal", method, windowState, fullCommand, -1);
    } else if (ToLower(terminal) == "tmux") {
        fullCommand = continueExecution ? "new-session -d '" + fullCommand + "'; attach" : "new-session -d '" + fullCommand + "'; attach";
        return Run("tmux", method, windowState, fullCommand, -1);
    } else {
        // Handle default case or error
        return -1; // or another error handling
    }
#endif
}
void WindowManager::SetPriority(int priority, pID procID)
{
    // If procID is NULL, use the current process
    if (procID == 0)
    {
        procID = pid;
    }

#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION, FALSE, procID);
    if (hProcess == NULL)
    {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return;
    }

    // Map the integer priority to appropriate Windows constants
    DWORD priorityClass;
    switch (priority)
    {
    case 0:
        priorityClass = IDLE_PRIORITY_CLASS;
        break;
    case 1:
        priorityClass = BELOW_NORMAL_PRIORITY_CLASS;
        break;
    case 2:
        priorityClass = NORMAL_PRIORITY_CLASS;
        break;
    case 3:
        priorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
        break;
    case 4:
        priorityClass = HIGH_PRIORITY_CLASS;
        break;
    case 5:
        priorityClass = REALTIME_PRIORITY_CLASS;
        break;
    default:
        priorityClass = (DWORD)priority;
        break;
    }

    if (priority < 0)
    {
        priorityClass = static_cast<DWORD>(std::abs(priority));
    }

    // Set the priority class
    if (!SetPriorityClass(hProcess, priorityClass))
    {
        std::cerr << "Failed to set priority class. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return;
    }

    // Optionally, set thread priority based on the same integer
    HANDLE hThread = GetCurrentThread();
    int threadPriority;
    switch (priority)
    {
    case 0:
        threadPriority = THREAD_PRIORITY_IDLE;
        break;
    case 1:
        threadPriority = THREAD_PRIORITY_LOWEST;
        break;
    case 2:
        threadPriority = THREAD_PRIORITY_BELOW_NORMAL;
        break;
    case 3:
        threadPriority = THREAD_PRIORITY_NORMAL;
        break;
    case 4:
        threadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
        break;
    case 5:
        threadPriority = THREAD_PRIORITY_HIGHEST;
        break;
    default:
        threadPriority = THREAD_PRIORITY_NORMAL; // Default if invalid
        break;
    }

    if (priority < 0)
    {
        threadPriority = std::abs(priority);
    }

    if (!SetThreadPriority(hThread, threadPriority))
    {
        std::cerr << "Failed to set thread priority. Error: " << GetLastError() << std::endl;
    }
    else
    {
        std::cout << "Priority set successfully." << std::endl;
    }

    // Clean up
    CloseHandle(hProcess);
#else
    // Linux implementation
    // Note: Linux does not have a direct equivalent for priority classes
    int niceValue;

    switch (priority)
    {
    case 0:
        niceValue = 19; // Lowest priority (nice value)
        break;
    case 1:
        niceValue = 10; // Below normal priority
        break;
    case 2:
        niceValue = 0;  // Normal priority
        break;
    case 3:
        niceValue = -10; // Above normal priority
        break;
    case 4:
        niceValue = -20; // High priority
        break;
    case 5:
        niceValue = -20; // Realtime priority (not directly supported by nice)
        break;
    default:
        niceValue = 0; // Default to normal priority
        break;
    }

    if (setpriority(PRIO_PROCESS, procID, niceValue) != 0)
    {
        std::cerr << "Failed to set priority. Error: " << errno << std::endl;
    }
    else
    {
        std::cout << "Priority set successfully." << std::endl;
    }
#endif
}
// Explicit instantiation for the Run method with int
template int64_t WindowManager::Run<int>(str, int, str, str, int);
template int64_t WindowManager::Run<str>(str, str, str, str, int);
template int64_t WindowManager::Run<ProcessMethod>(str, ProcessMethod, str, str, int);