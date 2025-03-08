#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <string>
#include <mutex>
#include <chrono>
#include "types.hpp"

#ifdef _WIN32
#include <windows.h>  // Only include if on Windows
#endif

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERR
};

class Logger {
public:
    bool timestampEnabled = true;
    bool writeToLog = true;

    Logger() = default;
    Logger(const std::string& logFile, bool enableTimestamp = true, bool write = true, LogLevel level = LogLevel::INFO);
    ~Logger();

    void createLog(const std::string& logFile);
    std::string getCurrentTime();
    std::string logLevelToString(LogLevel level);

    template <typename T>
    void log(T message, bool doPrint = true, LogLevel level = LogLevel::INFO);

    // Overloaded operator for output
    Logger& operator<<(const std::string& message);
    Logger& operator<<(const char* message);
    Logger& operator<<(int value);
    Logger& operator<<(long value);
    Logger& operator<<(double value);
    Logger& operator<<(bool value);
    #ifdef WINDOWS
    Logger& operator<<(int64_t value);
    #endif

    // Specialization for std::endl
    Logger& operator<<(std::ostream& (*manip)(std::ostream&));
#ifdef _WIN32
    Logger& operator<<(HWND hwnd);
#endif

    Logger& operator<<(LogLevel level);
    void setLogLevel(LogLevel level);
    void enableTimestamp(bool enable);

    template<typename... Args>
    void printf(const char* format, Args... args);

private:
    std::ofstream logFileStream;
    LogLevel logLevel;
    std::mutex logMutex;
};

extern Logger lo;

#endif // LOGGER_H
