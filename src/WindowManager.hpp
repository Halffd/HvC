#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <cstddef>
#include <iostream>
#include <windows.h>
#include <sstream>
#include <vector>
#include <thread>
#include <limits>
#include <cmath>
#include <unordered_map>
#include <variant>
#include "types.hpp"
#include "Util.hpp"
#include "Logger.h"
#include "Printer.h"
#ifndef WINDOWS
    #include <sys/wait.h>
    #include <sys/resource.h>
    #include <csignal>
#endif

extern group groups; // Map to store groups
extern str defaultTerminal;
// Struct to hold the window handle and the target process name
struct EnumWindowsData {
    wID id;
    std::string targetProcessName;

    EnumWindowsData(const std::string& processName) 
        : id(NULL), targetProcessName(processName) {}
};
enum class ProcessMethod {
    ContinueExecution,
    WaitForTerminate,
    WaitUntilStarts,
    SystemCall,
    AsyncProcessCreate,
    ForkProcess,
    CreateNewWindow,
    SameWindow,
    Shell,
    Invalid // Added for error handling
};

// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsProc(wID win, LPARAM lParam);
class WindowManager {
public:
    pID pid = -1;

    WindowManager();

    // Method to find a window based on various identifiers
    static wID Find(cstr identifier);

    // List all windows
    static void All();

    // Function to add a group
    static void AddGroup(cstr groupName, cstr identifier);
    
    static wID NewWindow(cstr name, std::vector<int>* dimensions = nullptr, bool hide = false);

    // Template method to run a process
    template <typename T>
    static int64_t Run(str path, T method, str windowState = "", str command = "", int priority = -1);

    static int64_t Terminal(cstr command, bool canPause = false, str windowState = "Hide", bool continueExecution = false, cstr terminal = defaultTerminal);

    void SetPriority(int priority = 0, pID procID = 0);
protected:
    // Function to get the ID of another process
    static pID GetProcessID(HANDLE otherProcessHandle);
    
    // Helper to get wID by PID
    static wID GetwIDByPID(pID pid);

    // Helper to get wID by process name
    static wID GetwIDByProcessName(cstr processName);

    // Helper to find a window in a specified group
    static wID FindWindowInGroup(cstr groupName);

    // Helper to parse the identifier string
    static str GetIdentifierType(cstr identifier);

    // Helper to get the value part of the identifier string
    static str GetIdentifierValue(cstr identifier);

    
    // Function to convert error code to a human-readable message
    static str GetErrorMessage(DWORD errorCode);
private:
    // Helper function to convert string to ProcessMethod
    static ProcessMethod toMethod(const std::string& method);

    // Function to create a process and handle common logic
    static bool CreateProcessWrapper(cstr path, cstr command, DWORD creationFlags, STARTUPINFO& si, PROCESS_INFORMATION& pi);
};
#endif // WINDOW_MANAGER_HPP