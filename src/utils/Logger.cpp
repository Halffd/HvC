#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <filesystem>

// Global logger instance
H::Logger lo("hvc.log");

H::Logger::Logger(const std::string& filename) {
    try {
        // Create log directory if it doesn't exist
        std::filesystem::create_directories("logs");
        
        // Open log file
        logFile.open("logs/" + filename, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
        
        // Log startup message
        log(INFO, "Logger initialized");
    } catch (const std::exception& e) {
        std::cerr << "Error initializing logger: " << e.what() << std::endl;
    }
}

H::Logger::~Logger() {
    if (logFile.is_open()) {
        log(INFO, "Logger shutting down");
        logFile.close();
    }
}

void H::Logger::debug(const std::string& message) {
    log(DEBUG, message);
}

void H::Logger::info(const std::string& message) {
    log(INFO, message);
}

void H::Logger::warning(const std::string& message) {
    log(WARNING, message);
}

void H::Logger::error(const std::string& message) {
    log(ERROR, message);
}

void H::Logger::fatal(const std::string& message) {
    log(FATAL, message);
}

void H::Logger::setLevel(Level level) {
    std::lock_guard<std::mutex> lock(logMutex);
    logLevel = level;
}

void H::Logger::log(Level level, const std::string& message) {
    if (level < logLevel) return;
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Get current time
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    // Log level strings
    const char* levelStrings[] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
    };
    
    // Format: [2023-01-01 12:34:56] [INFO] Message
    std::ostringstream logMessage;
    logMessage << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
               << "[" << levelStrings[level] << "] "
               << message;
    
    // Write to console
    std::cout << logMessage.str() << std::endl;
    
    // Write to file
    if (logFile.is_open()) {
        logFile << logMessage.str() << std::endl;
        logFile.flush();
    }
    
    // If it's an error or fatal, also write to stderr
    if (level >= ERROR) {
        std::cerr << logMessage.str() << std::endl;
    }
}

void LoggerPaths::EnsureLogDir() {
    try {
        std::filesystem::create_directories("logs");
    } catch (const std::exception& e) {
        std::cerr << "Failed to create log directory: " << e.what() << std::endl;
    }
}