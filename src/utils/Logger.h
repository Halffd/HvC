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
#include <filesystem>
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

// Log directory and file paths
namespace LoggerPaths {
    // Log directory
    static const std::string LOG_DIR = "log/";
    
    // Default log file
    static const std::string DEFAULT_LOG = LOG_DIR + "HvC.log";
    
    // Error log file
    static const std::string ERROR_LOG = LOG_DIR + "error.log";
    
    // Get full path for a log file
    inline std::string GetLogPath(const std::string& filename) {
        if (filename.find('/') != std::string::npos) {
            // If already contains a path separator, use as-is
            return filename;
        }
        return LOG_DIR + filename;
    }
    
    // Ensure log directory exists
    inline void EnsureLogDir() {
        namespace fs = std::filesystem;
        try {
            if (!fs::exists(LOG_DIR)) {
                fs::create_directories(LOG_DIR);
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Failed to create log directory: " << e.what() << "\n";
        }
    }
    
    // Rotate log file if it gets too large (>10MB)
    inline void RotateLogIfNeeded(const std::string& logPath, size_t maxSize = 10 * 1024 * 1024) {
        namespace fs = std::filesystem;
        try {
            if (fs::exists(logPath) && fs::file_size(logPath) > maxSize) {
                // Create timestamped backup
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                struct tm timeInfo;
                localtime_r(&time, &timeInfo);
                
                char timestamp[20];
                std::strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", &timeInfo);
                
                std::string backupPath = logPath + "." + timestamp;
                fs::rename(logPath, backupPath);
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Failed to rotate log file: " << e.what() << "\n";
        }
    }
}

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

    // Log directly to error log regardless of current log level
    void logError(const std::string& message);

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
    
    // Convenience methods for different log levels
    template <typename T>
    void debug(T message, bool doPrint = true) {
        log(message, doPrint, LogLevel::DEBUG);
    }
    
    template <typename T>
    void info(T message, bool doPrint = true) {
        log(message, doPrint, LogLevel::INFO);
    }
    
    template <typename T>
    void warning(T message, bool doPrint = true) {
        log(message, doPrint, LogLevel::WARNING);
    }
    
    template <typename T>
    void error(T message, bool doPrint = true) {
        log(message, doPrint, LogLevel::ERR);
        logError(message); // Also log to error log
    }

private:
    std::ofstream logFileStream;
    std::ofstream errorLogStream;
    LogLevel logLevel;
    std::mutex logMutex;
    std::string logFilePath;
};

extern Logger lo;

#endif // LOGGER_H
