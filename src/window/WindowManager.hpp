#pragma once
#include "types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <type_traits>
#include "WindowManagerDetector.hpp"

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace H {

class WindowManager {
public:
    WindowManager();
    ~WindowManager() = default;

    // Static window methods
    static wID GetActiveWindow();
    static wID GetwIDByPID(pID pid);
    static wID GetwIDByProcessName(cstr processName);
    static wID FindByClass(cstr className); 
    static wID FindByTitle(cstr title);
    static wID Find(cstr identifier);
    static wID FindWindowInGroup(cstr groupName);
    static wID NewWindow(cstr name, std::vector<int>* dimensions = nullptr, bool hide = false);

    // Process management
    static ProcessMethod toMethod(cstr method);
    static void SetPriority(int priority, pID procID = 0);
    static int64_t Terminal(cstr command, bool canPause, str windowState, bool continueExecution, cstr terminal = "");
    
    template<typename T>
    static int64_t Run(str path, T method, str windowState, str command, int priority);

    // Window manager info
    std::string GetCurrentWMName() const;
    bool IsWMSupported() const;
    bool IsX11() const;
    bool IsWayland() const;
    void All();

    // Group management
    static void AddGroup(cstr groupName, cstr identifier);

    // Window switching
    static void AltTab();

    // Helper methods
    static str GetIdentifierType(cstr identifier);
    static str GetIdentifierValue(cstr identifier);
    static str getProcessName(pid_t windowPID);

private:
    static bool InitializeX11();
    std::string DetectWindowManager() const;
    bool CheckWMProtocols() const;

    // Private members
    std::string wmName;
    bool wmSupported{false};
    WindowManagerDetector::WMType wmType{};  // Default initialization
};

// Template definition
template<typename T>
int64_t WindowManager::Run(str path, T method, str windowState, str command, int priority) {
#ifdef __linux__
    (void)windowState; // Suppress unused parameter warning
    (void)priority;    // Suppress unused parameter warning

    ProcessMethod processMethod;
    if constexpr (std::is_same_v<T, ProcessMethod>) {
        processMethod = method;
    } else if constexpr (std::is_same_v<T, int>) {
        processMethod = static_cast<ProcessMethod>(method);
    } else if constexpr (std::is_same_v<T, str>) {
        processMethod = toMethod(method);
    } else {
        return -1;
    }

    // Implementation of process creation logic
    switch (processMethod) {
        case ProcessMethod::WaitForTerminate: {
            pid_t childPid = ::fork();
            if (childPid == 0) {
                // Child process
                ::execl(path.c_str(), path.c_str(), command.c_str(), nullptr);
                ::exit(1);
            } else if (childPid > 0) {
                // Parent process
                int status;
                ::waitpid(childPid, &status, 0);
                return static_cast<int64_t>(childPid);
            }
            return -1;
        }
        
        case ProcessMethod::ForkProcess: {
            pid_t childPid = ::fork();
            if (childPid == 0) {
                // Child process
                ::execl(path.c_str(), path.c_str(), command.c_str(), nullptr);
                ::exit(1);
            } else if (childPid > 0) {
                // Parent process
                return static_cast<int64_t>(childPid);
            }
            return -1;
        }
        
        // Add other cases as needed...
        
        default:
            return -1;
    }
#else
    (void)path;       // Suppress unused parameter warnings
    (void)method;
    (void)windowState;
    (void)command;
    (void)priority;
    return -1;
#endif
}

} // namespace H