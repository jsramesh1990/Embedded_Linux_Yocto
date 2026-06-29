#include "logger/Logger.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <unistd.h>
#include <sys/time.h>
#include <yaml-cpp/yaml.h>

using namespace common;

// Static member initialization
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    stats.startTime = std::chrono::steady_clock::now();
    // Initialize level counters
    for (int i = 0; i <= static_cast<int>(LogLevel::OFF); ++i) {
        stats.levelCounts[static_cast<LogLevel>(i)] = 0;
    }
}

Logger::~Logger() {
    flush();
}

bool Logger::initialize(const std::string& configFile) {
    if (initialized) {
        return true;
    }
    
    if (!configFile.empty()) {
        loadConfigFromFile(configFile);
    }
    
    // If no sinks, add a default console sink
    if (sinks.empty()) {
        auto consoleSink = std::make_unique<ConsoleSink>();
        consoleSink->setLevel(currentLevel);
        consoleSink->setColored(colored);
        consoleSink->setFormat(format);
        addSink(std::move(consoleSink));
    }
    
    initialized = true;
    return true;
}

bool Logger::initialize(const Config& config) {
    if (initialized) {
        return true;
    }
    
    currentLevel = config.level;
    format = config.format;
    colored = config.colored;
    includeThreadId = config.includeThreadId;
    includeFileInfo = config.includeFileInfo;
    includeSessionId = config.includeSessionId;
    
    for (auto& sink : config.sinks) {
        addSink(std::move(sink));
    }
    
    initialized = true;
    return true;
}

void Logger::addSink(std::unique_ptr<LogSink> sink) {
    std::lock_guard<std::mutex> lock(sinksMutex);
    sink->setLevel(currentLevel);
    sink->setFormat(format);
    sink->setColored(colored);
    sinks.push_back(std::move(sink));
}

void Logger::removeSinks(const std::string& type) {
    std::lock_guard<std::mutex> lock(sinksMutex);
    sinks.erase(
        std::remove_if(sinks.begin(), sinks.end(),
            [&type](const std::unique_ptr<LogSink>& sink) {
                return sink->getName() == type;
            }),
        sinks.end()
    );
}

void Logger::clearSinks() {
    std::lock_guard<std::mutex> lock(sinksMutex);
    sinks.clear();
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(sinksMutex);
    currentLevel = level;
    for (auto& sink : sinks) {
        sink->setLevel(level);
    }
}

void Logger::log(LogLevel level, const std::string& message, const Json::Value& context) {
    if (!shouldLog(level)) {
        return;
    }
    
    LogEvent event;
    event.level = level;
    event.message = message;
    event.context = context;
    event.timestamp = std::chrono::system_clock::now();
    event.threadId = getThreadId();
    event.sessionId = sessionId;
    event.userId = userId;
    event.sequence = ++stats.totalLogs;
    
    // Add default context if not provided
    if (context.isNull()) {
        event.context = createDefaultContext();
    }
    
    logEvent(event);
}

void Logger::logEvent(const LogEvent& event) {
    incrementStats(event.level);
    
    // Call log callback if set
    if (logCallback) {
        try {
            logCallback(event);
        } catch (...) {}
    }
    
    // Format message
    std::string formatted = formatMessage(event);
    
    // Write to all sinks
    std::lock_guard<std::mutex> lock(sinksMutex);
    for (auto& sink : sinks) {
        if (sink->shouldLog(event.level)) {
            try {
                sink->write(event.level, formatted, event.context);
            } catch (const std::exception& e) {
                std::cerr << "Logger sink error: " << e.what() << std::endl;
                stats.errors++;
            }
        }
    }
}

void Logger::logWithLocation(LogLevel level, const std::string& message,
                            const char* file, int line, const char* function,
                            const Json::Value& context) {
    if (!shouldLog(level)) {
        return;
    }
    
    LogEvent event;
    event.level = level;
    event.message = message;
    event.context = context;
    event.timestamp = std::chrono::system_clock::now();
    event.threadId = getThreadId();
    event.file = file;
    event.line = line;
    event.function = function;
    event.sessionId = sessionId;
    event.userId = userId;
    event.sequence = ++stats.totalLogs;
    
    if (context.isNull()) {
        event.context = createDefaultContext();
        if (includeFileInfo) {
            event.context["file"] = file;
            event.context["line"] = line;
            event.context["function"] = function;
        }
    }
    
    logEvent(event);
}

bool Logger::shouldLog(LogLevel level) const {
    return level >= currentLevel && level != LogLevel::OFF;
}

std::string Logger::formatMessage(const LogEvent& event) {
    std::string result = format;
    
    // Replace placeholders
    size_t pos;
    std::string timeStr = getCurrentTime();
    std::string levelStr = logLevelToString(event.level);
    std::string coloredLevel = colored ? logLevelColor(event.level) + levelStr + "\033[0m" : levelStr;
    
    // %time - Timestamp
    pos = result.find("%time");
    while (pos != std::string::npos) {
        result.replace(pos, 5, timeStr);
        pos = result.find("%time", pos + timeStr.length());
    }
    
    // %level - Log level
    pos = result.find("%level");
    while (pos != std::string::npos) {
        result.replace(pos, 6, coloredLevel);
        pos = result.find("%level", pos + coloredLevel.length());
    }
    
    // %message - Message
    pos = result.find("%message");
    while (pos != std::string::npos) {
        result.replace(pos, 8, event.message);
        pos = result.find("%message", pos + event.message.length());
    }
    
    // %thread - Thread ID
    if (includeThreadId) {
        pos = result.find("%thread");
        while (pos != std::string::npos) {
            result.replace(pos, 7, event.threadId);
            pos = result.find("%thread", pos + event.threadId.length());
        }
    }
    
    // %session - Session ID
    if (includeSessionId && !sessionId.empty()) {
        pos = result.find("%session");
        while (pos != std::string::npos) {
            result.replace(pos, 8, sessionId);
            pos = result.find("%session", pos + sessionId.length());
        }
    }
    
    // %file - File name
    if (includeFileInfo && !event.file.empty()) {
        pos = result.find("%file");
        while (pos != std::string::npos) {
            std::string file = event.file;
            size_t lastSlash = file.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                file = file.substr(lastSlash + 1);
            }
            result.replace(pos, 5, file);
            pos = result.find("%file", pos + file.length());
        }
        
        pos = result.find("%line");
        while (pos != std::string::npos) {
            std::string line = std::to_string(event.line);
            result.replace(pos, 5, line);
            pos = result.find("%line", pos + line.length());
        }
        
        pos = result.find("%function");
        while (pos != std::string::npos) {
            result.replace(pos, 9, event.function);
            pos = result.find("%function", pos + event.function.length());
        }
    }
    
    // %context - JSON context
    if (!event.context.isNull()) {
        pos = result.find("%context");
        while (pos != std::string::npos) {
            Json::StreamWriterBuilder builder;
            builder.settings_["indentation"] = "";
            std::string contextStr = Json::writeString(builder, event.context);
            result.replace(pos, 8, contextStr);
            pos = result.find("%context", pos + contextStr.length());
        }
    }
    
    return result;
}

std::string Logger::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    struct tm tm;
    localtime_r(&time_t, &tm);
    
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer) + "." + std::to_string(ms.count());
}

std::string Logger::getThreadId() const {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

void Logger::incrementStats(LogLevel level) {
    stats.totalLogs++;
    stats.levelCounts[level]++;
}

Json::Value Logger::createDefaultContext() {
    Json::Value context;
    context["timestamp"] = getCurrentTime();
    context["thread"] = getThreadId();
    if (!sessionId.empty()) {
        context["session"] = sessionId;
    }
    if (!userId.empty()) {
        context["user"] = userId;
    }
    return context;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(sinksMutex);
    for (auto& sink : sinks) {
        try {
            sink->flush();
        } catch (...) {}
    }
}

Json::Value Logger::getStats() const {
    Json::Value statsJson;
    statsJson["total_logs"] = stats.totalLogs.load();
    statsJson["errors"] = stats.errors.load();
    statsJson["warnings"] = stats.warnings.load();
    
    for (const auto& [level, count] : stats.levelCounts) {
        statsJson["levels"][logLevelToString(level)] = count.load();
    }
    
    auto now = std::chrono::steady_clock::now();
    statsJson["uptime_seconds"] = std::chrono::duration_cast<std::chrono::seconds>(
        now - stats.startTime).count();
    
    return statsJson;
}

void Logger::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats.totalLogs = 0;
    stats.errors = 0;
    stats.warnings = 0;
    for (auto& [level, count] : stats.levelCounts) {
        count = 0;
    }
    stats.startTime = std::chrono::steady_clock::now();
}

void Logger::setLogCallback(std::function<void(const LogEvent&)> callback) {
    logCallback = callback;
}

void Logger::loadConfigFromFile(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "Logger: Failed to open config file: " << configFile << std::endl;
        return;
    }
    
    // Detect file extension
    std::string ext = configFile.substr(configFile.find_last_of(".") + 1);
    
    try {
        if (ext == "json") {
            Json::Value config;
            Json::CharReaderBuilder builder;
            std::string errs;
            if (Json::parseFromStream(builder, file, &config, &errs)) {
                loadConfigFromJson(config);
            } else {
                std::cerr << "Logger: Failed to parse JSON config: " << errs << std::endl;
            }
        } else if (ext == "yaml" || ext == "yml") {
            YAML::Node yaml = YAML::LoadFile(configFile);
            // Convert YAML to JSON and load
            Json::Value config;
            // Simple conversion for now
            if (yaml["level"]) {
                config["level"] = yaml["level"].as<std::string>();
            }
            if (yaml["format"]) {
                config["format"] = yaml["format"].as<std::string>();
            }
            if (yaml["colored"]) {
                config["colored"] = yaml["colored"].as<bool>();
            }
            if (yaml["sinks"] && yaml["sinks"].IsSequence()) {
                for (const auto& sinkYaml : yaml["sinks"]) {
                    Json::Value sinkJson;
                    if (sinkYaml["type"]) {
                        sinkJson["type"] = sinkYaml["type"].as<std::string>();
                    }
                    if (sinkYaml["config"]) {
                        for (const auto& item : sinkYaml["config"]) {
                            sinkJson["config"][item.first.as<std::string>()] = 
                                item.second.as<std::string>();
                        }
                    }
                    config["sinks"].append(sinkJson);
                }
            }
            loadConfigFromJson(config);
        } else {
            std::cerr << "Logger: Unsupported config format: " << ext << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Logger: Error loading config: " << e.what() << std::endl;
    }
}

void Logger::loadConfigFromJson(const Json::Value& config) {
    if (config.isMember("level")) {
        currentLevel = stringToLogLevel(config["level"].asString());
    }
    
    if (config.isMember("format")) {
        format = config["format"].asString();
    }
    
    if (config.isMember("colored")) {
        colored = config["colored"].asBool();
    }
    
    if (config.isMember("include_thread")) {
        includeThreadId = config["include_thread"].asBool();
    }
    
    if (config.isMember("include_file")) {
        includeFileInfo = config["include_file"].asBool();
    }
    
    if (config.isMember("include_session")) {
        includeSessionId = config["include_session"].asBool();
    }
    
    if (config.isMember("session_id")) {
        sessionId = config["session_id"].asString();
    }
    
    if (config.isMember("user_id")) {
        userId = config["user_id"].asString();
    }
    
    // Configure sinks
    if (config.isMember("sinks") && config["sinks"].isArray()) {
        for (const auto& sinkConfig : config["sinks"]) {
            if (!sinkConfig.isMember("type")) {
                continue;
            }
            
            std::string type = sinkConfig["type"].asString();
            Json::Value sinkParams = sinkConfig.isMember("config") ? 
                                     sinkConfig["config"] : Json::nullValue;
            
            LogSink* sink = createSink(type, sinkParams);
            if (sink) {
                sink->setLevel(currentLevel);
                sink->setFormat(format);
                sink->setColored(colored);
                addSink(std::unique_ptr<LogSink>(sink));
            }
        }
    }
}

LogSink* Logger::createSink(const std::string& type, const Json::Value& config) {
    if (type == "console" || type == "ConsoleSink") {
        auto sink = new ConsoleSink();
        if (config.isMember("use_stderr")) {
            sink->setUseStdErr(config["use_stderr"].asBool());
        }
        if (config.isMember("show_timestamp")) {
            sink->setShowTimestamp(config["show_timestamp"].asBool());
        }
        if (config.isMember("show_thread")) {
            sink->setShowThreadId(config["show_thread"].asBool());
        }
        sink->init(config);
        return sink;
    } else if (type == "file" || type == "FileSink") {
        auto sink = new FileSink();
        if (config.isMember("file_path")) {
            sink->setFilePath(config["file_path"].asString());
        }
        if (config.isMember("append")) {
            sink->setAppend(config["append"].asBool());
        }
        if (config.isMember("buffer_size")) {
            sink->setBufferSize(config["buffer_size"].asUInt64());
        }
        if (config.isMember("flush_interval")) {
            sink->setFlushInterval(config["flush_interval"].asInt());
        }
        sink->init(config);
        return sink;
    } else if (type == "rotating_file" || type == "RotatingFileSink") {
        auto sink = new RotatingFileSink();
        if (config.isMember("file_path")) {
            sink->setFilePath(config["file_path"].asString());
        }
        if (config.isMember("max_size")) {
            sink->setMaxSize(config["max_size"].asUInt64());
        }
        if (config.isMember("max_files")) {
            sink->setMaxFiles(config["max_files"].asInt());
        }
        if (config.isMember("compress")) {
            sink->setCompress(config["compress"].asBool());
        }
        sink->init(config);
        return sink;
    } else if (type == "syslog" || type == "SyslogSink") {
        auto sink = new SyslogSink();
        if (config.isMember("ident")) {
            sink->setIdent(config["ident"].asString());
        }
        sink->init(config);
        return sink;
    } else if (type == "network" || type == "NetworkSink") {
        auto sink = new NetworkSink();
        if (config.isMember("host")) {
            sink->setHost(config["host"].asString());
        }
        if (config.isMember("port")) {
            sink->setPort(config["port"].asInt());
        }
        if (config.isMember("batch")) {
            sink->setBatch(config["batch"].asBool());
        }
        if (config.isMember("batch_size")) {
            sink->setBatchSize(config["batch_size"].asUInt64());
        }
        sink->init(config);
        return sink;
    }
    
    return nullptr;
}
