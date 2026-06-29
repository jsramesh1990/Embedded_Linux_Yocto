#include "logger/FileSink.h"
#include <iostream>
#include <chrono>

using namespace common;

FileSink::FileSink() {
    name = "FileSink";
    lastFlush = std::chrono::steady_clock::now();
}

FileSink::~FileSink() {
    flush();
    closeFile();
}

bool FileSink::init(const Json::Value& config) {
    if (config.isMember("file_path")) {
        filePath = config["file_path"].asString();
    }
    if (config.isMember("append")) {
        append = config["append"].asBool();
    }
    if (config.isMember("buffer_size")) {
        bufferSize = config["buffer_size"].asUInt64();
    }
    if (config.isMember("flush_interval")) {
        flushInterval = config["flush_interval"].asInt();
    }
    
    if (!filePath.empty()) {
        return openFile();
    }
    
    return false;
}

bool FileSink::openFile() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (file.is_open()) {
        return true;
    }
    
    auto openMode = append ? std::ios::app : std::ios::trunc;
    file.open(filePath, std::ios::out | openMode);
    
    if (!file.is_open()) {
        std::cerr << "FileSink: Failed to open file: " << filePath << std::endl;
        return false;
    }
    
    // Set buffer size
    if (bufferSize > 0) {
        char* buffer = new char[bufferSize];
        file.rdbuf()->pubsetbuf(buffer, bufferSize);
    }
    
    return true;
}

void FileSink::closeFile() {
    std::lock_guard<std::mutex> lock(mutex);
    if (file.is_open()) {
        file.flush();
        file.close();
    }
}

bool FileSink::ensureFileOpen() {
    if (!file.is_open()) {
        return openFile();
    }
    return true;
}

void FileSink::write(LogLevel level, const std::string& message, const Json::Value& context) {
    if (!shouldLog(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!ensureFileOpen()) {
        return;
    }
    
    file << message << std::endl;
    
    autoFlush();
}

void FileSink::flush() {
    std::lock_guard<std::mutex> lock(mutex);
    if (file.is_open()) {
        file.flush();
    }
}

void FileSink::autoFlush() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFlush);
    
    if (elapsed.count() >= flushInterval) {
        file.flush();
        lastFlush = now;
    }
}
