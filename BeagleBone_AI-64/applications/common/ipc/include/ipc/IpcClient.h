#ifndef IPC_CLIENT_H
#define IPC_CLIENT_H

#include <string>
#include <functional>
#include <memory>
#include "IpcMessage.h"

namespace common {

/**
 * @brief IPC Client for connecting to IPC server
 * 
 * Provides a client interface for IPC communication
 * with automatic reconnection and error handling
 */
class IpcClient {
public:
    using MessageCallback = std::function<void(const IpcMessage&)>;
    using ConnectionCallback = std::function<void(bool connected)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    /**
     * @brief Constructor
     * @param serverAddress Server address (e.g., "unix:/tmp/socket", "tcp://localhost:5555")
     * @param clientId Unique client identifier
     */
    IpcClient(const std::string& serverAddress, const std::string& clientId = "");
    ~IpcClient();
    
    /**
     * @brief Connect to server
     * @param timeoutMs Connection timeout
     * @return true if connected successfully
     */
    bool connect(int timeoutMs = 5000);
    
    /**
     * @brief Disconnect from server
     */
    void disconnect();
    
    /**
     * @brief Check if connected
     */
    bool isConnected() const { return connected; }
    
    /**
     * @brief Send message to server
     * @param message Message to send
     * @param timeoutMs Send timeout
     * @return true if sent successfully
     */
    bool send(const IpcMessage& message, int timeoutMs = 3000);
    
    /**
     * @brief Send request and wait for response
     * @param message Request message
     * @param timeoutMs Response timeout
     * @return Response message
     */
    IpcMessage request(const IpcMessage& message, int timeoutMs = 5000);
    
    /**
     * @brief Send asynchronous request
     * @param message Request message
     * @param callback Response callback
     * @param timeoutMs Timeout in milliseconds
     * @return Request ID
     */
    std::string requestAsync(const IpcMessage& message, 
                            MessageCallback callback, 
                            int timeoutMs = 5000);
    
    /**
     * @brief Subscribe to messages
     * @param topic Topic to subscribe to
     * @param callback Message callback
     */
    void subscribe(const std::string& topic, MessageCallback callback);
    
    /**
     * @brief Unsubscribe from topic
     * @param topic Topic to unsubscribe from
     */
    void unsubscribe(const std::string& topic);
    
    /**
     * @brief Set connection callback
     * @param callback Connection callback
     */
    void setConnectionCallback(ConnectionCallback callback);
    
    /**
     * @brief Set error callback
     * @param callback Error callback
     */
    void setErrorCallback(ErrorCallback callback);
    
    /**
     * @brief Enable/disable auto-reconnect
     * @param enable Enable auto-reconnect
     * @param retryDelay Delay between retries (ms)
     * @param maxRetries Maximum retries (-1 = infinite)
     */
    void setAutoReconnect(bool enable, int retryDelay = 5000, int maxRetries = -1);
    
    /**
     * @brief Get client ID
     */
    std::string getClientId() const { return clientId; }
    
    /**
     * @brief Get server address
     */
    std::string getServerAddress() const { return serverAddress; }
    
    /**
     * @brief Get last error
     */
    std::string getLastError() const { return lastError; }
    
    /**
     * @brief Get statistics
     */
    Json::Value getStats() const;

private:
    std::string serverAddress;
    std::string clientId;
    std::atomic<bool> connected{false};
    std::atomic<bool> running{false};
    std::atomic<bool> autoReconnect{true};
    std::atomic<int> retryDelay{5000};
    std::atomic<int> maxRetries{-1};
    std::atomic<int> retryCount{0};
    std::string lastError;
    
    ConnectionCallback connectionCallback;
    ErrorCallback errorCallback;
    std::map<std::string, std::vector<MessageCallback>> subscriptions;
    std::map<std::string, MessageCallback> pendingRequests;
    
    void* connection;
    std::mutex mutex;
    std::thread workerThread;
    std::thread reconnectThread;
    std::chrono::steady_clock::time_point lastActivity;
    
    // Statistics
    struct {
        std::atomic<uint64_t> messagesSent{0};
        std::atomic<uint64_t> messagesReceived{0};
        std::atomic<uint64_t> errors{0};
        std::atomic<uint64_t> reconnections{0};
        std::chrono::steady_clock::time_point startTime;
    } stats;
    
    void processIncoming();
    void handleReconnect();
    void notifyConnection(bool status);
    void notifyError(const std::string& error);
    bool connectInternal();
    void disconnectInternal();
    bool sendInternal(const std::string& data);
    std::string receiveInternal(int timeoutMs);
    bool setupConnection();
    void cleanupConnection();
};

} // namespace common

#endif // IPC_CLIENT_H
