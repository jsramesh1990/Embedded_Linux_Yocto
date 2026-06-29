#include "utils/CircularBuffer.h"
#include <algorithm>

using namespace common::utils;

template<typename T>
CircularBuffer<T>::CircularBuffer(size_t cap) 
    : capacity(cap), head(0), tail(0), size(0) {
    if (capacity == 0) {
        capacity = 1;
    }
    buffer.resize(capacity);
}

template<typename T>
CircularBuffer<T>::~CircularBuffer() {}

template<typename T>
bool CircularBuffer<T>::push(const T& element) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (size >= capacity) {
        return false;
    }
    
    buffer[tail] = element;
    tail = (tail + 1) % capacity;
    size++;
    return true;
}

template<typename T>
bool CircularBuffer<T>::push(T&& element) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (size >= capacity) {
        return false;
    }
    
    buffer[tail] = std::move(element);
    tail = (tail + 1) % capacity;
    size++;
    return true;
}

template<typename T>
bool CircularBuffer<T>::pop(T& element) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (size == 0) {
        return false;
    }
    
    element = std::move(buffer[head]);
    head = (head + 1) % capacity;
    size--;
    return true;
}

template<typename T>
bool CircularBuffer<T>::peek(T& element) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (size == 0) {
        return false;
    }
    
    element = buffer[head];
    return true;
}

template<typename T>
bool CircularBuffer<T>::peekAt(size_t index, T& element) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (index >= size) {
        return false;
    }
    
    size_t pos = (head + index) % capacity;
    element = buffer[pos];
    return true;
}

template<typename T>
void CircularBuffer<T>::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    head = 0;
    tail = 0;
    size = 0;
}

template<typename T>
void CircularBuffer<T>::reserve(size_t newCapacity) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (newCapacity <= capacity) {
        return;
    }
    
    std::vector<T> newBuffer(newCapacity);
    size_t count = 0;
    
    // Copy existing elements
    for (size_t i = 0; i < size; ++i) {
        size_t pos = (head + i) % capacity;
        newBuffer[count++] = std::move(buffer[pos]);
    }
    
    buffer = std::move(newBuffer);
    capacity = newCapacity;
    head = 0;
    tail = size;
}

template<typename T>
std::vector<T> CircularBuffer<T>::toVector() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<T> result;
    result.reserve(size);
    
    for (size_t i = 0; i < size; ++i) {
        size_t pos = (head + i) % capacity;
        result.push_back(buffer[pos]);
    }
    
    return result;
}

template<typename T>
size_t CircularBuffer<T>::pushBulk(const std::vector<T>& elements) {
    std::lock_guard<std::mutex> lock(mutex);
    
    size_t pushed = 0;
    for (const auto& element : elements) {
        if (size >= capacity) {
            break;
        }
        buffer[tail] = element;
        tail = (tail + 1) % capacity;
        size++;
        pushed++;
    }
    
    return pushed;
}

template<typename T>
size_t CircularBuffer<T>::popBulk(size_t count, std::vector<T>& elements) {
    std::lock_guard<std::mutex> lock(mutex);
    
    size_t popped = 0;
    while (popped < count && size > 0) {
        elements.push_back(std::move(buffer[head]));
        head = (head + 1) % capacity;
        size--;
        popped++;
    }
    
    return popped;
}

// Explicit template instantiations for common types
template class CircularBuffer<int>;
template class CircularBuffer<std::string>;
template class CircularBuffer<void*>;
