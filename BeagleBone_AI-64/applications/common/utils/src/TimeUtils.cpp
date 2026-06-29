#include "utils/TimeUtils.h"
#include <thread>
#include <iomanip>
#include <regex>
#include <cmath>

using namespace common::utils;

std::string TimeUtils::currentTime(const std::string& format) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    struct tm tm;
    localtime_r(&time_t, &tm);
    
    char buffer[128];
    strftime(buffer, sizeof(buffer), format.c_str(), &tm);
    return std::string(buffer);
}

int64_t TimeUtils::currentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
}

int64_t TimeUtils::currentTimestampUs() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
}

int64_t TimeUtils::currentTimestampNs() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
}

std::string TimeUtils::timestampToString(int64_t timestamp, const std::string& format) {
    time_t time_t = timestamp / 1000;
    struct tm tm;
    localtime_r(&time_t, &tm);
    
    char buffer[128];
    strftime(buffer, sizeof(buffer), format.c_str(), &tm);
    return std::string(buffer);
}

int64_t TimeUtils::stringToTimestamp(const std::string& timeStr, const std::string& format) {
    struct tm tm = {};
    strptime(timeStr.c_str(), format.c_str(), &tm);
    time_t time_t = mktime(&tm);
    return static_cast<int64_t>(time_t) * 1000;
}

int64_t TimeUtils::parseDuration(const std::string& duration) {
    std::regex pattern(R"((\d+)(ms|s|m|h|d))");
    std::smatch match;
    
    if (!std::regex_match(duration, match, pattern)) {
        return 0;
    }
    
    int64_t value = std::stoll(match[1]);
    std::string unit = match[2];
    
    if (unit == "ms") return value;
    if (unit == "s") return value * 1000;
    if (unit == "m") return value * 60 * 1000;
    if (unit == "h") return value * 60 * 60 * 1000;
    if (unit == "d") return value * 24 * 60 * 60 * 1000;
    
    return 0;
}

std::string TimeUtils::formatDuration(int64_t milliseconds) {
    if (milliseconds < 1000) {
        return std::to_string(milliseconds) + "ms";
    }
    
    int64_t seconds = milliseconds / 1000;
    int64_t minutes = seconds / 60;
    int64_t hours = minutes / 60;
    int64_t days = hours / 24;
    
    if (days > 0) {
        return std::to_string(days) + "d " + 
               std::to_string(hours % 24) + "h " +
               std::to_string(minutes % 60) + "m";
    }
    
    if (hours > 0) {
        return std::to_string(hours) + "h " +
               std::to_string(minutes % 60) + "m";
    }
    
    if (minutes > 0) {
        return std::to_string(minutes) + "m " +
               std::to_string(seconds % 60) + "s";
    }
    
    return std::to_string(seconds) + "." +
           std::to_string(milliseconds % 1000) + "s";
}

void TimeUtils::sleep(int64_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

TimeUtils::Stopwatch::Stopwatch() : running(false) {}

void TimeUtils::Stopwatch::start() {
    startTime = std::chrono::steady_clock::now();
    running = true;
}

void TimeUtils::Stopwatch::stop() {
    if (running) {
        stopTime = std::chrono::steady_clock::now();
        running = false;
    }
}

void TimeUtils::Stopwatch::reset() {
    running = false;
}

int64_t TimeUtils::Stopwatch::elapsedMs() const {
    auto end = running ? std::chrono::steady_clock::now() : stopTime;
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime).count();
}

int64_t TimeUtils::Stopwatch::elapsedUs() const {
    auto end = running ? std::chrono::steady_clock::now() : stopTime;
    return std::chrono::duration_cast<std::chrono::microseconds>(end - startTime).count();
}

int64_t TimeUtils::Stopwatch::elapsedNs() const {
    auto end = running ? std::chrono::steady_clock::now() : stopTime;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - startTime).count();
}

std::string TimeUtils::Stopwatch::toString() const {
    return formatDuration(elapsedMs());
}

TimeUtils::Timer::Timer() : running(false) {}

TimeUtils::Timer::~Timer() {
    stop();
}

void TimeUtils::Timer::start(int64_t intervalMs, std::function<void()> callback) {
    if (running) {
        stop();
    }
    
    this->intervalMs = intervalMs;
    this->callback = callback;
    running = true;
    
    timerThread = std::thread([this]() {
        while (running) {
            std::unique_lock<std::mutex> lock(mutex);
            if (cv.wait_for(lock, std::chrono::milliseconds(intervalMs), 
                           [this]() { return !running; })) {
                break;
            }
            
            if (running && callback) {
                callback();
            }
        }
    });
}

void TimeUtils::Timer::stop() {
    running = false;
    cv.notify_all();
    if (timerThread.joinable()) {
        timerThread.join();
    }
}

bool TimeUtils::Timer::isRunning() const {
    return running;
}

std::string TimeUtils::toISO8601(std::chrono::system_clock::time_point time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        time.time_since_epoch()) % 1000;
    
    struct tm tm;
    gmtime_r(&time_t, &tm);
    
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);
    return std::string(buffer) + "." + padNumber(ms.count(), 3) + "Z";
}

std::chrono::system_clock::time_point TimeUtils::fromISO8601(const std::string& str) {
    struct tm tm = {};
    int ms = 0;
    
    // Parse format: YYYY-MM-DDTHH:MM:SS.sssZ
    std::regex pattern(R"((\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})\.?(\d{3})?Z)");
    std::smatch match;
    
    if (std::regex_match(str, match, pattern)) {
        tm.tm_year = std::stoi(match[1]) - 1900;
        tm.tm_mon = std::stoi(match[2]) - 1;
        tm.tm_mday = std::stoi(match[3]);
        tm.tm_hour = std::stoi(match[4]);
        tm.tm_min = std::stoi(match[5]);
        tm.tm_sec = std::stoi(match[6]);
        if (match[7].matched) {
            ms = std::stoi(match[7]);
        }
    }
    
    time_t time_t = timegm(&tm);
    auto duration = std::chrono::seconds(time_t) + std::chrono::milliseconds(ms);
    return std::chrono::system_clock::time_point(duration);
}

int TimeUtils::getTimezoneOffset() {
    time_t now = time(nullptr);
    struct tm local, gmt;
    localtime_r(&now, &local);
    gmtime_r(&now, &gmt);
    
    return (local.tm_hour - gmt.tm_hour) * 60 + (local.tm_min - gmt.tm_min);
}

std::string TimeUtils::padNumber(int number, int width) {
    std::string result = std::to_string(number);
    if (result.length() < static_cast<size_t>(width)) {
        result = std::string(width - result.length(), '0') + result;
    }
    return result;
}
