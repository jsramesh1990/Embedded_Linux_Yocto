#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <string>
#include <chrono>
#include <ctime>

namespace common {
namespace utils {

/**
 * @brief Time utilities
 */
class TimeUtils {
public:
    /**
     * @brief Get current time as string
     * @param format strftime format (default: "%Y-%m-%d %H:%M:%S")
     */
    static std::string currentTime(const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    /**
     * @brief Get current timestamp in milliseconds
     */
    static int64_t currentTimestampMs();
    
    /**
     * @brief Get current timestamp in microseconds
     */
    static int64_t currentTimestampUs();
    
    /**
     * @brief Get current timestamp in nanoseconds
     */
    static int64_t currentTimestampNs();
    
    /**
     * @brief Convert timestamp to string
     */
    static std::string timestampToString(int64_t timestamp, 
                                        const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    /**
     * @brief Convert string to timestamp
     */
    static int64_t stringToTimestamp(const std::string& timeStr,
                                    const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    /**
     * @brief Parse duration string to milliseconds
     * Supports: "1s", "500ms", "1m", "1h", "1d"
     */
    static int64_t parseDuration(const std::string& duration);
    
    /**
     * @brief Format duration in milliseconds to human-readable string
     */
    static std::string formatDuration(int64_t milliseconds);
    
    /**
     * @brief Sleep for specified duration
     */
    static void sleep(int64_t milliseconds);
    
    /**
     * @brief Get elapsed time since start
     */
    class Stopwatch {
    public:
        Stopwatch();
        void start();
        void stop();
        void reset();
        int64_t elapsedMs() const;
        int64_t elapsedUs() const;
        int64_t elapsedNs() const;
        std::string toString() const;
        
    private:
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point stopTime;
        bool running;
    };
    
    /**
     * @brief Timer for periodic events
     */
    class Timer {
    public:
        Timer();
        ~Timer();
        
        void start(int64_t intervalMs, std::function<void()> callback);
        void stop();
        bool isRunning() const;
        
    private:
        std::function<void()> callback;
        std::thread timerThread;
        std::atomic<bool> running;
        std::atomic<int64_t> intervalMs;
        std::mutex mutex;
        std::condition_variable cv;
    };
    
    /**
     * @brief Convert time to ISO 8601 format
     */
    static std::string toISO8601(std::chrono::system_clock::time_point time);
    
    /**
     * @brief Parse ISO 8601 time string
     */
    static std::chrono::system_clock::time_point fromISO8601(const std::string& str);
    
    /**
     * @brief Get timezone offset in minutes
     */
    static int getTimezoneOffset();

private:
    static std::string padNumber(int number, int width = 2);
};

} // namespace utils
} // namespace common

#endif // TIME_UTILS_H
