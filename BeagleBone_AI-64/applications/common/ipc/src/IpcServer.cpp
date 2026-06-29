#include "ipc/IpcManager.h"
#include "ipc/IpcMessage.h"
#include <iostream>

using namespace common;

// Simple client example
void ipcClientExample() {
    auto& ipc = IpcManager::getInstance();
    
    // Initialize with Unix socket
    ipc.initialize(IpcManager::TransportType::UNIX_SOCKET, "/tmp/myipc.sock");
    
    // Register message handler
    ipc.registerHandler("sensor/temperature", [](const IpcMessage& msg) {
        double temp = msg.getBody()["temperature"].asDouble();
        std::cout << "Temperature: " << temp << "°C" << std::endl;
    });
    
    // Send message
    Json::Value body;
    body["temperature"] = 25.5;
    body["humidity"] = 65.0;
    IpcMessage msg("sensor/data", body);
    ipc.sendMessage(msg);
    
    // Send request with response
    IpcMessage request("system/status", Json::objectValue);
    auto response = ipc.sendRequest(request, 5000);
    if (response.isValid()) {
        std::cout << "System status: " << response.getBody().toStyledString() << std::endl;
    }
}

// Server example
void ipcServerExample() {
    // Server side
    auto& ipc = IpcManager::getInstance();
    ipc.initialize(IpcManager::TransportType::UNIX_SOCKET, "/tmp/myipc.sock");
    
    // Handle incoming messages
    ipc.registerHandler("#", [](const IpcMessage& msg) {
        std::cout << "Received: " << msg.getTopic() << std::endl;
        
        // Send response for requests
        if (msg.getType() == IpcMessage::REQUEST) {
            IpcMessage response;
            response.setTopic(msg.getTopic());
            response.setType(IpcMessage::RESPONSE);
            response.setCorrelationId(msg.getRequestId());
            response.getBody()["status"] = "ok";
            IpcManager::getInstance().sendMessage(response);
        }
    });
    
    // Keep running
    std::this_thread::sleep_for(std::chrono::seconds(30));
}
