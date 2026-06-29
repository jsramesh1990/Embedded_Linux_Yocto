#ifndef IPC_MANAGER_H
#define IPC_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include "IpcMessage.h"

namespace common {

/**
 * @brief IPC Manager for inter-process communication
 * 
 * Supports multiple transport methods:
 * - D-Bus (Linux desktop)
 * - Unix Domain Sockets (local IPC)
 * - Shared Memory (high-speed data exchange)
 * - MQTT (network messaging)
 * - ZeroMQ (distributed messaging)
 */
class IpcManager {
public:
    /**
     * @brief Transport types supported
     */
    enum class TransportType {
        DBUS,           // D-Bus system bus
        UNIX_SOCKET,    // Unix domain socket
        SHARED_MEMORY,  // POSIX shared memory
        MQTT,           // MQTT protocol
        ZMQ,            // ZeroMQ
        TCP,            // TCP sockets
        UDP,            // UDP datagrams
        NAMED_PIPE,     // Named pipes (FIFO)
        MEMORY_MAPPED   // Memory-mapped files
    };
    
    /**
     * @brief Message handler callback type
     */
    using MessageHandler = std::function<void(const IpcMessage&)>;
    
    /**
     * @brief Error handler callback type
     */
    using ErrorHandler = std::function<void(const std::string& error)>;
    
    /**
     * @brief Connection status callback
     */
    using ConnectionHandler = std::function<void(bool connected)>;
    
    /**
     * @brief Get singleton instance
     */
    static IpcManager& getInstance();
    
    /**
     * @brief Initialize IPC manager
     * @param type Transport type
     * @param address Address/name for the transport
     * @param options Additional options as JSON
     * @return true if initialized successfully
     */
    bool initialize(TransportType type, const std::string& address, 
                    const Json::Value& options = Json::nullValue);
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const { return initialized; }
    
    /**
     * @brief Get current transport type
     */
    TransportType getTransportType() const { return transportType; }
    
    /**
     * @brief Get connection address
     */
    std::string getAddress() const { return address; }
    
    /**
     * @brief Register message handler for a topic
     * @param topic Message topic (supports wildcards: *, +, #)
     * @param handler Handler function
     * @param priority Handler priority (higher = called first)
     */
    void registerHandler(const std::string& topic, MessageHandler handler, 
                        int priority = 0);
    
    /**
     * @brief Unregister message handler
     * @param topic Message topic
     * @param handler Handler function to remove
     */
    void unregisterHandler(const std::string& topic, MessageHandler handler);
    
    /**
     * @brief Register error handler
     * @param handler Error handler function
     */
    void setErrorHandler(ErrorHandler handler);
    
    /**
     * @brief Register connection handler
     * @param handler Connection status handler
     */
    void setConnectionHandler(ConnectionHandler handler);
    
    /**
     * @brief Send message
     * @param message IPC message to send
     * @param timeoutMs Timeout in milliseconds (0 = no timeout)
     * @return true if sent successfully
     */
    bool sendMessage(const IpcMessage& message, int timeoutMs = 0);
    
    /**
     * @brief Send message and wait for response
     * @param message Request message
     * @param timeoutMs Timeout in milliseconds
     * @return Response message (empty if timeout)
     */
    IpcMessage sendRequest(const IpcMessage& message, int timeoutMs = 5000);
    
    /**
     * @brief Send asynchronous request with callback
     * @param message Request message
     * @param callback Response handler
     * @param timeoutMs Timeout in milliseconds
     * @return Request ID
     */
    std::string sendAsyncRequest(const IpcMessage& message, 
                                 MessageHandler callback, 
                                 int timeoutMs = 5000);
    
    /**
     * @brief Broadcast message to all listeners
     * @param message Message to broadcast
     */
    void broadcast(const IpcMessage& message);
    
    /**
     * @brief Publish message to a topic (for MQTT)
     * @param topic Topic name
     * @param message Message content
     * @param qos Quality of service (0, 1, 2)
     * @param retain Retain message on broker
     */
    void publish(const std::string& topic, const std::string& message, 
                 int qos = 0, bool retain = false);
    
    /**
     * @brief Subscribe to a topic (for MQTT)
     * @param topic Topic name (supports wildcards)
     * @param qos Quality of service
     */
    void subscribe(const std::string& topic, int qos = 0);
    
    /**
     * @brief Unsubscribe from a topic
     * @param topic Topic name
     */
    void unsubscribe(const std::string& topic);
    
    /**
     * @brief Start/stop message processing
     */
    void start();
    void stop();
    
    /**
     * @brief Check if running
     */
    bool isRunning() const { return running; }
    
    /**
     * @brief Get statistics
     * @return JSON object with statistics
     */
    Json::Value getStats() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
    /**
     * @brief Get connection status
     */
    bool isConnected() const { return connected; }
    
    /**
     * @brief Get last error message
     */
    std::string getLastError() const { return lastError; }
    
    /**
     * @brief Set timeout for operations
     * @param timeoutMs Timeout in milliseconds
     */
    void setTimeout(int timeoutMs) { defaultTimeout = timeoutMs; }
    
    /**
     * @brief Enable/disable message queuing
     * @param enable Enable queuing
     * @param maxQueueSize Maximum queue size
     */
    void setQueueing(bool enable, size_t maxQueueSize = 10000);
    
    /**
     * @brief Clear message queue
     */
    void clearQueue();

private:
    // Singleton
    IpcManager();
    ~IpcManager();
    IpcManager(const IpcManager&) = delete;
    IpcManager& operator=(const IpcManager&) = delete;
    
    // State
    TransportType transportType;
    std::string address;
    Json::Value options;
    std::atomic<bool> initialized{false};
    std::atomic<bool> running{false};
    std::atomic<bool> connected{false};
    std::atomic<int> defaultTimeout{5000};
    std::atomic<bool> queueingEnabled{true};
    std::atomic<size_t> maxQueueSize{10000};
    std::string lastError;
    
    // Handlers
    std::map<std::string, std::multimap<int, MessageHandler>> handlers;
    ErrorHandler errorHandler;
    ConnectionHandler connectionHandler;
    
    // Message queue
    struct QueuedMessage {
        IpcMessage message;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::queue<QueuedMessage> messageQueue;
    mutable std::mutex queueMutex;
    std::condition_variable queueCV;
    std::thread workerThread;
    
    // Statistics
    struct Stats {
        std::atomic<uint64_t> messagesSent{0};
        std::atomic<uint64_t> messagesReceived{0};
        std::atomic<uint64_t> messagesQueued{0};
        std::atomic<uint64_t> errors{0};
        std::atomic<uint64_t> timeouts{0};
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastMessageTime;
    } stats;
    
    // Request tracking
    struct PendingRequest {
        std::string requestId;
        IpcMessage request;
        MessageHandler callback;
        std::chrono::steady_clock::time_point expiry;
    };
    std::map<std::string, PendingRequest> pendingRequests;
    std::mutex pendingMutex;
    std::thread timeoutThread;
    
    // Transport specific data
    void* transportData;
    int socketFd;
    void* sharedMemory;
    void* mqttClient;
    void* zmqContext;
    void* zmqSocket;
    
    // Internal methods
    void processMessages();
    void processPendingRequests();
    void handleMessage(const IpcMessage& message);
    void notifyError(const std::string& error);
    void notifyConnection(bool connected);
    std::string generateRequestId();
    bool matchesTopic(const std::string& pattern, const std::string& topic);
    void checkTimeout();
    
    // Transport implementations
    bool initializeDBus(const std::string& address);
    bool initializeUnixSocket(const std::string& address);
    bool initializeSharedMemory(const std::string& address);
    bool initializeMQTT(const std::string& address);
    bool initializeZMQ(const std::string& address);
    bool initializeTCP(const std::string& address);
    bool initializeUDP(const std::string& address);
    bool initializeNamedPipe(const std::string& address);
    bool initializeMemoryMapped(const std::string& address);
    
    void sendToDBus(const IpcMessage& message);
    void sendToUnixSocket(const IpcMessage& message);
    void sendToSharedMemory(const IpcMessage& message);
    void sendToMQTT(const IpcMessage& message);
    void sendToZMQ(const IpcMessage& message);
    void sendToTCP(const IpcMessage& message);
    void sendToUDP(const IpcMessage& message);
    void sendToNamedPipe(const IpcMessage& message);
    void sendToMemoryMapped(const IpcMessage& message);
    
    void* setupUnixSocket(const std::string& address);
    void* setupSharedMemory(const std::string& address);
    void* setupMQTT(const std::string& address);
    void* setupZMQ(const std::string& address);
};

/**
 * @brief Macro for easy IPC messaging
 */
#define IPC_SEND(topic, body) \
    IpcManager::getInstance().sendMessage(IpcMessage(topic, body))

#define IPC_PUBLISH(topic, msg) \
    IpcManager::getInstance().publish(topic, msg)

#define IPC_REGISTER(topic, handler) \
    IpcManager::getInstance().registerHandler(topic, handler)

} // namespace common

#endif // IPC_MANAGER_H
