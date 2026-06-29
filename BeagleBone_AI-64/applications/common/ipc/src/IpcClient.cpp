#include "ipc/IpcClient.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace common;

IpcClient::IpcClient(const std::string& serverAddress, const std::string& clientId)
    : serverAddress(serverAddress), clientId(clientId.empty() ? generateRequestId() : clientId),
      connection(nullptr) {
    stats.startTime = std::chrono::steady_clock::now();
}

IpcClient::~IpcClient() {
    disconnect();
}

bool IpcClient::connect(int timeoutMs) {
    if (connected) {
        return true;
    }
    
    bool result = connectInternal();
    if (result) {
        connected = true;
        retryCount = 0;
        notifyConnection(true);
        start();
    } else {
        notifyError("Failed to connect to server");
        if (autoReconnect) {
            handleReconnect();
        }
    }
    
    return result;
}

bool IpcClient::connectInternal() {
    // Parse server address
    std::string protocol;
    std::string address;
    size_t pos = serverAddress.find("://");
    if (pos != std::string::npos) {
        protocol = serverAddress.substr(0, pos);
        address = serverAddress.substr(pos + 3);
    } else {
        protocol = "unix";
        address = serverAddress;
    }
    
    if (protocol == "unix") {
        // Unix domain socket
        connection = socket(AF_UNIX, SOCK_STREAM, 0);
        if (!connection) {
            return false;
        }
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, address.c_str(), sizeof(addr.sun_path) - 1);
        
        if (::connect((int)(intptr_t)connection, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close((int)(intptr_t)connection);
            connection = nullptr;
            return false;
        }
        
        return true;
    } else if (protocol == "tcp") {
        // TCP socket
        pos = address.find(":");
        if (pos == std::string::npos) {
            return false;
        }
        
        std::string host = address.substr(0, pos);
        int port = std::stoi(address.substr(pos + 1));
        
        connection = socket(AF_INET, SOCK_STREAM, 0);
        if (!connection) {
            return false;
        }
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        
        struct hostent* server = gethostbyname(host.c_str());
        if (!server) {
            close((int)(intptr_t)connection);
            connection = nullptr;
            return false;
        }
        
        memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
        
        if (::connect((int)(intptr_t)connection, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close((int)(intptr_t)connection);
            connection = nullptr;
            return false;
        }
        
        return true;
    }
    
    return false;
}

void IpcClient::disconnect() {
    if (!connected) return;
    
    connected = false;
    running = false;
    
    if (workerThread.joinable()) {
        workerThread.join();
    }
    if (reconnectThread.joinable()) {
        reconnectThread.join();
    }
    
    cleanupConnection();
    notifyConnection(false);
}

void IpcClient::start() {
    if (running) return;
    running = true;
    workerThread = std::thread(&IpcClient::processIncoming, this);
}

void IpcClient::processIncoming() {
    while (running && connected) {
        std::string data = receiveInternal(100);
        if (!data.empty()) {
            IpcMessage message;
            if (message.deserialize(data)) {
                stats.messagesReceived++;
                lastActivity = std::chrono::steady_clock::now();
                
                // Check if it's a response to a pending request
                std::string requestId = message.getCorrelationId();
                if (!requestId.empty()) {
                    auto it = pendingRequests.find(requestId);
                    if (it != pendingRequests.end()) {
                        try {
                            it->second(message);
                        } catch (...) {}
                        pendingRequests.erase(it);
                        continue;
                    }
                }
                
                // Check subscriptions
                std::string topic = message.getTopic();
                std::lock_guard<std::mutex> lock(mutex);
                for (const auto& [subTopic, callbacks] : subscriptions) {
                    if (subTopic == topic || subTopic == "*") {
                        for (const auto& callback : callbacks) {
                            try {
                                callback(message);
                            } catch (...) {}
                        }
                    }
                }
            }
        }
        
        // Check connection status
        if (connected && !isConnected()) {
            notifyConnection(false);
            if (autoReconnect) {
                handleReconnect();
            }
        }
    }
}

void IpcClient::handleReconnect() {
    if (!autoReconnect) return;
    
    if (reconnectThread.joinable()) {
        reconnectThread.join();
    }
    
    reconnectThread = std::thread([this]() {
        while (!connected && running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay));
            if (connectInternal()) {
                connected = true;
                retryCount = 0;
                stats.reconnections++;
                notifyConnection(true);
                start();
                break;
            }
            retryCount++;
            if (maxRetries >= 0 && retryCount >= maxRetries) {
                notifyError("Max reconnection attempts reached");
                break;
            }
        }
    });
}

bool IpcClient::send(const IpcMessage& message, int timeoutMs) {
    if (!connected) {
        lastError = "Not connected";
        return false;
    }
    
    std::string data = message.serialize();
    bool result = sendInternal(data);
    
    if (result) {
        stats.messagesSent++;
        lastActivity = std::chrono::steady_clock::now();
    } else {
        notifyError("Send failed");
    }
    
    return result;
}

bool IpcClient::sendInternal(const std::string& data) {
    if (!connection) {
        return false;
    }
    
    // Add message length prefix (4 bytes)
    uint32_t len = htonl(data.length());
    std::string msg;
    msg.append((char*)&len, 4);
    msg.append(data);
    
    ssize_t sent = send((int)(intptr_t)connection, msg.c_str(), msg.length(), MSG_NOSIGNAL);
    return sent == (ssize_t)msg.length();
}

std::string IpcClient::receiveInternal(int timeoutMs) {
    if (!connection) {
        return "";
    }
    
    // Set timeout
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt((int)(intptr_t)connection, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Read message length
    uint32_t len = 0;
    ssize_t received = recv((int)(intptr_t)connection, &len, sizeof(len), 0);
    if (received != sizeof(len)) {
        return "";
    }
    len = ntohl(len);
    
    if (len > 1024 * 1024) {
        lastError = "Message size too large";
        return "";
    }
    
    // Read message data
    std::vector<char> buffer(len + 1, 0);
    size_t total = 0;
    while (total < len) {
        received = recv((int)(intptr_t)connection, buffer.data() + total, len - total, 0);
        if (received <= 0) {
            return "";
        }
        total += received;
    }
    
    return std::string(buffer.data(), len);
}

IpcMessage IpcClient::request(const IpcMessage& message, int timeoutMs) {
    IpcMessage response;
    
    if (!connected) {
        lastError = "Not connected";
        return response;
    }
    
    std::string requestId = generateRequestId();
    IpcMessage req = message;
    req.setRequestId(requestId);
    req.setType(IpcMessage::REQUEST);
    
    std::promise<IpcMessage> promise;
    auto future = promise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(mutex);
        pendingRequests[requestId] = [&promise](const IpcMessage& msg) {
            promise.set_value(msg);
        };
    }
    
    if (!send(req, timeoutMs)) {
        std::lock_guard<std::mutex> lock(mutex);
        pendingRequests.erase(requestId);
        return response;
    }
    
    auto status = future.wait_for(std::chrono::milliseconds(timeoutMs));
    if (status == std::future_status::timeout) {
        std::lock_guard<std::mutex> lock(mutex);
        pendingRequests.erase(requestId);
        stats.errors++;
        lastError = "Request timeout";
        return response;
    }
    
    response = future.get();
    return response;
}

void IpcClient::subscribe(const std::string& topic, MessageCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    subscriptions[topic].push_back(callback);
}

void IpcClient::unsubscribe(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex);
    subscriptions.erase(topic);
}

void IpcClient::setAutoReconnect(bool enable, int retryDelay, int maxRetries) {
    autoReconnect = enable;
    this->retryDelay = retryDelay;
    this->maxRetries = maxRetries;
}

void IpcClient::notifyConnection(bool status) {
    if (connectionCallback) {
        try {
            connectionCallback(status);
        } catch (...) {}
    }
}

void IpcClient::notifyError(const std::string& error) {
    lastError = error;
    stats.errors++;
    if (errorCallback) {
        try {
            errorCallback(error);
        } catch (...) {}
    }
}

void IpcClient::cleanupConnection() {
    if (connection) {
        close((int)(intptr_t)connection);
        connection = nullptr;
    }
}

Json::Value IpcClient::getStats() const {
    Json::Value statsJson;
    statsJson["messages_sent"] = stats.messagesSent.load();
    statsJson["messages_received"] = stats.messagesReceived.load();
    statsJson["errors"] = stats.errors.load();
    statsJson["reconnections"] = stats.reconnections.load();
    statsJson["connected"] = connected;
    
    auto now = std::chrono::steady_clock::now();
    statsJson["uptime_seconds"] = std::chrono::duration_cast<std::chrono::seconds>(
        now - stats.startTime).count();
    
    return statsJson;
}
