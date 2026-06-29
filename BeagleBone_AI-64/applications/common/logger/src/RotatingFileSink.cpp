#include "logger/RotatingFileSink.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <zlib.h>

using namespace common;

RotatingFileSink::RotatingFileSink() {
    name = "RotatingFileSink";
    lastRotation = std::chrono::steady_clock::now();
}

RotatingFileSink::~RotatingFileSink() {
    flush();
}

bool RotatingFileSink::init(const Json::Value& config) {
    if (config.isMember("file_path")) {
        basePath = config["file_path"].asString();
        filePath = basePath;
    }
    
    if (config.isMember("max_size")) {
        maxSize = config["max_size"].asUInt64();
    }
    
    if (config.isMember("max_files")) {
        maxFiles = config["max_files"].asInt();
    }
    
    if (config.isMember("rotation_time")) {
        rotationTime = config["rotation_time"].asInt();
    }
    
    if (config.isMember("rotation_policy")) {
        std::string policy = config["rotation_policy"].asString();
        if (policy == "size") rotationPolicy = RotationPolicy::SIZE;
        else if (policy == "time") rotationPolicy = RotationPolicy::TIME;
        else if (policy == "size_time") rotationPolicy = RotationPolicy::SIZE_AND_TIME;
        else if (policy == "manual") rotationPolicy = RotationPolicy::MANUAL;
    }
    
    if (config.isMember("compress")) {
        compress = config["compress"].asBool();
    }
    
    // Determine starting index
    for (int i = maxFiles - 1; i >= 0; --i) {
        std::string testPath = getFileName(i);
        if (std::filesystem::exists(testPath)) {
            currentIndex = i + 1;
            break;
        }
    }
    
    if (currentIndex >= maxFiles) {
        currentIndex = maxFiles - 1;
    }
    
    filePath = getFileName(currentIndex);
    return FileSink::init(config);
}

bool RotatingFileSink::rotate() {
    std::lock_guard<std::mutex> lock(rotationMutex);
    
    // Close current file
    flush();
    closeFile();
    
    // Rotate files
    performRotation();
    
    // Open new file
    filePath = getFileName(0);
    return openFile();
}

bool RotatingFileSink::shouldRotate() {
    if (rotationPolicy == RotationPolicy::MANUAL) {
        return false;
    }
    
    bool sizeExceeded = false;
    bool timeExceeded = false;
    
    // Check file size
    if (rotationPolicy == RotationPolicy::SIZE || 
        rotationPolicy == RotationPolicy::SIZE_AND_TIME) {
        if (std::filesystem::exists(filePath)) {
            auto size = std::filesystem::file_size(filePath);
            sizeExceeded = size >= maxSize;
        }
    }
    
    // Check time
    if (rotationPolicy == RotationPolicy::TIME || 
        rotationPolicy == RotationPolicy::SIZE_AND_TIME) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRotation);
        timeExceeded = elapsed.count() >= rotationTime;
    }
    
    if (rotationPolicy == RotationPolicy::SIZE) {
        return sizeExceeded;
    } else if (rotationPolicy == RotationPolicy::TIME) {
        return timeExceeded;
    } else {
        return sizeExceeded || timeExceeded;
    }
}

void RotatingFileSink::performRotation() {
    // Move existing files
    for (int i = maxFiles - 1; i > 0; --i) {
        std::string oldPath = getFileName(i - 1);
        std::string newPath = getFileName(i);
        
        if (std::filesystem::exists(oldPath)) {
            try {
                std::filesystem::rename(oldPath, newPath);
                // Compress if enabled
                if (compress && i == maxFiles - 1) {
                    compressFile(newPath);
                }
            } catch (const std::exception& e) {
                std::cerr << "RotatingFileSink: Failed to rename file: " << e.what() << std::endl;
            }
        }
    }
    
    // Clean up old files
    cleanupOldFiles();
    
    currentIndex = 0;
    lastRotation = std::chrono::steady_clock::now();
}

void RotatingFileSink::cleanupOldFiles() {
    // Remove files beyond maxFiles
    for (int i = maxFiles; i < maxFiles * 2; ++i) {
        std::string path = getFileName(i);
        if (std::filesystem::exists(path)) {
            try {
                std::filesystem::remove(path);
            } catch (...) {}
        }
        
        // Check compressed files
        std::string compressedPath = path + ".gz";
        if (std::filesystem::exists(compressedPath)) {
            try {
                std::filesystem::remove(compressedPath);
            } catch (...) {}
        }
    }
}

std::string RotatingFileSink::getFileName(int index) const {
    if (index == 0) {
        return basePath;
    }
    
    std::string ext = "";
    size_t extPos = basePath.find_last_of('.');
    if (extPos != std::string::npos) {
        ext = basePath.substr(extPos);
        std::string base = basePath.substr(0, extPos);
        return base + "." + std::to_string(index) + ext;
    }
    
    return basePath + "." + std::to_string(index);
}

std::string RotatingFileSink::getCurrentFileName() const {
    return filePath;
}

void RotatingFileSink::write(LogLevel level, const std::string& message, 
                             const Json::Value& context) {
    if (!shouldLog(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Check if rotation is needed
    if (shouldRotate()) {
        lock.unlock();
        rotate();
        lock.lock();
    }
    
    if (!ensureFileOpen()) {
        return;
    }
    
    file << message << std::endl;
    
    autoFlush();
}

void RotatingFileSink::flush() {
    std::lock_guard<std::mutex> lock(mutex);
    if (file.is_open()) {
        file.flush();
    }
}

void RotatingFileSink::compressFile(const std::string& filePath) {
#ifdef HAVE_ZLIB
    // Open the file to compress
    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open()) {
        return;
    }
    
    std::string compressedPath = filePath + ".gz";
    gzFile output = gzopen(compressedPath.c_str(), "wb");
    if (!output) {
        return;
    }
    
    // Read and compress
    char buffer[8192];
    while (input.read(buffer, sizeof(buffer))) {
        gzwrite(output, buffer, input.gcount());
    }
    
    gzclose(output);
    input.close();
    
    // Remove original file
    std::filesystem::remove(filePath);
#endif
}
