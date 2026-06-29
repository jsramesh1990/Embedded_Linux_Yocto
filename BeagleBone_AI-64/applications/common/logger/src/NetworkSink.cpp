#include "logger/NetworkSink.h"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

using namespace common;

NetworkSink::NetworkSink() : socketHandle(nullptr) {
    name = "NetworkSink";
}

NetworkSink::~NetworkSink() {
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
    disconnect();
}

bool NetworkSink::init(const Json::Value& config) {
    if (config.isMember("protocol")) {
        std::string proto = config["protocol"].asString();
        if (proto == "tcp") protocol = Protocol::TCP;
        else if (proto == "udp") protocol = Protocol::UDP;
        else if (proto == "syslog") protocol = Protocol::SYSLOG;
        else if (proto == "http") protocol = Protocol::HTTP;
        else if (proto == "https") protocol = Protocol::HTTPS;
        else if (proto == "websocket") protocol = Protocol::WEBSOCKET;
    }
    
    if (config.isMember("host")) {
        host = config["host"].asString();
    }
    
    if (config.isMember("port")) {
        port = config["port"].asInt();
    }
    
    if (config.isMember("format")) {
        format = config["format"].asString();
    }
    
    if (config.isMember("batch")) {
        batch = config["batch"].asBool();
    }
    
    if (config.isMember("batch_size")) {
        batchSize = config["batch_size"].asUInt64();
    }
    
    if (config.isMember("retry_attempts")) {
        retryAttempts = config["retry_attempts"].asInt();
    }
    
    if (config.isMember("retry_delay")) {
        retryDelay = config["retry_delay"].asInt();
    }
    
    // Connect to server
    if (!connect()) {
        std::cerr << "NetworkSink: Failed to connect to " << host << ":" << port << std::endl;
        return false;
    }
    
    // Start worker thread for batching
    if (batch) {
        running = true;
        workerThread = std::thread(&NetworkSink::processQueue, this);
    }
    
    return true;
}

bool NetworkSink::connect() {
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = (protocol == Protocol::UDP) ? SOCK_DGRAM : SOCK_STREAM;
    
    std::string portStr = std::to_string(port);
    int status = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (status != 0) {
        std::cerr << "NetworkSink: getaddrinfo error: " << gai_strerror(status) << std::endl;
        return false;
    }
    
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        socketHandle = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socketHandle < 0) {
            continue;
        }
        
        if (::connect((int)(intptr_t)socketHandle, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        
        close((int)(intptr_t)socketHandle);
        socketHandle = nullptr;
    }
    
    freeaddrinfo(result);
    
    if (socketHandle == nullptr) {
        return false;
    }
    
    connected = true;
    return true;
}

void NetworkSink::disconnect() {
    if (socketHandle) {
        close((int)(intptr_t)socketHandle);
        socketHandle = nullptr;
    }
    connected = false;
}

void NetworkSink::write(LogLevel level, const std::string& message, const Json::Value& context) {
    if (!shouldLog(level)) {
        return;
    }
    
    std::string formatted = formatMessage(message, context);
    
    if (batch) {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.size() < batchSize * 2) {
            queue.push(formatted);
        }
    } else {
        sendData(formatted);
    }
}

void NetworkSink::sendData(const std::string& data) {
    if (!connected && !connect()) {
        std::cerr << "NetworkSink: Not connected, dropping message" << std::endl;
        return;
    }
    
    bool sent = false;
    for (int attempt = 0; attempt < retryAttempts; ++attempt) {
        ssize_t result = send((int)(intptr_t)socketHandle, data.c_str(), data.length(), 0);
        if (result > 0) {
            sent = true;
            break;
        }
        
        // Reconnect on failure
        disconnect();
        if (connect()) {
            continue;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay));
    }
    
    if (!sent) {
        std::cerr << "NetworkSink: Failed to send data after " << retryAttempts << " attempts" << std::endl;
    }
}

void NetworkSink::processQueue() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::vector<std::string> batchMessages;
        {
            std::lock_guard<std::mutex> lock(mutex);
            while (!queue.empty() && batchMessages.size() < batchSize) {
                batchMessages.push_back(queue.front());
                queue.pop();
            }
        }
        
        if (batchMessages.empty()) {
            continue;
        }
        
        // Send as JSON array
        Json::Value batch;
        for (const auto& msg : batchMessages) {
            batch.append(msg);
        }
        
        Json::StreamWriterBuilder builder;
        builder.settings_["indentation"] = "";
        std::string data = Json::writeString(builder, batch);
        sendData(data);
    }
}

std::string NetworkSink::formatMessage(const std::string& message, const Json::Value& context) {
    if (format == "json") {
        Json::Value json;
        json["message"] = message;
        json["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        json["level"] = logLevelToString(minLevel);
        if (!context.isNull()) {
            json["context"] = context;
        }
        
        Json::StreamWriterBuilder builder;
        builder.settings_["indentation"] = "";
        return Json::writeString(builder, json);
    } else if (format == "syslog") {
        return "<" + std::to_string(logLevelToSyslog(minLevel)) + ">" + message;
    } else {
        return message;
    }
}

void NetworkSink::flush() {
    // Process remaining messages
    if (batch) {
        std::lock_guard<std::mutex> lock(mutex);
        while (!queue.empty()) {
            std::string msg = queue.front();
            queue.pop();
            sendData(msg);
        }
    }
}
