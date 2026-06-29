#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <map>
#include <functional>
#include <chrono>
#include <json/json.h>
#include "LogLevel.h"
#include "LogSink.h"
#include "ConsoleSink.h"
#include "FileSink.h"
#include "SyslogSink.h"
#include "NetworkSink.h"
#include "RotatingFileSink.h"

namespace common {

/**
 * @brief Main Logger class for the application
 * 
 * Provides comprehensive logging with multiple sinks,
 * structured logging, and performance monitoring
 */
class Logger {
public:
    /**
     * @brief Log event structure for structured logging
     */
    struct LogEvent {
        LogLevel level;
        std::string message;
        Json::Value context;
        std::chrono::system_clock::time_point timestamp;
        std::string threadId;
        std::string file;
        int line;
        std::string function;
        std::string sessionId;
        std::string userId;
        uint64_t sequence;
    };
    
    /**
     * @brief Logger configuration
     */
    struct Config {
        LogLevel level = LogLevel::INFO;
        std::string format = "[%time] [%level] %message";
        bool colored = false;
        bool includeThreadId = true;
        bool includeFileInfo = false;
        bool includeSessionId = false;
        std::vector<std::unique_ptr<LogSink>> sinks;
    };
    
    /**
     * @brief Get singleton instance
     */
    static Logger& getInstance();
    
    /**
     * @brief Initialize logger with configuration
     * @param configFile Path to config file (JSON/YAML)
     * @return true if initialization successful
     */
    bool initialize(const std::string& configFile = "");
    
    /**
     * @brief Initialize logger with configuration
     * @param config Configuration object
     */
    bool initialize(const Config& config);
    
    /**
     * @brief Check if logger is initialized
     */
    bool isInitialized() const { return initialized; }
    
    /**
     * @brief Add a log sink
     * @param sink Sink to add
     */
    void addSink(std::unique_ptr<LogSink> sink);
    
    /**
     * @brief Remove all sinks of a specific type
     * @param type Sink type name
     */
    void removeSinks(const std::string& type);
    
    /**
     * @brief Remove all sinks
     */
    void clearSinks();
    
    /**
     * @brief Set global log level
     * @param level Minimum log level
     */
    void setLevel(LogLevel level);
    
    /**
     * @brief Get global log level
     */
    LogLevel getLevel() const { return currentLevel; }
    
    /**
     * @brief Set log format pattern
     * @param format Format string
     */
    void setFormat(const std::string& format) { this->format = format; }
    
    /**
     * @brief Enable/disable colored output
     */
    void setColored(bool colored) { this->colored = colored; }
    
    /**
     * @brief Set session ID
     */
    void setSessionId(const std::string& sessionId) { this->sessionId = sessionId; }
    
    /**
     * @brief Set user ID
     */
    void setUserId(const std::string& userId) { this->userId = userId; }
    
    /**
     * @brief Get session ID
     */
    std::string getSessionId() const { return sessionId; }
    
    /**
     * @brief Log a message
     * @param level Log level
     * @param message Message to log
     * @param context Additional context as JSON
     */
    void log(LogLevel level, const std::string& message, 
             const Json::Value& context = Json::nullValue);
    
    /**
     * @brief Log a structured event
     * @param event Log event structure
     */
    void logEvent(const LogEvent& event);
    
    /**
     * @brief Convenience methods for each log level
     */
    void trace(const std::string& message, const Json::Value& context = Json::nullValue) {
        log(LogLevel::TRACE, message, context);
    }
    void debug(const std::string& message, const Json::Value& context = Json::nullValue) {
        log(LogLevel::DEBUG, message, context);
    }
    void info(const std::string& message, const Json::Value& context = Json::nullValue) {
        log(LogLevel::INFO, message, context);
    }
    void notice(const std::string& message, const Json::Value& context = Json::nullValue) {
        log(LogLevel::NOTICE, message, context);
    }
    void warn(const std::string& message, const Json::Value& context = Json::nullValue) {
        log(LogLevel::WARN, message, context);
    }
    void error(const std::string& message, const Json::Value& context = Json::nullValue) {
        log(LogLevel::ERROR, message, context);
    }
    void critical(const std::string& message, const Json::Value& context = Json::nullValue) {
        log(LogLevel::CRITICAL, message, context);
    }
    void fatal(const std::string& message, const Json::Value& context = Json::nullValue) {
        log(LogLevel::FATAL, message, context);
    }
    
    /**
     * @brief Log with file/line info
     */
    void logWithLocation(LogLevel level, const std::string& message,
                         const char* file, int line, const char* function,
                         const Json::Value& context = Json::nullValue);
    
    /**
     * @brief Flush all sinks
     */
    void flush();
    
    /**
     * @brief Get statistics
     * @return JSON statistics
     */
    Json::Value getStats() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
    /**
     * @brief Set callback for log events
     * @param callback Function called for each log event
     */
    void setLogCallback(std::function<void(const LogEvent&)> callback);
    
    /**
     * @brief Enable/disable performance monitoring
     */
    void setPerformanceMonitoring(bool enable) { performanceMonitoring = enable; }
    
    /**
     * @brief Get logger configuration
     */
    Config getConfig() const;

private:
    // Singleton
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // State
    bool initialized = false;
    LogLevel currentLevel = LogLevel::INFO;
    std::string format = "[%time] [%level] %message";
    bool colored = false;
    bool includeThreadId = true;
    bool includeFileInfo = false;
    bool includeSessionId = false;
    bool performanceMonitoring = false;
    std::string sessionId;
    std::string userId;
    std::function<void(const LogEvent&)> logCallback;
    
    // Sinks
    std::vector<std::unique_ptr<LogSink>> sinks;
    mutable std::mutex sinksMutex;
    
    // Statistics
    struct Stats {
        std::atomic<uint64_t> totalLogs{0};
        std::atomic<uint64_t> errors{0};
        std::atomic<uint64_t> warnings{0};
        std::map<LogLevel, std::atomic<uint64_t>> levelCounts;
        std::chrono::steady_clock::time_point startTime;
    };
    Stats stats;
    std::mutex statsMutex;
    
    // Internal methods
    std::string formatMessage(const LogEvent& event);
    std::string getCurrentTime() const;
    std::string getThreadId() const;
    void incrementStats(LogLevel level);
    bool shouldLog(LogLevel level) const;
    Json::Value createDefaultContext();
    void loadConfigFromFile(const std::string& configFile);
    void loadConfigFromJson(const Json::Value& config);
    LogSink* createSink(const std::string& type, const Json::Value& config);
};

/**
 * @brief Macros for easy logging with file/line info
 */
#define LOG_TRACE(msg, ...) \
    Logger::getInstance().logWithLocation(LogLevel::TRACE, msg, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_DEBUG(msg, ...) \
    Logger::getInstance().logWithLocation(LogLevel::DEBUG, msg, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_INFO(msg, ...) \
    Logger::getInstance().logWithLocation(LogLevel::INFO, msg, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_NOTICE(msg, ...) \
    Logger::getInstance().logWithLocation(LogLevel::NOTICE, msg, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_WARN(msg, ...) \
    Logger::getInstance().logWithLocation(LogLevel::WARN, msg, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_ERROR(msg, ...) \
    Logger::getInstance().logWithLocation(LogLevel::ERROR, msg, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_CRITICAL(msg, ...) \
    Logger::getInstance().logWithLocation(LogLevel::CRITICAL, msg, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_FATAL(msg, ...) \
    Logger::getInstance().logWithLocation(LogLevel::FATAL, msg, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

/**
 * @brief Macro for logging with context
 */
#define LOG_WITH_CONTEXT(level, msg, context) \
    Logger::getInstance().logWithLocation(level, msg, __FILE__, __LINE__, __FUNCTION__, context)

} // namespace common

#endif // LOGGER_H
