#ifndef CONSOLE_SINK_H
#define CONSOLE_SINK_H

#include "LogSink.h"
#include <iostream>
#include <mutex>

namespace common {

/**
 * @brief Console log sink (stdout/stderr)
 * 
 * Writes log messages to standard output or error
 * with optional color support
 */
class ConsoleSink : public LogSink {
public:
    ConsoleSink(bool useStdErr = false);
    virtual ~ConsoleSink() = default;
    
    /**
     * @brief Initialize console sink
     * @param config Configuration JSON
     */
    virtual bool init(const Json::Value& config) override;
    
    /**
     * @brief Write log message to console
     * @param level Log level
     * @param message Formatted message
     * @param context Additional context
     */
    virtual void write(LogLevel level, const std::string& message,
                      const Json::Value& context = Json::nullValue) override;
    
    /**
     * @brief Flush output stream
     */
    virtual void flush() override;
    
    /**
     * @brief Set whether to use stderr
     */
    void setUseStdErr(bool useStdErr) { this->useStdErr = useStdErr; }
    
    /**
     * @brief Enable/disable timestamps in output
     */
    void setShowTimestamp(bool show) { showTimestamp = show; }
    
    /**
     * @brief Enable/disable thread IDs in output
     */
    void setShowThreadId(bool show) { showThreadId = show; }

private:
    bool useStdErr;
    bool showTimestamp = true;
    bool showThreadId = true;
    std::mutex mutex;
    
    std::string formatMessage(LogLevel level, const std::string& message,
                             const Json::Value& context);
    std::string getCurrentTime() const;
};

} // namespace common

#endif // CONSOLE_SINK_H
