#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <chrono>

namespace common {
namespace utils {

/**
 * @brief Thread pool for concurrent task execution
 */
class ThreadPool {
public:
    /**
     * @brief Constructor
     * @param threadCount Number of worker threads (0 = auto-detect)
     */
    ThreadPool(size_t threadCount = 0);
    
    /**
     * @brief Destructor
     */
    ~ThreadPool();
    
    /**
     * @brief Submit task for execution
     * @param task Callable task
     * @return std::future for the result
     */
    template<typename F, typename... Args>
    auto submit(F&& task, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    /**
     * @brief Submit task with priority (higher = earlier)
     */
    template<typename F, typename... Args>
    auto submitPriority(int priority, F&& task, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    /**
     * @brief Submit task with timeout
     * @param timeoutMs Timeout in milliseconds
     * @param task Callable task
     * @return std::future for the result
     */
    template<typename F, typename... Args>
    auto submitWithTimeout(int timeoutMs, F&& task, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    /**
     * @brief Wait for all tasks to complete
     */
    void waitAll();
    
    /**
     * @brief Stop the thread pool (waits for all tasks)
     */
    void stop();
    
    /**
     * @brief Stop immediately (cancels pending tasks)
     */
    void stopNow();
    
    /**
     * @brief Check if stopped
     */
    bool isStopped() const { return stopped; }
    
    /**
     * @brief Get number of worker threads
     */
    size_t getThreadCount() const { return workers.size(); }
    
    /**
     * @brief Get number of pending tasks
     */
    size_t getPendingCount() const;
    
    /**
     * @brief Get number of active tasks
     */
    size_t getActiveCount() const;
    
    /**
     * @brief Set maximum queue size
     */
    void setMaxQueueSize(size_t size) { maxQueueSize = size; }
    
    /**
     * @brief Get maximum queue size
     */
    size_t getMaxQueueSize() const { return maxQueueSize; }

private:
    struct Task {
        int priority;
        std::packaged_task<void()> task;
        std::chrono::steady_clock::time_point enqueueTime;
        std::chrono::milliseconds timeout;
        
        bool operator<(const Task& other) const {
            // Higher priority first
            if (priority != other.priority) {
                return priority < other.priority;
            }
            // Older tasks first
            return enqueueTime > other.enqueueTime;
        }
    };
    
    std::vector<std::thread> workers;
    std::priority_queue<Task> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stopped;
    std::atomic<bool> stopNowFlag;
    std::atomic<size_t> activeTasks;
    std::atomic<size_t> maxQueueSize;
    
    void workerLoop();
    bool popTask(Task& task);
    void processTask(Task& task);
};

} // namespace utils
} // namespace common

#endif // THREAD_POOL_H
