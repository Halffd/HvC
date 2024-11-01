#include "WindowManager.hpp"

group groups;
// Method to find a window based on various identifiers
HWND WindowManager::Find(cstr identifier) {
    str type = GetIdentifierType(identifier);
    str value = GetIdentifierValue(identifier);

    if (type == "class") {
        return FindWindowA(value.c_str(), NULL);
    } else if (type == "pid") {
        DWORD pid = std::stoul(value);
        return GetHWNDByPID(pid);
    } else if (type == "exe") {
        return GetHWNDByProcessName(value);
    } else if (type == "id") {
        return reinterpret_cast<HWND>(std::stoul(value));
    } else if (type == "group") {
        return FindWindowInGroup(value);
    } else {
        std::cerr << "Invalid identifier type!" << std::endl;
        return NULL;
    }
}

// List all windows
void WindowManager::All() {
    HWND hwnd = GetTopWindow(NULL);
    while (hwnd) {
        char title[256];
        GetWindowTextA(hwnd, title, sizeof(title));
        if (IsWindowVisible(hwnd) && strlen(title) > 0) {
            lo << "ID: " << hwnd << " | Title: " << title;
        }
        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }
}

// Function to add a group
void WindowManager::AddGroup(cstr groupName, cstr identifier) {
    groups[groupName] = identifier;
}

// Helper to get HWND by PID
HWND WindowManager::GetHWNDByPID(DWORD pid) {
    HWND hwnd = NULL;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        DWORD windowPID;
        GetWindowThreadProcessId(hwnd, &windowPID);
        if (windowPID == lParam) {
            *((HWND*)lParam) = hwnd;
            return FALSE; // Stop enumeration
        }
        return TRUE; // Continue enumeration
    }, (LPARAM)&pid);
    return hwnd;
}

// Helper to get HWND by process name
// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);
    DWORD windowPID;
    GetWindowThreadProcessId(hwnd, &windowPID);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, windowPID);
    
    if (hProcess) {
        char name[MAX_PATH];
        if (GetModuleFileNameExA(hProcess, NULL, name, sizeof(name))) {
            std::string fullName(name);
            std::string processBaseName = fullName.substr(fullName.find_last_of("\\") + 1);

            if (processBaseName == data->targetProcessName) {
                data->hwnd = hwnd; // Set the found HWND
                CloseHandle(hProcess);
                return FALSE; // Stop enumeration
            }
        }
        CloseHandle(hProcess);
    }
    return TRUE; // Continue enumeration
}

// Method to get HWND by process name
HWND WindowManager::GetHWNDByProcessName(cstr processName) {
    EnumWindowsData data(processName); // Create data structure
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data)); // Pass the data structure
    return data.hwnd; // Return the found HWND or NULL
}
// Helper to find a window in a specified group
HWND WindowManager::FindWindowInGroup(cstr groupName) {
    auto it = groups.find(groupName);
    if (it != groups.end()) {
        return Find(it->second); // Use the identifier from the group
    } else {
        std::cerr << "Group not found!" << std::endl;
        return NULL;
    }
}

// Helper to parse the identifier string
str WindowManager::GetIdentifierType(cstr identifier) {
    std::istringstream iss(identifier);
    str type;
    iss >> type; // Get the first word
    return type;
}

// Helper to get the value part of the identifier string
str WindowManager::GetIdentifierValue(cstr identifier) {
    size_t pos = identifier.find(' ');
    if (pos != str::npos) {
        return identifier.substr(pos + 1); // Return everything after the first space
    }
    return ""; // Return empty if no space found
}