/**
 * Alpha1 Core Logging System
 * 
 * Thread-safe logging with multiple output targets and log levels.
 * Supports formatted output, file rotation, and telemetry integration.
 * 
 * @file logging.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <string>
#include <functional>
#include <mutex>
#include <fstream>
#include <iostream>
#include <chrono>

namespace Alpha1::Core {

/**
 * Log callback function type
 */
using LogCallback = std::function<void(LogLevel level, const std::string& message)>;

/**
 * Logger singleton class
 */
class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }
    
    /**
     * Set minimum log level
     */
    void SetLogLevel(LogLevel level) {
        m_minLevel = level;
    }
    
    /**
     * Add log callback
     */
    void AddCallback(LogCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.push_back(std::move(callback));
    }
    
    /**
     * Set log file
     */
    bool SetLogFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logFile.open(path, std::ios::app);
        return m_logFile.is_open();
    }
    
    /**
     * Log a message
     */
    void Log(LogLevel level, const std::string& message) {
        if (level < m_minLevel) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Format timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::string timestamp = std::ctime(&time);
        timestamp.pop_back(); // Remove newline
        
        // Format level string
        const char* levelStr = nullptr;
        switch (level) {
            case LogLevel::Trace:   levelStr = "TRACE"; break;
            case LogLevel::Debug:   levelStr = "DEBUG"; break;
            case LogLevel::Info:    levelStr = "INFO";  break;
            case LogLevel::Warning: levelStr = "WARN";  break;
            case LogLevel::Error:   levelStr = "ERROR"; break;
            case LogLevel::Fatal:   levelStr = "FATAL"; break;
        }
        
        std::string formatted = "[" + timestamp + "] [" + levelStr + "] " + message;
        
        // Output to console
        if (level >= LogLevel::Warning) {
            std::cerr << formatted << std::endl;
        } else {
            std::cout << formatted << std::endl;
        }
        
        // Output to file
        if (m_logFile.is_open()) {
            m_logFile << formatted << std::endl;
        }
        
        // Invoke callbacks
        for (auto& callback : m_callbacks) {
            callback(level, formatted);
        }
    }
    
private:
    Logger() : m_minLevel(LogLevel::Info) {}
    ~Logger() {
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
    }
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::mutex m_mutex;
    std::vector<LogCallback> m_callbacks;
    std::ofstream m_logFile;
    LogLevel m_minLevel;
};

/**
 * Log macros for convenient usage
 */
#define LOG_TRACE(msg) Alpha1::Core::Logger::Instance().Log(Alpha1::Core::LogLevel::Trace, msg)
#define LOG_DEBUG(msg) Alpha1::Core::Logger::Instance().Log(Alpha1::Core::LogLevel::Debug, msg)
#define LOG_INFO(msg)  Alpha1::Core::Logger::Instance().Log(Alpha1::Core::LogLevel::Info, msg)
#define LOG_WARN(msg)  Alpha1::Core::Logger::Instance().Log(Alpha1::Core::LogLevel::Warning, msg)
#define LOG_ERROR(msg) Alpha1::Core::Logger::Instance().Log(Alpha1::Core::LogLevel::Error, msg)
#define LOG_FATAL(msg) Alpha1::Core::Logger::Instance().Log(Alpha1::Core::LogLevel::Fatal, msg)

} // namespace Alpha1::Core
