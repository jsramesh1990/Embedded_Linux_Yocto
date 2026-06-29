#ifndef SYSLOG_SINK_H
#define SYSLOG_SINK_H

#include "LogSink.h"
#include <string>
#include <mutex>

namespace common {

/**
 * @brief Syslog sink
 * 
 * Writes log messages to the system logger (syslog)
 * Only available on Unix-like systems
 */
class SyslogSink : public LogSink {
public:
    SyslogSink();
    virtual ~SyslogSink();
    
    /**
     * @brief Initialize syslog sink
     * @param config Configuration JSON
     *        - ident: Syslog identifier (default: "app")
     *        - facility: Syslog facility (default: "user")
     *        - options: Syslog options
     */
    virtual bool init(const Json::Value& config) override;
    
    /**
     * @brief Write log message to syslog
     * @param level Log level
     * @param message Formatted message
     * @param context Additional context
     */
    virtual void write(LogLevel level, const std::string& message,
                      const Json::Value& context = Json::nullValue) override;
    
    /**
     * @brief Flush syslog (no-op for syslog)
     */
    virtual void flush() override {}
    
    /**
     * @brief Set syslog identifier
     */
    void setIdent(const std::string& ident) { this->ident = ident; }
    
    /**
     * @brief Set syslog facility
     */
    void setFacility(int facility) { this->facility = facility; }
    
    /**
     * @brief Set syslog options
     */
    void setOptions(int options) { this->options = options; }

private:
    std::string ident = "app";
    int facility = 1;  // LOG_USER
    int options = 0;   // LOG_NDELAY | LOG_PID
    bool initialized = false;
    std::mutex mutex;
    
    void openLog();
    void closeLog();
};

} // namespace common

#endif // SYSLOG_SINK_H
