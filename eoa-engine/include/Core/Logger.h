#pragma once

#include "Core/Platform.h"
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace EOA {

// ============================================
// СИСТЕМА ЛОГИРОВАНИЯ (Logger)
// ============================================

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    static Logger* GetInstance() {
        static Logger instance;
        return &instance;
    }
    
    void SetLogLevel(LogLevel level) { MinLevel = level; }
    LogLevel GetLogLevel() const { return MinLevel; }
    
    // Логирование с разными уровнями
    void Log(LogLevel level, const std::string& message) {
        if (level < MinLevel) return;
        
        std::stringstream ss;
        ss << "[" << GetLevelString(level) << "] " 
           << GetTimestamp() << ": " << message;
        
        std::string line = ss.str();
        
        #ifdef EOA_PLATFORM_WINDOWS
            OutputDebugStringA(line.c_str());
            OutputDebugStringA("\n");
        #endif
        
        std::cout << line << std::endl;
        
        // Сохранение в файл (можно добавить)
        if (LogFile.is_open()) {
            LogFile << line << std::endl;
        }
    }
    
    void OpenLogFile(const std::string& path) {
        LogFile.open(path, std::ios::app);
    }
    
    void CloseLogFile() {
        if (LogFile.is_open()) {
            LogFile.close();
        }
    }
    
    // Удобные макросы
    #define EOA_LOG_TRACE(msg) EOA::Logger::GetInstance()->Log(EOA::LogLevel::Trace, msg)
    #define EOA_LOG_DEBUG(msg) EOA::Logger::GetInstance()->Log(EOA::LogLevel::Debug, msg)
    #define EOA_LOG_INFO(msg)  EOA::Logger::GetInstance()->Log(EOA::LogLevel::Info, msg)
    #define EOA_LOG_WARN(msg)  EOA::Logger::GetInstance()->Log(EOA::LogLevel::Warning, msg)
    #define EOA_LOG_ERROR(msg) EOA::Logger::GetInstance()->Log(EOA::LogLevel::Error, msg)
    #define EOA_LOG_CRITICAL(msg) EOA::Logger::GetInstance()->Log(EOA::LogLevel::Critical, msg)
    
private:
    Logger() {
        MinLevel = LogLevel::Info;
    }
    
    std::string GetLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace:    return "TRACE";
            case LogLevel::Debug:    return "DEBUG";
            case LogLevel::Info:     return "INFO";
            case LogLevel::Warning:  return "WARN";
            case LogLevel::Error:    return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }
    
    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    LogLevel MinLevel;
    std::ofstream LogFile;
};

} // namespace EOA

// Глобальные макросы
#define EOA_LOG_TRACE(msg) EOA::Logger::GetInstance()->Log(EOA::LogLevel::Trace, msg)
#define EOA_LOG_DEBUG(msg) EOA::Logger::GetInstance()->Log(EOA::LogLevel::Debug, msg)
#define EOA_LOG_INFO(msg)  EOA::Logger::GetInstance()->Log(EOA::LogLevel::Info, msg)
#define EOA_LOG_WARN(msg)  EOA::Logger::GetInstance()->Log(EOA::LogLevel::Warning, msg)
#define EOA_LOG_ERROR(msg) EOA::Logger::GetInstance()->Log(EOA::LogLevel::Error, msg)
#define EOA_LOG_CRITICAL(msg) EOA::Logger::GetInstance()->Log(EOA::LogLevel::Critical, msg)
