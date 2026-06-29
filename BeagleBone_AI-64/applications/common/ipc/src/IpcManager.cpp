#include "ipc/IpcManager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <thread>

#ifdef IPC_HAVE_DBUS
#include <dbus/dbus.h>
#endif

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace common;

IpcManager::IpcManager() 
    : transportData(nullptr), socketFd(-1), sharedMemory(nullptr),
      mqttClient(nullptr), zmqContext(nullptr), zmqSocket(nullptr) {
    stats.startTime = std::chrono::steady_clock::now();
    stats.lastMessageTime = stats.startTime;
}

IpcManager::~IpcManager() {
    stop();
    cleanupConnection();
}

IpcManager& IpcManager::getInstance() {
    static IpcManager instance;
    return instance;
}

bool IpcManager::initialize(TransportType type, const std::string& address, 
                            const Json::Value& options) {
    if (initialized) {
        return true;
    }
    
    this->transportType = type;
    this->address = address;
    this->options = options;
    
    // Set defaults from options
    if (options.isMember("timeout")) {
        defaultTimeout = options["timeout"].asInt();
    }
    if (options.isMember("queueing")) {
        queueingEnabled = options["queueing"].asBool();
    }
    if (options.isMember("maxQueueSize")) {
        maxQueueSize = options["maxQueueSize"].asUInt64();
    }
    
    bool result = false;
    switch (type) {
        case TransportType::DBUS:
            result = initializeDBus(address);
            break;
        case TransportType::UNIX_SOCKET:
            result = initializeUnixSocket(address);
            break;
        case TransportType::SHARED_MEMORY:
            result = initializeSharedMemory(address);
            break;
        case TransportType::MQTT:
            result = initializeMQTT(address);
            break;
        case TransportType::ZMQ:
            result = initializeZMQ(address);
            break;
        case TransportType::TCP:
            result = initializeTCP(address);
            break;
        case TransportType::UDP:
            result = initializeUDP(address);
            break;
        case TransportType::NAMED_PIPE:
            result = initializeNamedPipe(address);
            break;
        case TransportType::MEMORY_MAPPED:
            result = initializeMemoryMapped(address);
            break;
        default:
            lastError = "Unsupported transport type";
            return false;
    }
    
    if (result) {
        initialized = true;
        start();
    }
    
    return result;
}

bool IpcManager::initializeUnixSocket(const std::string& address) {
    socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socketFd < 0) {
        lastError = "Failed to create Unix socket: " + std::string(strerror(errno));
        return false;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, address.c_str(), sizeof(addr.sun_path) - 1);
    
    // Remove existing socket file
    unlink(address.c_str());
    
    if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        lastError = "Failed to bind Unix socket: " + std::string(strerror(errno));
        close(socketFd);
        socketFd = -1;
        return false;
    }
    
    if (listen(socketFd, 10) < 0) {
        lastError = "Failed to listen on Unix socket: " + std::string(strerror(errno));
        close(socketFd);
        socketFd = -1;
        return false;
    }
    
    // Set non-blocking
    int flags = fcntl(socketFd, F_GETFL, 0);
    fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);
    
    connected = true;
    return true;
}

bool IpcManager::initializeSharedMemory(const std::string& address) {
    int shmFd = shm_open(address.c_str(), O_CREAT | O_RDWR, 0666);
    if (shmFd < 0) {
        lastError = "Failed to create shared memory: " + std::string(strerror(errno));
        return false;
    }
    
    // Set size from options or default
    size_t size = 1024 * 1024; // 1MB default
    if (options.isMember("size")) {
        size = options["size"].asUInt64();
    }
    
    if (ftruncate(shmFd, size) < 0) {
        lastError = "Failed to resize shared memory: " + std::string(strerror(errno));
        close(shmFd);
        return false;
    }
    
    sharedMemory = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (sharedMemory == MAP_FAILED) {
        lastError = "Failed to map shared memory: " + std::string(strerror(errno));
        close(shmFd);
        return false;
    }
    
    close(shmFd);
    connected = true;
    return true;
}

bool IpcManager::initializeMQTT(const std::string& address) {
    // MQTT initialization - simplified for this example
    // In production, you'd use a library like Paho MQTT
    std::cout << "IPC: MQTT initialized at " << address << std::endl;
    
    // Parse broker address
    size_t pos = address.find("://");
    if (pos != std::string::npos) {
        std::string protocol = address.substr(0, pos);
        std::string rest = address.substr(pos + 3);
        // Connect to broker
        // Implementation would use MQTT library here
    }
    
    connected = true;
    return true;
}

void IpcManager::registerHandler(const std::string& topic, MessageHandler handler, int priority) {
    std::lock_guard<std::mutex> lock(queueMutex);
    handlers[topic].insert({priority, handler});
}

void IpcManager::unregisterHandler(const std::string& topic, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(queueMutex);
    auto it = handlers.find(topic);
    if (it != handlers.end()) {
        for (auto rit = it->second.begin(); rit != it->second.end(); ) {
            // Can't compare std::function directly, we'll use a token approach
            // Simplified: clear all handlers for topic
            if (rit->second.target_type() == handler.target_type()) {
                rit = it->second.erase(rit);
            } else {
                ++rit;
            }
        }
    }
}

bool IpcManager::sendMessage(const IpcMessage& message, int timeoutMs) {
    if (!initialized || !connected) {
        lastError = "Not initialized or not connected";
        return false;
    }
    
    if (queueingEnabled) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (messageQueue.size() >= maxQueueSize) {
            lastError = "Message queue full";
            return false;
        }
        messageQueue.push({message, std::chrono::steady_clock::now()});
        queueCV.notify_one();
        stats.messagesQueued++;
        return true;
    }
    
    // Send directly
    switch (transportType) {
        case TransportType::DBUS:
            sendToDBus(message);
            break;
        case TransportType::UNIX_SOCKET:
            sendToUnixSocket(message);
            break;
        case TransportType::SHARED_MEMORY:
            sendToSharedMemory(message);
            break;
        case TransportType::MQTT:
            sendToMQTT(message);
            break;
        case TransportType::ZMQ:
            sendToZMQ(message);
            break;
        case TransportType::TCP:
            sendToTCP(message);
            break;
        case TransportType::UDP:
            sendToUDP(message);
            break;
        case TransportType::NAMED_PIPE:
            sendToNamedPipe(message);
            break;
        case TransportType::MEMORY_MAPPED:
            sendToMemoryMapped(message);
            break;
        default:
            return false;
    }
    
    stats.messagesSent++;
    return true;
}

IpcMessage IpcManager::sendRequest(const IpcMessage& message, int timeoutMs) {
    IpcMessage response;
    
    if (!initialized || !connected) {
        lastError = "Not initialized or not connected";
        return response;
    }
    
    std::string requestId = generateRequestId();
    IpcMessage req = message;
    req.setRequestId(requestId);
    req.setType(IpcMessage::REQUEST);
    
    // Register for response
    std::promise<IpcMessage> promise;
    auto future = promise.get_future();
    
    registerHandler("response/" + requestId, [&promise](const IpcMessage& msg) {
        promise.set_value(msg);
    }, 10);
    
    // Send request
    if (!sendMessage(req, timeoutMs)) {
        unregisterHandler("response/" + requestId, nullptr);
        return response;
    }
    
    // Wait for response
    auto status = future.wait_for(std::chrono::milliseconds(timeoutMs));
    if (status == std::future_status::timeout) {
        unregisterHandler("response/" + requestId, nullptr);
        stats.timeouts++;
        lastError = "Request timeout";
        return response;
    }
    
    response = future.get();
    unregisterHandler("response/" + requestId, nullptr);
    return response;
}

std::string IpcManager::sendAsyncRequest(const IpcMessage& message, 
                                         MessageHandler callback, 
                                         int timeoutMs) {
    std::string requestId = generateRequestId();
    IpcMessage req = message;
    req.setRequestId(requestId);
    req.setType(IpcMessage::REQUEST);
    
    // Store pending request
    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        auto expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        pendingRequests[requestId] = {requestId, req, callback, expiry};
    }
    
    // Register handler for response
    registerHandler("response/" + requestId, callback, 10);
    
    // Send request
    if (!sendMessage(req, timeoutMs)) {
        unregisterHandler("response/" + requestId, nullptr);
        std::lock_guard<std::mutex> lock(pendingMutex);
        pendingRequests.erase(requestId);
        return "";
    }
    
    return requestId;
}

void IpcManager::broadcast(const IpcMessage& message) {
    // Internal broadcast to registered handlers
    std::lock_guard<std::mutex> lock(queueMutex);
    
    std::string topic = message.getTopic();
    for (const auto& [pattern, handlerList] : handlers) {
        if (matchesTopic(pattern, topic)) {
            for (const auto& [priority, handler] : handlerList) {
                try {
                    handler(message);
                } catch (const std::exception& e) {
                    notifyError("Handler error: " + std::string(e.what()));
                }
            }
        }
    }
}

bool IpcManager::matchesTopic(const std::string& pattern, const std::string& topic) {
    // Simple wildcard matching
    // Supports: * (single level), # (multi-level), + (single level)
    if (pattern == "#" || pattern == "*") {
        return true;
    }
    
    if (pattern == topic) {
        return true;
    }
    
    // More complex matching could be added here
    return false;
}

void IpcManager::processMessages() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait_for(lock, std::chrono::milliseconds(100), 
                        [this] { return !messageQueue.empty() || !running; });
        
        while (!messageQueue.empty()) {
            QueuedMessage queued = messageQueue.front();
            messageQueue.pop();
            lock.unlock();
            
            // Process message based on transport type
            try {
                switch (transportType) {
                    case TransportType::DBUS:
                        sendToDBus(queued.message);
                        break;
                    case TransportType::UNIX_SOCKET:
                        sendToUnixSocket(queued.message);
                        break;
                    case TransportType::SHARED_MEMORY:
                        sendToSharedMemory(queued.message);
                        break;
                    case TransportType::MQTT:
                        sendToMQTT(queued.message);
                        break;
                    case TransportType::ZMQ:
                        sendToZMQ(queued.message);
                        break;
                    case TransportType::TCP:
                        sendToTCP(queued.message);
                        break;
                    case TransportType::UDP:
                        sendToUDP(queued.message);
                        break;
                    case TransportType::NAMED_PIPE:
                        sendToNamedPipe(queued.message);
                        break;
                    case TransportType::MEMORY_MAPPED:
                        sendToMemoryMapped(queued.message);
                        break;
                    default:
                        break;
                }
            } catch (const std::exception& e) {
                notifyError("Send error: " + std::string(e.what()));
            }
            
            // Also broadcast internally
            broadcast(queued.message);
            
            stats.messagesSent++;
            lock.lock();
        }
    }
}

void IpcManager::start() {
    if (running) return;
    running = true;
    workerThread = std::thread(&IpcManager::processMessages, this);
    timeoutThread = std::thread(&IpcManager::checkTimeout, this);
}

void IpcManager::stop() {
    if (!running) return;
    running = false;
    queueCV.notify_all();
    if (workerThread.joinable()) {
        workerThread.join();
    }
    if (timeoutThread.joinable()) {
        timeoutThread.join();
    }
}

void IpcManager::checkTimeout() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::lock_guard<std::mutex> lock(pendingMutex);
        auto now = std::chrono::steady_clock::now();
        for (auto it = pendingRequests.begin(); it != pendingRequests.end(); ) {
            if (now > it->second.expiry) {
                // Timeout - remove pending request
                unregisterHandler("response/" + it->first, nullptr);
                it = pendingRequests.erase(it);
                stats.timeouts++;
            } else {
                ++it;
            }
        }
    }
}

std::string IpcManager::generateRequestId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    for (int i = 0; i < 8; ++i) {
        ss << dis(gen);
    }
    return ss.str();
}

void IpcManager::notifyError(const std::string& error) {
    lastError = error;
    stats.errors++;
    if (errorHandler) {
        try {
            errorHandler(error);
        } catch (...) {}
    }
}

void IpcManager::notifyConnection(bool status) {
    connected = status;
    if (connectionHandler) {
        try {
            connectionHandler(status);
        } catch (...) {}
    }
}

Json::Value IpcManager::getStats() const {
    Json::Value statsJson;
    statsJson["messages_sent"] = stats.messagesSent.load();
    statsJson["messages_received"] = stats.messagesReceived.load();
    statsJson["messages_queued"] = stats.messagesQueued.load();
    statsJson["errors"] = stats.errors.load();
    statsJson["timeouts"] = stats.timeouts.load();
    statsJson["connected"] = connected;
    statsJson["running"] = running;
    
    auto now = std::chrono::steady_clock::now();
    statsJson["uptime_seconds"] = std::chrono::duration_cast<std::chrono::seconds>(
        now - stats.startTime).count();
    
    return statsJson;
}

void IpcManager::resetStats() {
    stats.messagesSent = 0;
    stats.messagesReceived = 0;
    stats.messagesQueued = 0;
    stats.errors = 0;
    stats.timeouts = 0;
    stats.startTime = std::chrono::steady_clock::now();
}

void IpcManager::sendToUnixSocket(const IpcMessage& message) {
    if (socketFd < 0) return;
    
    std::string msgStr = message.serialize();
    ssize_t sent = send(socketFd, msgStr.c_str(), msgStr.length(), 0);
    if (sent < 0) {
        notifyError("Unix socket send failed: " + std::string(strerror(errno)));
    }
}

void IpcManager::sendToSharedMemory(const IpcMessage& message) {
    if (!sharedMemory) return;
    
    std::string msgStr = message.serialize();
    size_t size = msgStr.length() + 1;
    // Add header with size
    memcpy(sharedMemory, &size, sizeof(size_t));
    memcpy((char*)sharedMemory + sizeof(size_t), msgStr.c_str(), msgStr.length());
}

void IpcManager::sendToMQTT(const IpcMessage& message) {
    // Simplified MQTT send
    std::string topic = message.getTopic();
    std::string payload = message.serialize();
    // In production, use MQTT library
}

void IpcManager::sendToZMQ(const IpcMessage& message) {
    // Simplified ZMQ send
    if (!zmqSocket) return;
    std::string data = message.serialize();
    // zmq_send(zmqSocket, data.c_str(), data.length(), 0);
}

void IpcManager::setQueueing(bool enable, size_t maxQueueSize) {
    queueingEnabled = enable;
    this->maxQueueSize = maxQueueSize;
}

void IpcManager::clearQueue() {
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!messageQueue.empty()) {
        messageQueue.pop();
    }
}

void IpcManager::cleanupConnection() {
    switch (transportType) {
        case TransportType::UNIX_SOCKET:
            if (socketFd >= 0) {
                close(socketFd);
                socketFd = -1;
            }
            break;
        case TransportType::SHARED_MEMORY:
            if (sharedMemory) {
                size_t size = 1024 * 1024; // Should track actual size
                munmap(sharedMemory, size);
                sharedMemory = nullptr;
            }
            break;
        default:
            break;
    }
}

void IpcManager::handleMessage(const IpcMessage& message) {
    stats.messagesReceived++;
    
    // Check for response to pending request
    if (message.getType() == IpcMessage::RESPONSE) {
        std::string requestId = message.getCorrelationId();
        if (!requestId.empty()) {
            // Find pending request
            std::lock_guard<std::mutex> lock(pendingMutex);
            auto it = pendingRequests.find(requestId);
            if (it != pendingRequests.end()) {
                // Trigger callback
                if (it->second.callback) {
                    try {
                        it->second.callback(message);
                    } catch (...) {}
                }
                pendingRequests.erase(it);
            }
        }
    }
    
    // Broadcast to handlers
    broadcast(message);
}

void IpcManager::publish(const std::string& topic, const std::string& message, 
                         int qos, bool retain) {
    Json::Value body;
    body["data"] = message;
    IpcMessage msg(topic, body);
    msg.setType(IpcMessage::NOTIFICATION);
    sendMessage(msg);
}

void IpcManager::subscribe(const std::string& topic, int qos) {
    // Subscribe to topic
    IpcMessage msg("subscribe", Json::objectValue);
    msg.getBody()["topic"] = topic;
    msg.getBody()["qos"] = qos;
    sendMessage(msg);
}

void IpcManager::unsubscribe(const std::string& topic) {
    IpcMessage msg("unsubscribe", Json::objectValue);
    msg.getBody()["topic"] = topic;
    sendMessage(msg);
}

std::string IpcManager::getLastError() const {
    return lastError;
}

void IpcManager::setErrorHandler(ErrorHandler handler) {
    errorHandler = handler;
}

void IpcManager::setConnectionHandler(ConnectionHandler handler) {
    connectionHandler = handler;
}
