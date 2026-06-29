#include "logger/SyslogSink.h"
#include <syslog.h>
#include <iostream>

using namespace common;

SyslogSink::SyslogSink() {
    name = "SyslogSink";
}

SyslogSink::~SyslogSink() {
    closeLog();
}

bool SyslogSink::init(const Json::Value& config) {
    if (config.isMember("ident")) {
        ident = config["ident"].asString();
    }
    if (config.isMember("facility")) {
        std::string facility = config["facility"].asString();
        if (facility == "user") facility = LOG_USER;
        else if (facility == "local0") facility = LOG_LOCAL0;
        else if (facility == "local1") facility = LOG_LOCAL1;
        else if (facility == "local2") facility = LOG_LOCAL2;
        else if (facility == "local3") facility = LOG_LOCAL3;
        else if (facility == "local4") facility = LOG_LOCAL4;
        else if (facility == "local5") facility = LOG_LOCAL5;
        else if (facility == "local6") facility = LOG_LOCAL6;
        else if (facility == "local7") facility = LOG_LOCAL7;
        else facility = LOG_USER;
        this->facility = facility;
    }
    
    openLog();
    return true;
}

void SyslogSink::openLog() {
    if (initialized) return;
    
    openlog(ident.c_str(), LOG_PID | LOG_NDELAY, facility);
    initialized = true;
}

void SyslogSink::closeLog() {
    if (initialized) {
        closelog();
        initialized = false;
    }
}

void SyslogSink::write(LogLevel level, const std::string& message, const Json::Value& context) {
    if (!shouldLog(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!initialized) {
        openLog();
    }
    
    int priority = logLevelToSyslog(level);
    syslog(priority, "%s", message.c_str());
}
