#ifndef FILE_SINK_H
#define FILE_SINK_H

#include "LogSink.h"
#include <fstream>
#include <mutex>
#include <memory>

namespace common {

/**
 * @brief File log sink
 * 
 * Writes log messages to a file with optional rotation
 */
class FileSink : public LogSink {
public:
    FileSink();
    virtual ~FileSink();
    
    /**
     * @brief Initialize file sink
     * @param config Configuration JSON
     *        - file_path: Path to log file
     *        - append: Append to existing file (default: true)
     *        - buffer_size: Buffer size in bytes
     *        - flush_interval: Auto-flush interval in milliseconds
     */
    virtual bool init(const Json::Value& config) override;
    
    /**
     * @brief Write log message to file
     * @param level Log level
     * @param message Formatted message
     * @param context Additional context
     */
    virtual void write(LogLevel level, const std::string& message,
                      const Json::Value& context = Json::nullValue) override;
    
    /**
     * @brief Flush file buffer
     */
    virtual void flush() override;
    
    /**
     * @brief Set file path
     */
    void setFilePath(const std::string& path) { filePath = path; }
    
    /**
     * @brief Set append mode
     */
    void setAppend(bool append) { this->append = append; }
    
    /**
     * @brief Set buffer size
     */
    void setBufferSize(size_t size) { bufferSize = size; }
    
    /**
     * @brief Set flush interval in milliseconds
     */
    void setFlushInterval(int ms) { flushInterval = ms; }
    
    /**
     * @brief Check if file is open
     */
    bool isOpen() const { return file.is_open(); }

protected:
    std::string filePath;
    bool append = true;
    size_t bufferSize = 8192;
    int flushInterval = 1000; // milliseconds
    std::ofstream file;
    std::mutex mutex;
    std::chrono::steady_clock::time_point lastFlush;
    
    bool openFile();
    void closeFile();
    bool ensureFileOpen();
    void autoFlush();
};

} // namespace common

#endif // FILE_SINK_H
