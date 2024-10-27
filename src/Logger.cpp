#include "Logger.h"

Logger lo("HvC.log");

// Constructor
Logger::Logger(const std::string& logFile, bool enableTimestamp, bool write, LogLevel level)
    : timestampEnabled(enableTimestamp), writeToLog(write), logLevel(level) {
    if (writeToLog) {
        createLog(logFile);
    }
}

// Destructor
Logger::~Logger() {
    if (logFileStream.is_open()) {
        logFileStream.close();
    }
}

void Logger::createLog(const std::string& logFile) {
    logFileStream.open(logFile, std::ios::app);
    if (!logFileStream.is_open()) {
        throw std::runtime_error("Failed to open log file.");
    }
}

std::string Logger::getCurrentTime() {
    if (!timestampEnabled) {
        return "";
    }
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%d/%m/%Y %H:%M:%S");
    return ss.str();
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERR: return "ERROR";
        case LogLevel::DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}
template <typename T>
void Logger::log(T message, bool doPrint, LogLevel level) {
    if (level < logLevel) return;

    std::lock_guard<std::mutex> lock(logMutex);
    
    // Use stringstream to handle any type for message
    std::ostringstream ss;
    ss << getCurrentTime() << " [" << logLevelToString(level) << "] " << message;
    std::string logMessage = ss.str();

    if (doPrint) {
        std::cout << logMessage << std::endl;
    }

    if (writeToLog && logFileStream.is_open()) {
        logFileStream << logMessage << std::endl;
    }
}

// Overloaded operator<< for standard and custom types
Logger& Logger::operator<<(const std::string& message) {
    log(message, true, LogLevel::INFO);
    return *this;
}

Logger& Logger::operator<<(const char* message) {
    log(std::string(message), true, LogLevel::INFO);
    return *this;
}

Logger& Logger::operator<<(int value) {
    log(std::to_string(value), true, LogLevel::INFO);
    return *this;
}

Logger& Logger::operator<<(long value) {
    log(std::to_string(value), true, LogLevel::INFO);
    return *this;
}

Logger& Logger::operator<<(double value) {
    log(std::to_string(value), true, LogLevel::INFO);
    return *this;
}

Logger& Logger::operator<<(bool value) {
    log(value ? "true" : "false", true, LogLevel::INFO);
    return *this;
}

#ifdef _WIN32
Logger& Logger::operator<<(HWND hwnd) {
    std::ostringstream ss;
    ss << reinterpret_cast<void*>(hwnd);  // Convert HWND to a void* for logging
    log(ss.str(), true, LogLevel::INFO);
    return *this;
}
#endif

Logger& Logger::operator<<(LogLevel level) {
    setLogLevel(level);
    return *this;
}

void Logger::setLogLevel(LogLevel level) {
    logLevel = level;
}

void Logger::enableTimestamp(bool enable) {
    timestampEnabled = enable;
}

template<typename... Args>
void Logger::printf(const char* format, Args... args) {
    char buffer[4096];
    std::snprintf(buffer, sizeof(buffer), format, args...);
    log(buffer, false);
}

// Explicit instantiation for template methods (optional)
template void Logger::log<int>(int, bool, LogLevel);
template void Logger::log<long>(long, bool, LogLevel);
template void Logger::log<double>(double, bool, LogLevel);
template void Logger::log<bool>(bool, bool, LogLevel);
template void Logger::log<std::string>(std::string, bool, LogLevel);
template void Logger::log<const char*>(const char*, bool, LogLevel);