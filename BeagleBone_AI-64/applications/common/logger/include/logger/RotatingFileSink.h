#ifndef ROTATING_FILE_SINK_H
#define ROTATING_FILE_SINK_H

#include "FileSink.h"
#include <string>
#include <vector>

namespace common {

/**
 * @brief Rotating file log sink
 * 
 * Extends FileSink with log rotation capabilities
 * Supports size-based and time-based rotation
 */
class RotatingFileSink : public FileSink {
public:
    enum class RotationPolicy {
        SIZE,           // Rotate when file size exceeds limit
        TIME,           // Rotate based on time interval
        SIZE_AND_TIME,  // Rotate when either condition is met
        MANUAL          // Manual rotation
    };
    
    RotatingFileSink();
    virtual ~RotatingFileSink();
    
    /**
     * @brief Initialize rotating file sink
     * @param config Configuration JSON
     *        - file_path: Base path for log files
     *        - max_size: Maximum file size in bytes (default: 10MB)
     *        - max_files: Maximum number of files to keep (default: 5)
     *        - rotation_time: Time interval for rotation (in seconds)
     *        - rotation_policy: "size", "time", "size_time", "manual"
     *        - compress: Compress rotated files (default: false)
     */
    virtual bool init(const Json::Value& config) override;
    
    /**
     * @brief Write log message
     */
    virtual void write(LogLevel level, const std::string& message,
                      const Json::Value& context = Json::nullValue) override;
    
    /**
     * @brief Flush and rotate if needed
     */
    virtual void flush() override;
    
    /**
     * @brief Manually rotate logs
     */
    bool rotate();
    
    /**
     * @brief Set maximum file size
     */
    void setMaxSize(size_t size) { maxSize = size; }
    
    /**
     * @brief Set maximum number of files to keep
     */
    void setMaxFiles(int count) { maxFiles = count; }
    
    /**
     * @brief Set rotation policy
     */
    void setRotationPolicy(RotationPolicy policy) { rotationPolicy = policy; }
    
    /**
     * @brief Set rotation time interval (seconds)
     */
    void setRotationTime(int seconds) { rotationTime = seconds; }
    
    /**
     * @brief Enable/disable compression
     */
    void setCompress(bool compress) { this->compress = compress; }
    
    /**
     * @brief Get current file name
     */
    std::string getCurrentFileName() const;

protected:
    RotationPolicy rotationPolicy = RotationPolicy::SIZE;
    size_t maxSize = 10 * 1024 * 1024;  // 10MB
    int maxFiles = 5;
    int rotationTime = 86400;  // 24 hours
    bool compress = false;
    
    std::string basePath;
    int currentIndex = 0;
    std::chrono::steady_clock::time_point lastRotation;
    std::mutex rotationMutex;
    
    bool shouldRotate();
    void performRotation();
    void cleanupOldFiles();
    std::string getFileName(int index) const;
    std::string getNextFileName() const;
    void compressFile(const std::string& filePath);
};

} // namespace common

#endif // ROTATING_FILE_SINK_H
