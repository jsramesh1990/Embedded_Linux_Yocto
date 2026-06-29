#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <vector>
#include <mutex>
#include <atomic>
#include <cstddef>

namespace common {
namespace utils {

/**
 * @brief Thread-safe circular buffer
 * @tparam T Type of elements stored
 */
template<typename T>
class CircularBuffer {
public:
    /**
     * @brief Constructor
     * @param capacity Maximum capacity of the buffer
     */
    CircularBuffer(size_t capacity);
    
    /**
     * @brief Destructor
     */
    ~CircularBuffer();
    
    /**
     * @brief Push element to buffer
     * @param element Element to push
     * @return true if element was pushed, false if buffer is full
     */
    bool push(const T& element);
    
    /**
     * @brief Push element with move semantics
     * @param element Element to push
     * @return true if element was pushed, false if buffer is full
     */
    bool push(T&& element);
    
    /**
     * @brief Pop element from buffer
     * @param element Reference to store popped element
     * @return true if element was popped, false if buffer is empty
     */
    bool pop(T& element);
    
    /**
     * @brief Peek at front element without removing
     * @param element Reference to store element
     * @return true if element exists, false if buffer is empty
     */
    bool peek(T& element) const;
    
    /**
     * @brief Peek at element at index
     * @param index Index from front
     * @param element Reference to store element
     * @return true if element exists, false if index out of range
     */
    bool peekAt(size_t index, T& element) const;
    
    /**
     * @brief Check if buffer is empty
     */
    bool empty() const { return size == 0; }
    
    /**
     * @brief Check if buffer is full
     */
    bool full() const { return size >= capacity; }
    
    /**
     * @brief Get current size
     */
    size_t size() const { return size.load(); }
    
    /**
     * @brief Get capacity
     */
    size_t capacity() const { return capacity; }
    
    /**
     * @brief Clear buffer
     */
    void clear();
    
    /**
     * @brief Reserve space
     */
    void reserve(size_t newCapacity);
    
    /**
     * @brief Get all elements as vector
     */
    std::vector<T> toVector() const;
    
    /**
     * @brief Bulk push multiple elements
     * @param elements Vector of elements
     * @return Number of elements pushed
     */
    size_t pushBulk(const std::vector<T>& elements);
    
    /**
     * @brief Bulk pop multiple elements
     * @param count Number of elements to pop
     * @param elements Vector to store popped elements
     * @return Number of elements popped
     */
    size_t popBulk(size_t count, std::vector<T>& elements);

private:
    std::vector<T> buffer;
    size_t capacity;
    std::atomic<size_t> head;  // Read position
    std::atomic<size_t> tail;  // Write position
    std::atomic<size_t> size;
    mutable std::mutex mutex;
};

} // namespace utils
} // namespace common

#endif // CIRCULAR_BUFFER_H
