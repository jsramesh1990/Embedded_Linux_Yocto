#ifndef LOG_LEVEL_H
#define LOG_LEVEL_H

#include <string>
#include <map>

namespace common {

/**
 * @brief Log levels supported by the logging system
 */
enum class LogLevel {
    TRACE = 0,      // Very detailed debugging
    DEBUG = 1,      // Debugging information
    INFO = 2,       // General information
    NOTICE = 3,     // Noticeable events
    WARN = 4,       // Warning conditions
    ERROR = 5,      // Error conditions
    CRITICAL = 6,   // Critical conditions
    FATAL = 7,      // Fatal errors (application crash)
    OFF = 8         // No logging
};

/**
 * @brief Convert LogLevel to string
 */
inline std::string logLevelToString(LogLevel level) {
    static const std::map<LogLevel, std::string> levelMap = {
        {LogLevel::TRACE, "TRACE"},
        {LogLevel::DEBUG, "DEBUG"},
        {LogLevel::INFO, "INFO"},
        {LogLevel::NOTICE, "NOTICE"},
        {LogLevel::WARN, "WARN"},
        {LogLevel::ERROR, "ERROR"},
        {LogLevel::CRITICAL, "CRITICAL"},
        {LogLevel::FATAL, "FATAL"},
        {LogLevel::OFF, "OFF"}
    };
    
    auto it = levelMap.find(level);
    return it != levelMap.end() ? it->second : "UNKNOWN";
}

/**
 * @brief Convert string to LogLevel
 */
inline LogLevel stringToLogLevel(const std::string& str) {
    static const std::map<std::string, LogLevel> levelMap = {
        {"TRACE", LogLevel::TRACE},
        {"DEBUG", LogLevel::DEBUG},
        {"INFO", LogLevel::INFO},
        {"NOTICE", LogLevel::NOTICE},
        {"WARN", LogLevel::WARN},
        {"WARNING", LogLevel::WARN},
        {"ERROR", LogLevel::ERROR},
        {"CRITICAL", LogLevel::CRITICAL},
        {"FATAL", LogLevel::FATAL},
        {"OFF", LogLevel::OFF}
    };
    
    auto it = levelMap.find(str);
    return it != levelMap.end() ? it->second : LogLevel::INFO;
}

/**
 * @brief Get ANSI color code for log level
 */
inline std::string logLevelColor(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "\033[37m";     // White
        case LogLevel::DEBUG: return "\033[36m";     // Cyan
        case LogLevel::INFO: return "\033[32m";      // Green
        case LogLevel::NOTICE: return "\033[34m";    // Blue
        case LogLevel::WARN: return "\033[33m";      // Yellow
        case LogLevel::ERROR: return "\033[31m";     // Red
        case LogLevel::CRITICAL: return "\033[35m";  // Magenta
        case LogLevel::FATAL: return "\033[41m";     // Red background
        default: return "\033[0m";                   // Reset
    }
}

/**
 * @brief Get syslog priority for log level
 */
inline int logLevelToSyslog(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return 7;   // LOG_DEBUG
        case LogLevel::DEBUG: return 7;   // LOG_DEBUG
        case LogLevel::INFO: return 6;    // LOG_INFO
        case LogLevel::NOTICE: return 5;  // LOG_NOTICE
        case LogLevel::WARN: return 4;    // LOG_WARNING
        case LogLevel::ERROR: return 3;   // LOG_ERR
        case LogLevel::CRITICAL: return 2; // LOG_CRIT
        case LogLevel::FATAL: return 1;   // LOG_ALERT
        default: return 6;
    }
}

} // namespace common

#endif // LOG_LEVEL_H
