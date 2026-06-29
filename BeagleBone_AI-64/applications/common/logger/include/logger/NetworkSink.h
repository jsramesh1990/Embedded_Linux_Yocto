#ifndef NETWORK_SINK_H
#define NETWORK_SINK_H

#include "LogSink.h"
#include <string>
#include <mutex>
#include <thread>
#include <queue>
#include <atomic>

namespace common {

/**
 * @brief Network log sink
 * 
 * Sends log messages to a remote server via TCP/UDP
 * Supports JSON, Syslog, and custom formats
 */
class NetworkSink : public LogSink {
public:
    enum class Protocol {
        TCP,
        UDP,
        SYSLOG,
        HTTP,
        HTTPS,
        WEBSOCKET
    };
    
    NetworkSink();
    virtual ~NetworkSink();
    
    /**
     * @brief Initialize network sink
     * @param config Configuration JSON
     *        - protocol: "tcp", "udp", "syslog", "http", "https", "websocket"
     *        - host: Remote host
     *        - port: Remote port (default: 514 for syslog, 80 for http)
     *        - format: "json", "syslog", "custom"
     *        - batch: Batch send messages (default: false)
     *        - batch_size: Messages per batch (default: 100)
     *        - retry_attempts: Number of retries (default: 3)
     *        - retry_delay: Delay between retries in ms (default: 1000)
     */
    virtual bool init(const Json::Value& config) override;
    
    /**
     * @brief Write log message to network
     * @param level Log level
     * @param message Formatted message
     * @param context Additional context
     */
    virtual void write(LogLevel level, const std::string& message,
                      const Json::Value& context = Json::nullValue) override;
    
    /**
     * @brief Flush any queued messages
     */
    virtual void flush() override;
    
    /**
     * @brief Set protocol
     */
    void setProtocol(Protocol protocol) { this->protocol = protocol; }
    
    /**
     * @brief Set remote host
     */
    void setHost(const std::string& host) { this->host = host; }
    
    /**
     * @brief Set remote port
     */
    void setPort(int port) { this->port = port; }
    
    /**
     * @brief Enable/disable batching
     */
    void setBatch(bool batch) { this->batch = batch; }
    
    /**
     * @brief Set batch size
     */
    void setBatchSize(size_t size) { batchSize = size; }

private:
    Protocol protocol = Protocol::TCP;
    std::string host = "localhost";
    int port = 514;
    std::string format = "json";
    bool batch = false;
    size_t batchSize = 100;
    int retryAttempts = 3;
    int retryDelay = 1000;
    bool connected = false;
    
    std::mutex mutex;
    std::queue<std::string> queue;
    std::atomic<bool> running{false};
    std::thread workerThread;
    
    void* socketHandle;
    
    bool connect();
    void disconnect();
    void sendData(const std::string& data);
    void processQueue();
    std::string formatMessage(const std::string& message, const Json::Value& context);
    std::string serialize(const Json::Value& json);
};

} // namespace common

#endif // NETWORK_SINK_H
