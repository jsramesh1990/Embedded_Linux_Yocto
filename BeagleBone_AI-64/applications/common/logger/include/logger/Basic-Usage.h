#include "logger/Logger.h"
#include <iostream>

using namespace common;

void basicExample() {
    auto& logger = Logger::getInstance();
    
    // Initialize with default console sink
    logger.initialize();
    
    // Set log level
    logger.setLevel(LogLevel::DEBUG);
    
    // Simple logging
    logger.info("Application started");
    logger.debug("Debug message with value: " + std::to_string(42));
    logger.warn("Warning: something might be wrong");
    logger.error("Error occurred");
    
    // Logging with context
    Json::Value context;
    context["user"] = "admin";
    context["action"] = "login";
    logger.info("User logged in", context);
    
    // Structured logging
    logger.trace("Trace message");
    logger.notice("Notice message");
    logger.critical("Critical error!");
}
