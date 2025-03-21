#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace H {

// Type aliases
using str = std::string;
using cstr = const std::string&;
using wID = unsigned long;
using pID = unsigned long;

// Group type definition
using group = std::unordered_map<std::string, std::vector<std::string>>;

// Process method enum
enum class ProcessMethod {
    Invalid = -1,
    WaitForTerminate = 0,
    ForkProcess = 1,
    ContinueExecution = 2,
    WaitUntilStarts = 3,
    CreateNewWindow = 4,
    AsyncProcessCreate = 5,
    SystemCall = 6,
    SameWindow = 7,
    Shell = 8
};

// Helper function to convert string to lowercase
inline std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Common typedefs for key handling
#ifndef Key_typedef
#define Key_typedef
typedef unsigned long Key;
#endif

// Helper struct for Windows EnumWindows
#ifdef _WIN32
struct EnumWindowsData {
    std::string targetProcessName;
    wID id = 0;
    
    explicit EnumWindowsData(const std::string& name) : targetProcessName(name) {}
};
#endif

} // namespace H
