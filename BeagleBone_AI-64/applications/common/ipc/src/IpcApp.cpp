#include "ipc/IpcManager.h"
#include "ipc/IpcMessage.h"
#include "ipc/IpcClient.h"
#include "ipc/IpcServer.h"
#include "logger/Logger.h"
#include "config/ConfigManager.h"

class ApplicationIPC {
private:
    std::unique_ptr<IpcServer> server;
    std::unique_ptr<IpcClient> client;
    bool isServerMode;
    
public:
    ApplicationIPC(bool serverMode = false) : isServerMode(serverMode) {
        auto& config = ConfigManager::getInstance();
        std::string ipcAddress = config.get<std::string>("ipc.address", "/tmp/app.sock");
        
        if (serverMode) {
            // Server mode
            server = std::make_unique<IpcServer>(ipcAddress, "AppServer");
            setupServer();
        } else {
            // Client mode
            client = std::make_unique<IpcClient>(ipcAddress, config.get<std::string>("ipc.client_id", "client-001"));
            setupClient();
        }
    }
    
    void setupServer() {
        server->setConnectionHandler([](const std::string& clientId, bool connected) {
            LOG_INFO("Client " + clientId + " " + (connected ? "connected" : "disconnected"));
        });
        
        // Register all message handlers
        server->registerHandler("sensor/*", [this](const std::string& clientId, const IpcMessage& msg) {
            handleSensorData(clientId, msg);
        });
        
        server->registerHandler("system/*", [this](const std::string& clientId, const IpcMessage& msg) {
            handleSystemCommand(clientId, msg);
        });
        
        server->registerHandler("config/*", [this](const std::string& clientId, const IpcMessage& msg) {
            handleConfigRequest(clientId, msg);
        });
        
        // Start server
        if (!server->start(50)) {
            LOG_ERROR("Failed to start IPC server");
        }
    }
    
    void setupClient() {
        client->setConnectionCallback([](bool connected) {
            LOG_INFO("IPC " + std::string(connected ? "connected" : "disconnected"));
        });
        
        client->setErrorCallback([](const std::string& error) {
            LOG_ERROR("IPC error: " + error);
        });
        
        client->setAutoReconnect(true, 5000, -1);
        
        // Subscribe to messages
        client->subscribe("sensor/#", [this](const IpcMessage& msg) {
            handleSensorUpdate(msg);
        });
        
        client->subscribe("system/#", [this](const IpcMessage& msg) {
            handleSystemUpdate(msg);
        });
        
        // Connect to server
        if (!client->connect()) {
            LOG_ERROR("Failed to connect to IPC server");
        }
    }
    
    void handleSensorData(const std::string& clientId, const IpcMessage& msg) {
        std::string sensorType = msg.getBody()["type"].asString();
        Json::Value data = msg.getBody()["data"];
        
        LOG_INFO("Sensor data from " + clientId + ": " + sensorType);
        
        // Process sensor data
        // Store in database, update GUI, etc.
        
        // Broadcast to all clients
        server->broadcast(msg, clientId);
    }
    
    void handleSystemCommand(const std::string& clientId, const IpcMessage& msg) {
        std::string command = msg.getBody()["command"].asString();
        LOG_INFO("System command from " + clientId + ": " + command);
        
        // Execute command
        if (command == "restart") {
            // Restart application
        } else if (command == "shutdown") {
            // Shutdown
        }
        
        // Send response
        IpcMessage response;
        response.setTopic("system/response");
        response.setCorrelationId(msg.getRequestId());
        response.getBody()["status"] = "ok";
        response.getBody()["command"] = command;
        server->sendToClient(clientId, response);
    }
    
    void handleConfigRequest(const std::string& clientId, const IpcMessage& msg) {
        auto& config = ConfigManager::getInstance();
        std::string action = msg.getBody()["action"].asString();
        
        IpcMessage response;
        response.setTopic("config/response");
        response.setCorrelationId(msg.getRequestId());
        
        if (action == "get") {
            std::string key = msg.getBody()["key"].asString();
            response.getBody()["value"] = config.get<std::string>(key, "");
        } else if (action == "set") {
            std::string key = msg.getBody()["key"].asString();
            std::string value = msg.getBody()["value"].asString();
            config.set(key, value);
            response.getBody()["status"] = "ok";
        }
        
        server->sendToClient(clientId, response);
    }
    
    void handleSensorUpdate(const IpcMessage& msg) {
        // Update local sensor cache
        // Update GUI
        // Trigger alerts if needed
    }
    
    void handleSystemUpdate(const IpcMessage& msg) {
        // Handle system-wide updates
    }
    
    void sendSensorData(const std::string& sensorType, const Json::Value& data) {
        if (client) {
            IpcMessage msg("sensor/" + sensorType, Json::objectValue);
            msg.getBody()["type"] = sensorType;
            msg.getBody()["data"] = data;
            msg.getBody()["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            client->send(msg);
        }
    }
};
