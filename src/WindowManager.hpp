#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <iostream>
#include <windows.h>
#include <sstream>
#include <vector>
#include <psapi.h>
#include "Logger.h"
#include "Printer.h"
#include "types.hpp"

extern group groups; // Map to store groups
// Struct to hold the window handle and the target process name
struct EnumWindowsData {
    HWND hwnd;
    std::string targetProcessName;

    EnumWindowsData(const std::string& processName) 
        : hwnd(NULL), targetProcessName(processName) {}
};

// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
class WindowManager {
public:
    // Method to find a window based on various identifiers
    static HWND Find(cstr identifier);

    // List all windows
    static void All();

    // Function to add a group
    static void AddGroup(cstr groupName, cstr identifier);

protected:
    // Helper to get HWND by PID
    static HWND GetHWNDByPID(DWORD pid);

    // Helper to get HWND by process name
    static HWND GetHWNDByProcessName(cstr processName);

    // Helper to find a window in a specified group
    static HWND FindWindowInGroup(cstr groupName);

    // Helper to parse the identifier string
    static str GetIdentifierType(cstr identifier);

    // Helper to get the value part of the identifier string
    static str GetIdentifierValue(cstr identifier);
};

#endif // WINDOW_MANAGER_HPP