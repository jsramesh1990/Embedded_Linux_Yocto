#ifndef LOG_SINK_H
#define LOG_SINK_H

#include <string>
#include <json/json.h>
#include "LogLevel.h"

namespace common {

/**
 * @brief Base interface for log sinks
 * 
 * A log sink receives log messages and outputs them to a destination
 * (console, file, syslog, network, etc.)
 */
class LogSink {
public:
    LogSink() = default;
    virtual ~LogSink() = default;
    
    /**
     * @brief Initialize the sink
     * @param config Configuration for the sink
     * @return true if initialization successful
     */
    virtual bool init(const Json::Value& config) { return true; }
    
    /**
     * @brief Write a log message
     * @param level Log level
     * @param message Formatted log message
     * @param context Additional context as JSON
     */
    virtual void write(LogLevel level, const std::string& message, 
                      const Json::Value& context = Json::nullValue) = 0;
    
    /**
     * @brief Flush any buffered messages
     */
    virtual void flush() {}
    
    /**
     * @brief Set minimum log level for this sink
     * @param level Minimum log level
     */
    virtual void setLevel(LogLevel level) { minLevel = level; }
    
    /**
     * @brief Get minimum log level
     */
    virtual LogLevel getLevel() const { return minLevel; }
    
    /**
     * @brief Check if this level should be logged
     */
    virtual bool shouldLog(LogLevel level) const {
        return level >= minLevel;
    }
    
    /**
     * @brief Set format pattern
     * @param pattern Format string (e.g., "%time [%level] %message")
     */
    virtual void setFormat(const std::string& pattern) { formatPattern = pattern; }
    
    /**
     * @brief Get format pattern
     */
    virtual std::string getFormat() const { return formatPattern; }
    
    /**
     * @brief Enable/disable colored output
     */
    virtual void setColored(bool colored) { this->colored = colored; }
    
    /**
     * @brief Check if colored output is enabled
     */
    virtual bool isColored() const { return colored; }
    
    /**
     * @brief Get sink name
     */
    virtual std::string getName() const { return name; }
    
    /**
     * @brief Set sink name
     */
    virtual void setName(const std::string& name) { this->name = name; }

protected:
    LogLevel minLevel = LogLevel::TRACE;
    std::string formatPattern = "[%time] [%level] %message";
    bool colored = false;
    std::string name = "LogSink";
};

} // namespace common

#endif // LOG_SINK_H
