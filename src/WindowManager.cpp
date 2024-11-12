#include "WindowManager.hpp"

group groups;

#ifdef WINDOWS
str defaultTerminal = "Cmd";
#else
str defaultTerminal = "gnome-terminal";
#endif

WindowManager::WindowManager(){
    pid = GetCurrentProcessId();
}

// Method to find a window based on various identifiers
wID WindowManager::Find(cstr identifier) {
    str type = GetIdentifierType(identifier);
    str value = GetIdentifierValue(identifier);

    if (type == "class") {
        return FindWindowA(value.c_str(), NULL);
    } else if (type == "pid") {
        DWORD pid = std::stoul(value);
        return GetwIDByPID(pid);
    } else if (type == "exe") {
        return GetwIDByProcessName(value);
    } else if (type == "id") {
        return reinterpret_cast<wID>(std::stoul(value));
    } else if (type == "group") {
        return FindWindowInGroup(value);
    } else {
        std::cerr << "Invalid identifier type!" << std::endl;
        return NULL;
    }
}

// List all windows
void WindowManager::All() {
    wID win = GetTopWindow(NULL);
    while (win) {
        char title[256];
        GetWindowTextA(win, title, sizeof(title));
        if (IsWindowVisible(win) && strlen(title) > 0) {
            lo << "ID: " << win << " | Title: " << title;
        }
        win = GetNextWindow(win, GW_HWNDNEXT);
    }
}

// Function to add a group
void WindowManager::AddGroup(cstr groupName, cstr identifier) {
    // If the group does not exist, create a new vector
    if (groups.find(groupName) == groups.end()) {
        groups[groupName] = std::vector<str>(); // Create a new vector
        groups[groupName].reserve(4);  // Preallocate space
    }
    // Add the identifier to the group's vector
    groups[groupName].push_back(identifier);
}

// Helper to get wID by PID
wID WindowManager::GetwIDByPID(DWORD pid) {
    wID win = NULL;
    EnumWindows([](wID win, LPARAM lParam) -> BOOL {
        DWORD windowPID;
        GetWindowThreadProcessId(win, &windowPID);
        if (windowPID == lParam) {
            *((wID*)lParam) = win;
            return FALSE; // Stop enumeration
        }
        return TRUE; // Continue enumeration
    }, (LPARAM)&pid);
    return win;
}

// Helper to get wID by process name
// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsProc(wID win, LPARAM lParam) {
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);
    DWORD windowPID;
    GetWindowThreadProcessId(win, &windowPID);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, windowPID);
    
    if (hProcess) {
        char name[MAX_PATH];
        if (GetModuleFileNameExA(hProcess, NULL, name, sizeof(name))) {
            str fullName(name);
            str processBaseName = fullName.substr(fullName.find_last_of("\\") + 1);

            if (processBaseName == data->targetProcessName) {
                data->id = win; // Set the found wID
                CloseHandle(hProcess);
                return FALSE; // Stop enumeration
            }
        }
        CloseHandle(hProcess);
    }
    return TRUE; // Continue enumeration
}

// Method to get wID by process name
wID WindowManager::GetwIDByProcessName(cstr processName) {
    EnumWindowsData data(processName); // Create data structure
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data)); // Pass the data structure
    return data.id; // Return the found wID or NULL
}
// Helper to find a window in a specified group
wID WindowManager::FindWindowInGroup(cstr groupName) {
    // Check if the group exists in the map
    auto it = groups.find(groupName);
    if (it != groups.end()) {
        // Iterate over the vector of windows in the group
        for (cstr window : it->second) {
            // Attempt to find the window
            wID foundWindow = Find(window.c_str());
            if (foundWindow) {
                return foundWindow; // Return the found window
            }
        }
    }
    return NULL;
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

// Function to get the current process ID and the ID of another process
pID WindowManager::GetProcessID(HANDLE otherProcessHandle) {   
    // Get the process ID of the specified process handle
    pID processID = GetProcessId(otherProcessHandle);

    // Output the process IDs
    std::cout << "Current Process ID: " << processID << std::endl;
    if (processID > 0) {
        return processID;
    } else {
        std::cout << "Invalid process handle." << std::endl;
        return -1;
    }
}
// Create a dummy window for hotkey listening
wID WindowManager::NewWindow(cstr name, std::vector<int>* dimensions, bool hide) {
    // If dimensions is nullptr, initialize it with four zeros
    std::vector<int> dim = dimensions ? *dimensions : std::vector<int>{0, 0, 0, 0};

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

    wID hwnd = CreateWindowEx(0,
                                wc.lpszClassName,
                                name.c_str(),
                                WS_OVERLAPPEDWINDOW, // Use a style that allows for resizing
                                dim[0],              // x
                                dim[1],              // y
                                dim[2],              // width
                                dim[3],              // height
                                HWND_MESSAGE,        // Message-only window
                                nullptr,
                                wc.hInstance,
                                nullptr);

    // Hide the window if specified
    if (hide) {
        ShowWindow(hwnd, SW_HIDE);
    }

    return hwnd;
}

ProcessMethod WindowManager::toMethod(cstr method) {
    static const std::unordered_map<str, ProcessMethod> methodMap = {
        {"WaitForTerminate", ProcessMethod::WaitForTerminate},
        {"ForkProcess", ProcessMethod::ForkProcess},
        {"ContinueExecution", ProcessMethod::ContinueExecution},
        {"WaitUntilStarts", ProcessMethod::WaitUntilStarts},
        {"CreateNewWindow", ProcessMethod::CreateNewWindow},
        {"AsyncProcessCreate", ProcessMethod::AsyncProcessCreate},
        {"SystemCall", ProcessMethod::SystemCall}
    };

    auto it = methodMap.find(method);
    return (it != methodMap.end()) ? it->second : ProcessMethod::Invalid;
}
// Function to convert error code to a human-readable message
str WindowManager::GetErrorMessage(DWORD errorCode) {
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
bool WindowManager::CreateProcessWrapper(cstr path, cstr command, DWORD creationFlags, STARTUPINFO& si, PROCESS_INFORMATION& pi) {
    if(path.empty()) {
        return CreateProcess(
            NULL,
            const_cast<char*>(command.c_str()),
            nullptr, nullptr, FALSE,
            creationFlags, nullptr, nullptr,
            &si, &pi
        );
    } else {
        return CreateProcess(
            path.c_str(),
            const_cast<char*>(command.c_str()),
            nullptr, nullptr, FALSE,
            creationFlags, nullptr, nullptr,
            &si, &pi
        );
    }
}
template <typename T>
int64_t WindowManager::Run(str path, T method, str windowState, str command, int priority) {
    ProcessMethod processMethod;
    windowState = ToLower(windowState);
    // Convert method to ProcessMethod if it's a string
    if constexpr (std::is_same_v<T, str>) {
        processMethod = toMethod(method);
    } else if constexpr (std::is_same_v<T, int>) {
        processMethod = static_cast<ProcessMethod>(method);
    } else if constexpr (std::is_same_v<T, ProcessMethod>) {
        processMethod = method;
    } else {
        processMethod = ProcessMethod::Invalid;
    }

#ifdef WINDOWS
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    DWORD creationFlags = windowState == "call" ? CREATE_NEW_PROCESS_GROUP : CREATE_NEW_CONSOLE; // Start with the new process group flag
    // Add the no window flag if you want to suppress the command window
    if (windowState == "hide") {
        creationFlags |= CREATE_NO_WINDOW; // Suppress the console window
    }
    if(!command.empty() && !path.empty() && processMethod != ProcessMethod::Shell){
        command = "\"" + path + "\" " + command;
        path = "";
        lo << "Path " << path << "\nCommand " << command;
    }
    // Set window state based on the `windowState` parameter
    if (!windowState.empty()) {
        si.dwFlags |= STARTF_USESHOWWINDOW; // Enable `wShowWindow` setting
        
        if (windowState == "max") {
            si.wShowWindow = SW_SHOWMAXIMIZED;
        } else if (windowState == "min") {
            si.wShowWindow = SW_SHOWMINIMIZED;
        } else if (windowState == "hide") {
            si.wShowWindow = SW_HIDE;
        } else if (windowState == "unfocused"){
            si.wShowWindow = SW_SHOWNOACTIVATE;
        } else if (windowState == "focused"){
            si.wShowWindow = SW_SHOW;
        } else {
            si.wShowWindow = SW_SHOWNORMAL; // Default
        }
    }

    int processFlags = 0;
    if (priority != -1) {
        switch (priority) {
            case 0: processFlags  = IDLE_PRIORITY_CLASS; break;
            case 1: processFlags  = BELOW_NORMAL_PRIORITY_CLASS; break;
            case 2: processFlags  = NORMAL_PRIORITY_CLASS; break;
            case 3: processFlags  = ABOVE_NORMAL_PRIORITY_CLASS; break;
            case 4: processFlags  = HIGH_PRIORITY_CLASS; break;
            case 5: processFlags  = REALTIME_PRIORITY_CLASS; break;
            case 6: processFlags  = 0x00000200; break;
            default:processFlags  = NORMAL_PRIORITY_CLASS; break;
        }
        creationFlags |= processFlags;
    }
    switch (processMethod) {
        case ProcessMethod::AsyncProcessCreate:
            std::thread([path]() {
                system(path.c_str());
            }).detach();
            return 0;  // Return immediately for async
            break;

        case ProcessMethod::SystemCall:
            return system(path.c_str());  // Synchronous call, returns exit code
        case ProcessMethod::ForkProcess: {
            int forkedProcessID = _spawnl(_P_WAIT, path.c_str(), path.c_str(), NULL);
            if (forkedProcessID == -1) {
                std::cerr << "Failed to fork process." << std::endl;
            } else {
                std::cout << "Forked process ID: " << forkedProcessID << std::endl;
                return forkedProcessID;
            }
            break;
        }
        case ProcessMethod::CreateNewWindow:{
            creationFlags &= ~CREATE_NO_WINDOW;
            creationFlags &= ~CREATE_NEW_PROCESS_GROUP;
            creationFlags |= CREATE_NEW_CONSOLE;
            processMethod = ProcessMethod::ContinueExecution;
            break;
        }
        case ProcessMethod::SameWindow: {
            creationFlags &= ~CREATE_NEW_CONSOLE;
            creationFlags &= ~CREATE_NEW_PROCESS_GROUP;
            processMethod = ProcessMethod::WaitForTerminate;
            break;
        }
        case ProcessMethod::Shell:{
            // Use ShellExecute to open the Steam URL
            HINSTANCE result = ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, si.wShowWindow);
               // Check if the operation was successful
            if ((intptr_t)result <= 32) {
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
    
        case ProcessMethod::WaitUntilStarts: break;
        case ProcessMethod::WaitForTerminate: break;
        case ProcessMethod::ContinueExecution: break;
    }
    // Attempt to create the process once
    if (!CreateProcessWrapper(path, command, creationFlags, si, pi)) {
        DWORD error = GetLastError();
        std::cerr << "Failed to create process. Error: " << error << " - " << GetErrorMessage(error) << std::endl;
        return -1;  // Return error code
    }
    if(priority != -1){
        WindowManager *wm = new WindowManager();
        wm->SetPriority((int) processFlags, GetProcessId(pi.hProcess));
        delete wm;
    }
    switch (processMethod) {
        case ProcessMethod::WaitForTerminate:
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return exitCode;
        case ProcessMethod::ContinueExecution:
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return pi.dwProcessId;  // Return process ID for asynchronous
        case ProcessMethod::WaitUntilStarts:
            WaitForInputIdle(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return pi.dwProcessId;  // Return process ID after process starts
        default:
            std::cerr << "Invalid process method." << std::endl;
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return -1;
    }

#else  // Linux/Unix implementation

    pid_t pid;
    int status;

    int priorityClass = 0; // Default priority
    if (priority != -1) {
        switch (priority) {
            case 1: priorityClass = -10; break;  // High priority
            case 2: priorityClass = -20; break;  // Real-time priority
            case 3: priorityClass = 10; break;   // Below normal
            case 4: priorityClass = 19; break;   // Idle
            default: priorityClass = 0; break;   // Normal
        }
    }

    switch (processMethod) {
        case ProcessMethod::WaitForTerminate:
            pid = fork();
            if (pid == 0) {
                setpriority(PRIO_PROCESS, 0, priorityClass);
                execl(path.c_str(), path.c_str(), command.c_str(), (char*) NULL);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    return WEXITSTATUS(status);  // Return exit code after waiting
                }
            } else {
                std::cerr << "Failed to create process." << std::endl;
            }
            break;

        case ProcessMethod::ForkProcess:
        case ProcessMethod::ContinueExecution:
            pid = fork();
            if (pid == 0) {
                setpriority(PRIO_PROCESS, 0, priorityClass);
                execl(path.c_str(), path.c_str(), command.c_str(), (char*) NULL);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                return pid;  // Return process ID immediately for async
            }
            break;

        case ProcessMethod::WaitUntilStarts:
            pid = fork();
            if (pid == 0) {
                setpriority(PRIO_PROCESS, 0, priorityClass);
                execl(path.c_str(), path.c_str(), command.c_str(), (char*) NULL);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                usleep(100000);  // Give some time for process to start
                return pid;  // Return process ID after process starts
            }
            break;

        case ProcessMethod::AsyncProcessCreate:
            std::thread([path]() {
                system(path.c_str());
            }).detach();
            return 0;  // Return immediately for async

        case ProcessMethod::SystemCall:
            status = system(path.c_str());
            return WEXITSTATUS(status);  // Return exit code for system call

        case ProcessMethod::Invalid:
        default:
            std::cerr << "Invalid process method." << std::endl;
            break;
    }
#endif

    return -1;  // Return -1 if something fails
}
// Terminal function to open a terminal in a new window
int64_t WindowManager::Terminal(cstr command, bool canPause, str windowState, bool continueExecution, cstr terminal) {
    str fullCommand;

#ifdef WINDOWS
    // Prepare the command based on the window state
    if (ToLower(terminal) == "powershell") {
        // Use PowerShell for maximized window
        fullCommand = continueExecution ? "-NoExit " : ""; // Keep PowerShell open if continueExecution is true
        fullCommand += command; // Add the command to execute
        // Launch PowerShell
        return Run("C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", 0, windowState, fullCommand, -1);
    } else {
        // Prepare the command for cmd.exe
        fullCommand = continueExecution ? "/k" : "/c";
        fullCommand += " \"" + command + "\"";
        if (canPause) {
            fullCommand += " && pause"; // Add pause if required
        }
        // Use cmd.exe for Windows
        return Run("C:\\Windows\\System32\\cmd.exe", 0, windowState, fullCommand, -1);
    }
#else
    // Prepare the command for Linux
    fullCommand = command;
    if (canPause) {
        fullCommand += "; read -p 'Press enter to continue...'"; // Pause on Linux
    }
    // Use gnome-terminal or xterm (change based on your preference)
    cstr terminalPath = defaultTerminal; // or "xterm" or another terminal emulator
    return Run(terminalPath, 0, windowState, fullCommand, -1);
#endif
}

void WindowManager::SetPriority(int priority, pID procID) {
    HANDLE hProcess;

    // If procID is NULL, use the current process
    if (procID == 0) {
        procID = pid;
    }

    // Open the process with necessary permissions
    hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION, FALSE, procID);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return;
    }

    // Map the integer priority to appropriate Windows constants
    DWORD priorityClass;
    switch (priority) {
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
        case 6:
            priorityClass = 0x00000200; 
            break;
        default:
            priorityClass = (DWORD) priority;
            break;
    }

    if(priority < 0){
        priorityClass = static_cast<DWORD>(std::abs(priority));
    }
    // Set the priority class
    if (!SetPriorityClass(hProcess, priorityClass)) {
        std::cerr << "Failed to set priority class. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return;
    }
    if(pid != procID) return;
    // Optionally, set thread priority based on the same integer
    HANDLE hThread = GetCurrentThread();
    int threadPriority;
    switch (priority) {
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
        case 6:
            threadPriority = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        default:
            threadPriority = THREAD_PRIORITY_NORMAL; // Default if invalid
            break;
    } 
    if(priority < 0){
        threadPriority = std::abs(priority);
    }

    if (!SetThreadPriority(hThread, threadPriority)) {
        std::cerr << "Failed to set thread priority. Error: " << GetLastError() << std::endl;
    } else {
        std::cout << "Priority set successfully." << std::endl;
    }

    // Clean up
    CloseHandle(hProcess);
}

// Explicit instantiation for the Run method with int
template int64_t WindowManager::Run<int>(str, int, str, str, int);
template int64_t WindowManager::Run<str>(str, str, str, str, int);
template int64_t WindowManager::Run<ProcessMethod>(str, ProcessMethod, str, str, int);