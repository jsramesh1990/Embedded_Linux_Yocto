#include "logger/ConsoleSink.h"
#include <iomanip>
#include <chrono>
#include <ctime>

using namespace common;

ConsoleSink::ConsoleSink(bool useStdErr) : useStdErr(useStdErr) {
    name = "ConsoleSink";
}

bool ConsoleSink::init(const Json::Value& config) {
    if (config.isMember("use_stderr")) {
        useStdErr = config["use_stderr"].asBool();
    }
    if (config.isMember("show_timestamp")) {
        showTimestamp = config["show_timestamp"].asBool();
    }
    if (config.isMember("show_thread")) {
        showThreadId = config["show_thread"].asBool();
    }
    return true;
}

void ConsoleSink::write(LogLevel level, const std::string& message, const Json::Value& context) {
    if (!shouldLog(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    std::ostream& out = useStdErr ? std::cerr : std::cout;
    out << formatMessage(level, message, context);
    
    if (flushInterval > 0) {
        out.flush();
    }
}

void ConsoleSink::flush() {
    std::lock_guard<std::mutex> lock(mutex);
    if (useStdErr) {
        std::cerr.flush();
    } else {
        std::cout.flush();
    }
}

std::string ConsoleSink::formatMessage(LogLevel level, const std::string& message,
                                       const Json::Value& context) {
    std::string result = message;
    
    if (showTimestamp) {
        std::string time = getCurrentTime();
        result = "[" + time + "] " + result;
    }
    
    if (showThreadId) {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        result = "[" + ss.str() + "] " + result;
    }
    
    // Add newline
    result += "\n";
    
    // Add color
    if (colored) {
        result = logLevelColor(level) + result + "\033[0m";
    }
    
    return result;
}

std::string ConsoleSink::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    struct tm tm;
    localtime_r(&time_t, &tm);
    
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
    return std::string(buffer) + "." + std::to_string(ms.count());
}
