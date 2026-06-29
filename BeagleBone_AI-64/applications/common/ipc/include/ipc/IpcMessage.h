#ifndef IPC_MESSAGE_H
#define IPC_MESSAGE_H

#include <string>
#include <chrono>
#include <map>
#include <json/json.h>

namespace common {

/**
 * @brief IPC Message structure
 * 
 * Represents a message for IPC communication with
 * comprehensive metadata and payload support
 */
class IpcMessage {
public:
    /**
     * @brief Message types
     */
    enum Type {
        REQUEST = 1,        // Request message
        RESPONSE = 2,       // Response message
        NOTIFICATION = 3,   // Notification (fire and forget)
        ERROR = 4,          // Error message
        HEARTBEAT = 5,      // Heartbeat/keepalive
        SUBSCRIBE = 6,      // Subscription request
        UNSUBSCRIBE = 7,    // Unsubscription request
        ACK = 8,            // Acknowledgement
        NACK = 9,           // Negative acknowledgement
        PING = 10,          // Ping message
        PONG = 11,          // Pong response
        METRICS = 12,       // Metrics/statistics
        CONFIG = 13,        // Configuration update
        AUTH = 14,          // Authentication message
        STREAM = 15,        // Streaming data
        BATCH = 16          // Batch of messages
    };
    
    /**
     * @brief Message priority levels
     */
    enum Priority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    };
    
    /**
     * @brief Constructor
     */
    IpcMessage();
    
    /**
     * @brief Constructor with topic and body
     * @param topic Message topic
     * @param body Message body as JSON
     */
    IpcMessage(const std::string& topic, const Json::Value& body);
    
    /**
     * @brief Copy constructor
     */
    IpcMessage(const IpcMessage& other);
    
    /**
     * @brief Assignment operator
     */
    IpcMessage& operator=(const IpcMessage& other);
    
    /**
     * @brief Destructor
     */
    ~IpcMessage();
    
    // Getters and setters
    void setTopic(const std::string& topic);
    std::string getTopic() const;
    
    void setBody(const Json::Value& body);
    Json::Value getBody() const;
    
    void setType(int type);
    int getType() const;
    
    void setPriority(int priority);
    int getPriority() const;
    
    void setSender(const std::string& sender);
    std::string getSender() const;
    
    void setRecipient(const std::string& recipient);
    std::string getRecipient() const;
    
    void setCorrelationId(const std::string& correlationId);
    std::string getCorrelationId() const;
    
    void setRequestId(const std::string& requestId);
    std::string getRequestId() const;
    
    void setTimestamp(std::chrono::system_clock::time_point timestamp);
    std::chrono::system_clock::time_point getTimestamp() const;
    
    void setExpiry(std::chrono::system_clock::time_point expiry);
    std::chrono::system_clock::time_point getExpiry() const;
    
    void setTtl(int ttlMs);
    int getTtl() const;
    
    void setFlags(uint32_t flags);
    uint32_t getFlags() const;
    
    void setHeader(const std::string& key, const Json::Value& value);
    Json::Value getHeader(const std::string& key) const;
    std::map<std::string, Json::Value> getHeaders() const;
    
    void setMetadata(const std::string& key, const Json::Value& value);
    Json::Value getMetadata(const std::string& key) const;
    
    // Helper methods
    bool isRequest() const { return type == REQUEST; }
    bool isResponse() const { return type == RESPONSE; }
    bool isNotification() const { return type == NOTIFICATION; }
    bool isError() const { return type == ERROR; }
    bool isExpired() const;
    bool isValid() const;
    
    // Serialization
    std::string serialize() const;
    bool deserialize(const std::string& data);
    
    // Binary serialization (for efficient transfer)
    std::vector<uint8_t> serializeBinary() const;
    bool deserializeBinary(const std::vector<uint8_t>& data);
    
    // Convenience methods
    void setError(const std::string& code, const std::string& message);
    std::string getErrorCode() const;
    std::string getErrorMessage() const;
    
    void setSuccess(bool success);
    bool getSuccess() const;
    
    void setProgress(int percent);
    int getProgress() const;
    
    void setData(const std::string& key, const Json::Value& data);
    Json::Value getData(const std::string& key) const;
    
    // Type conversion methods
    std::string getBodyAsString() const;
    int getBodyAsInt() const;
    double getBodyAsDouble() const;
    bool getBodyAsBool() const;

private:
    std::string topic;
    Json::Value body;
    int type;
    int priority;
    std::string sender;
    std::string recipient;
    std::string correlationId;
    std::string requestId;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point expiry;
    int ttlMs;
    uint32_t flags;
    std::map<std::string, Json::Value> headers;
    std::map<std::string, Json::Value> metadata;
    
    void initDefaults();
};

} // namespace common

#endif // IPC_MESSAGE_H
