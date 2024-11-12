#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <map>
#include <vector>
#include <cstdlib> // For getenv()
#include "Printer.h"

// Platform detection and macro definitions
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <process.h>
    #include <psapi.h>
    #define WINDOWS 1
    #define OS_NAME "Windows"
    using wID = HWND;
    using pID = DWORD;  
    // #define DESKTOP_ENVIRONMENT "Unknown" // Placeholder for Windows
    // #define WINDOW_MANAGER "Unknown" // Placeholder for Windows
#elif defined(__linux__)
    #include <unistd.h>
    #include <pwd.h>
    #define LINUX 2
    #define OS_NAME "Linux"
    // Macros to retrieve desktop environment and window manager
    #define DESKTOP_ENVIRONMENT (getenv("XDG_CURRENT_DESKTOP") ? getenv("XDG_CURRENT_DESKTOP") : "Unknown")
    #define WINDOW_MANAGER (getenv("WM_NAME") ? getenv("WM_NAME") : "Unknown") // WM_NAME is not standard; adjust as needed
    using wID = void*; // Use void* or an appropriate type for Linux
    using pID = pid_t; // Example type for process ID
#elif defined(__APPLE__)
    #include <unistd.h>
    #define MAC 3
    #define OS_NAME "macOS"
    #define DESKTOP_ENVIRONMENT "Aqua" // Placeholder for macOS
    #define WINDOW_MANAGER "Unknown" // Placeholder for macOS
    using wID = void*; // Use void* or an appropriate type for Linux
    using pID = pid_t; // Example type for process ID
#else
    #error "Unsupported platform"
#endif

using str = std::string; // Alias for string type
using cstr = const str&; // Alias for const string reference
using group = std::map<str, std::vector<str>>; // Alias for a map of string to vector of strings
using null = decltype(NULL); // Alias for null pointer type

#endif // TYPES_H