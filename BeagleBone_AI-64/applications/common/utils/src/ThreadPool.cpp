#include "utils/ThreadPool.h"
#include <iostream>
#include <algorithm>

using namespace common::utils;

ThreadPool::ThreadPool(size_t threadCount) 
    : stopped(false), stopNowFlag(false), activeTasks(0), maxQueueSize(SIZE_MAX) {
    
    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) {
            threadCount = 4; // Fallback
        }
    }
    
    for (size_t i = 0; i < threadCount; ++i) {
        workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

template<typename F, typename... Args>
auto ThreadPool::submit(F&& task, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using ReturnType = typename std::result_of<F(Args...)>::type;
    
    auto packaged = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(task), std::forward<Args>(args)...)
    );
    
    std::future<ReturnType> result = packaged->get_future();
    
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        
        if (stopped || stopNowFlag) {
            throw std::runtime_error("ThreadPool is stopped");
        }
        
        if (tasks.size() >= maxQueueSize) {
            throw std::runtime_error("ThreadPool queue is full");
        }
        
        Task task;
        task.priority = 0;
        task.task = [packaged]() { (*packaged)(); };
        task.enqueueTime = std::chrono::steady_clock::now();
        task.timeout = std::chrono::milliseconds::max();
        tasks.push(std::move(task));
    }
    
    condition.notify_one();
    return result;
}

template<typename F, typename... Args>
auto ThreadPool::submitPriority(int priority, F&& task, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using ReturnType = typename std::result_of<F(Args...)>::type;
    
    auto packaged = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(task), std::forward<Args>(args)...)
    );
    
    std::future<ReturnType> result = packaged->get_future();
    
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        
        if (stopped || stopNowFlag) {
            throw std::runtime_error("ThreadPool is stopped");
        }
        
        if (tasks.size() >= maxQueueSize) {
            throw std::runtime_error("ThreadPool queue is full");
        }
        
        Task task;
        task.priority = priority;
        task.task = [packaged]() { (*packaged)(); };
        task.enqueueTime = std::chrono::steady_clock::now();
        task.timeout = std::chrono::milliseconds::max();
        tasks.push(std::move(task));
    }
    
    condition.notify_one();
    return result;
}

template<typename F, typename... Args>
auto ThreadPool::submitWithTimeout(int timeoutMs, F&& task, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using ReturnType = typename std::result_of<F(Args...)>::type;
    
    auto packaged = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(task), std::forward<Args>(args)...)
    );
    
    std::future<ReturnType> result = packaged->get_future();
    
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        
        if (stopped || stopNowFlag) {
            throw std::runtime_error("ThreadPool is stopped");
        }
        
        if (tasks.size() >= maxQueueSize) {
            throw std::runtime_error("ThreadPool queue is full");
        }
        
        Task task;
        task.priority = 0;
        task.task = [packaged]() { (*packaged)(); };
        task.enqueueTime = std::chrono::steady_clock::now();
        task.timeout = std::chrono::milliseconds(timeoutMs);
        tasks.push(std::move(task));
    }
    
    condition.notify_one();
    return result;
}

void ThreadPool::workerLoop() {
    while (!stopped || !tasks.empty()) {
        Task task;
        
        if (popTask(task)) {
            processTask(task);
        }
    }
}

bool ThreadPool::popTask(Task& task) {
    std::unique_lock<std::mutex> lock(queueMutex);
    
    condition.wait(lock, [this]() {
        return !tasks.empty() || stopped || stopNowFlag;
    });
    
    if (stopNowFlag) {
        return false;
    }
    
    if (tasks.empty()) {
        return false;
    }
    
    task = std::move(const_cast<Task&>(tasks.top()));
    tasks.pop();
    return true;
}

void ThreadPool::processTask(Task& task) {
    activeTasks++;
    
    try {
        // Check if task has timed out
        auto now = std::chrono::steady_clock::now();
        if (task.timeout != std::chrono::milliseconds::max()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - task.enqueueTime);
            if (elapsed > task.timeout) {
                // Task timed out - skip execution
                activeTasks--;
                return;
            }
        }
        
        // Execute task
        task.task();
    } catch (const std::exception& e) {
        // Log error but don't propagate
        std::cerr << "ThreadPool task error: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "ThreadPool task unknown error" << std::endl;
    }
    
    activeTasks--;
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(queueMutex);
    condition.wait(lock, [this]() {
        return tasks.empty() && activeTasks == 0;
    });
}

void ThreadPool::stop() {
    stopped = true;
    condition.notify_all();
    
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::stopNow() {
    stopNowFlag = true;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!tasks.empty()) {
            tasks.pop();
        }
    }
    condition.notify_all();
    stop();
}

size_t ThreadPool::getPendingCount() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return tasks.size();
}

size_t ThreadPool::getActiveCount() const {
    return activeTasks.load();
}

// Explicit template instantiations
template std::future<void> ThreadPool::submit(std::function<void()>, ...);
template std::future<int> ThreadPool::submit(std::function<int()>, ...);
template std::future<std::string> ThreadPool::submit(std::function<std::string()>, ...);
