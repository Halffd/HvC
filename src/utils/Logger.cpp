#include "Logger.h"

Logger lo(LoggerPaths::DEFAULT_LOG);

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
    if (errorLogStream.is_open()) {
        errorLogStream.close();
    }
}

void Logger::createLog(const std::string& logFile) {
    // Ensure the log directory exists
    LoggerPaths::EnsureLogDir();
    
    // Get the full path for the log file
    logFilePath = LoggerPaths::GetLogPath(logFile);
    
    // Rotate log if needed
    LoggerPaths::RotateLogIfNeeded(logFilePath);
    
    // Open log file
    logFileStream.open(logFilePath, std::ios::app);
    if (!logFileStream.is_open()) {
        throw std::runtime_error("Failed to open log file: " + logFilePath);
    }
    
    // Also open error log file
    std::string errorLogPath = LoggerPaths::ERROR_LOG;
    LoggerPaths::RotateLogIfNeeded(errorLogPath);
    errorLogStream.open(errorLogPath, std::ios::app);
    if (!errorLogStream.is_open()) {
        std::cerr << "Warning: Failed to open error log file: " << errorLogPath << std::endl;
    }
    
    // Log starting message
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream startMsg;
    startMsg << "\n=============================================\n"
             << "Log started at " << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
             << "\n=============================================\n";
             
    logFileStream << startMsg.str();
    logFileStream.flush();
}

std::string Logger::getCurrentTime() {
    if (!timestampEnabled) {
        return "";
    }
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
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

void Logger::logError(const std::string& message) {
    if (!errorLogStream.is_open()) {
        std::string errorLogPath = LoggerPaths::ERROR_LOG;
        LoggerPaths::EnsureLogDir();
        errorLogStream.open(errorLogPath, std::ios::app);
        if (!errorLogStream.is_open()) {
            std::cerr << "Failed to open error log for writing" << std::endl;
            return;
        }
    }
    
    std::lock_guard<std::mutex> lock(logMutex);
    std::string timestamp = getCurrentTime();
    errorLogStream << timestamp << " [ERROR] " << message << std::endl;
    errorLogStream.flush();
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
        // Print to appropriate output stream based on level
        if (level == LogLevel::ERR || level == LogLevel::WARNING) {
            std::cerr << logMessage << std::endl;
        } else {
            std::cout << logMessage << std::endl;
        }
    }

    if (writeToLog && logFileStream.is_open()) {
        logFileStream << logMessage << std::endl;
        logFileStream.flush(); // Ensure logs are written immediately
    }
}

// Overloaded operator<< for standard and custom types
Logger& Logger::operator<<(const std::string& message) {
    log(message, true, LogLevel::INFO);
    return *this;
}

#ifdef WINDOWS
Logger& Logger::operator<<(int64_t value) {
    log(std::to_string(value), true, LogLevel::INFO);
    return *this;
}
#endif

// Specialization for std::endl
Logger& Logger::operator<<(std::ostream& (*manip)(std::ostream&)) {
    // Log the manipulator as a string
    if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl)) {
        log("\n", true, LogLevel::INFO);
    } else {
        log("Unknown manipulator", true, LogLevel::INFO);
    }
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
template void Logger::log<std::string>(std::string message, bool doPrint, LogLevel level);
template void Logger::log<const char*>(const char* message, bool doPrint, LogLevel level);
template void Logger::log<int>(int message, bool doPrint, LogLevel level);
template void Logger::log<double>(double message, bool doPrint, LogLevel level);
template void Logger::log<bool>(bool message, bool doPrint, LogLevel level);

template void Logger::debug<std::string>(std::string message, bool doPrint);
template void Logger::debug<const char*>(const char* message, bool doPrint);
template void Logger::info<std::string>(std::string message, bool doPrint);
template void Logger::info<const char*>(const char* message, bool doPrint);
template void Logger::warning<std::string>(std::string message, bool doPrint);
template void Logger::warning<const char*>(const char* message, bool doPrint);
template void Logger::error<std::string>(std::string message, bool doPrint);
template void Logger::error<const char*>(const char* message, bool doPrint);