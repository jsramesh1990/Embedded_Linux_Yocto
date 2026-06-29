#ifndef IPC_SERVER_H
#define IPC_SERVER_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include "IpcMessage.h"

namespace common {

/**
 * @brief IPC Server for handling client connections
 * 
 * Provides a server interface for IPC communication
 * with support for multiple clients and message routing
 */
class IpcServer {
public:
    using ClientHandler = std::function<void(const std::string& clientId, const IpcMessage&)>;
    using ConnectionHandler = std::function<void(const std::string& clientId, bool connected)>;
    using ErrorHandler = std::function<void(const std::string& error)>;
    
    /**
     * @brief Client information
     */
    struct ClientInfo {
        std::string id;
        std::string address;
        std::chrono::steady_clock::time_point connected;
        std::chrono::steady_clock::time_point lastActivity;
        uint64_t messagesSent;
        uint64_t messagesReceived;
        bool authenticated;
        std::vector<std::string> subscriptions;
    };
    
    /**
     * @brief Constructor
     * @param serverAddress Server address (e.g., "unix:/tmp/socket", "tcp://localhost:5555")
     * @param serverName Server name
     */
    IpcServer(const std::string& serverAddress, const std::string& serverName = "");
    ~IpcServer();
    
    /**
     * @brief Start server
     * @param maxClients Maximum number of clients
     * @return true if started successfully
     */
    bool start(int maxClients = 100);
    
    /**
     * @brief Stop server
     */
    void stop();
    
    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return running; }
    
    /**
     * @brief Send message to a specific client
     * @param clientId Client ID
     * @param message Message to send
     * @return true if sent successfully
     */
    bool sendToClient(const std::string& clientId, const IpcMessage& message);
    
    /**
     * @brief Broadcast message to all clients
     * @param message Message to broadcast
     * @param excludeClientId Client to exclude (optional)
     */
    void broadcast(const IpcMessage& message, const std::string& excludeClientId = "");
    
    /**
     * @brief Send message to clients subscribed to a topic
     * @param topic Topic
     * @param message Message to send
     */
    void sendToSubscribers(const std::string& topic, const IpcMessage& message);
    
    /**
     * @brief Register message handler
     * @param topic Topic
     * @param handler Message handler
     */
    void registerHandler(const std::string& topic, ClientHandler handler);
    
    /**
     * @brief Register connection handler
     * @param handler Connection handler
     */
    void setConnectionHandler(ConnectionHandler handler);
    
    /**
     * @brief Register error handler
     * @param handler Error handler
     */
    void setErrorHandler(ErrorHandler handler);
    
    /**
     * @brief Authenticate client
     * @param clientId Client ID
     * @param token Authentication token
     * @return true if authenticated
     */
    bool authenticateClient(const std::string& clientId, const std::string& token);
    
    /**
     * @brief Get connected clients
     * @return Map of client IDs to client info
     */
    std::map<std::string, ClientInfo> getClients() const;
    
    /**
     * @brief Get client info
     * @param clientId Client ID
     * @return Client info (if connected)
     */
    std::optional<ClientInfo> getClientInfo(const std::string& clientId) const;
    
    /**
     * @brief Kick client
     * @param clientId Client ID
     * @param reason Reason for kick
     * @return true if kicked successfully
     */
    bool kickClient(const std::string& clientId, const std::string& reason = "");
    
    /**
     * @brief Get statistics
     * @return JSON statistics
     */
    Json::Value getStats() const;
    
    /**
     * @brief Get server status
     * @return JSON status
     */
    Json::Value getStatus() const;
    
    /**
     * @brief Set authentication required
     * @param required Require authentication
     * @param authCallback Authentication callback
     */
    void setAuthentication(bool required, 
                          std::function<bool(const std::string&, const std::string&)> authCallback);
    
    /**
     * @brief Set message size limit
     * @param maxSize Maximum message size in bytes
     */
    void setMaxMessageSize(size_t maxSize) { maxMessageSize = maxSize; }
    
    /**
     * @brief Set timeout for client inactivity
     * @param timeoutMs Timeout in milliseconds
     */
    void setClientTimeout(int timeoutMs) { clientTimeout = timeoutMs; }

private:
    std::string serverAddress;
    std::string serverName;
    std::atomic<bool> running{false};
    std::atomic<bool> authRequired{false};
    std::atomic<size_t> maxMessageSize{1024 * 1024}; // 1MB
    std::atomic<int> clientTimeout{60000}; // 60 seconds
    int maxClients;
    
    void* serverSocket;
    std::map<std::string, void*> clientConnections;
    std::map<std::string, ClientInfo> clients;
    std::map<std::string, std::vector<ClientHandler>> handlers;
    
    ConnectionHandler connectionHandler;
    ErrorHandler errorHandler;
    std::function<bool(const std::string&, const std::string&)> authCallback;
    
    mutable std::mutex clientsMutex;
    mutable std::mutex handlersMutex;
    std::thread acceptThread;
    std::vector<std::thread> clientThreads;
    
    // Statistics
    struct {
        std::atomic<uint64_t> totalClients{0};
        std::atomic<uint64_t> totalMessagesReceived{0};
        std::atomic<uint64_t> totalMessagesSent{0};
        std::atomic<uint64_t> authFailures{0};
        std::atomic<uint64_t> errors{0};
        std::chrono::steady_clock::time_point startTime;
    } stats;
    
    void acceptClients();
    void handleClient(void* connection);
    void processClientMessage(void* connection, const std::string& data);
    void cleanupClient(const std::string& clientId);
    bool setupServerSocket();
    void cleanupServerSocket();
    void* acceptConnection();
    bool sendToConnection(void* connection, const std::string& data);
    std::string receiveFromConnection(void* connection, int timeoutMs = 100);
    void checkClientTimeouts();
    bool validateMessage(const IpcMessage& message) const;
};

} // namespace common

#endif // IPC_SERVER_H
