#include "config/ConfigManager.h"
#include "config/ConfigSchema.h"
#include "config/ConfigTypes.h"

using namespace common;

// Example configuration schema definition
void setupConfiguration() {
    auto& config = ConfigManager::getInstance();
    auto& schema = ConfigSchema::getInstance();
    
    // Define schema rules
    std::vector<ConfigSchema::Rule> rules = {
        // System configuration
        ConfigSchema::Rule()
            .setKey("system.hostname")
            .setType(ConfigValueType::STRING)
            .setRequired(true)
            .setPattern("^[a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?$")
            .setDefault("bbb-ai64")
            .addTag("system")
            .addTag("network"),
            
        ConfigSchema::Rule()
            .setKey("system.timeout")
            .setType(ConfigValueType::INTEGER)
            .setMin(1000)
            .setMax(60000)
            .setDefault("5000")
            .addTag("system")
            .addTag("performance"),
            
        // Sensor configuration
        ConfigSchema::Rule()
            .setKey("sensors.imu.enabled")
            .setType(ConfigValueType::BOOLEAN)
            .setRequired(true)
            .setDefault("true")
            .addTag("sensor")
            .addTag("hardware"),
            
        ConfigSchema::Rule()
            .setKey("sensors.imu.sampling_rate")
            .setType(ConfigValueType::INTEGER)
            .setMin(1)
            .setMax(1000)
            .setDefault("100")
            .addTag("sensor")
            .addTag("performance"),
            
        ConfigSchema::Rule()
            .setKey("sensors.gps.enabled")
            .setType(ConfigValueType::BOOLEAN)
            .setDefault("true")
            .addTag("sensor")
            .addTag("hardware"),
            
        // Network configuration
        ConfigSchema::Rule()
            .setKey("network.wifi.ssid")
            .setType(ConfigValueType::STRING)
            .setRequired(false)
            .addTag("network")
            .addTag("wifi"),
            
        ConfigSchema::Rule()
            .setKey("network.wifi.password")
            .setType(ConfigValueType::STRING)
            .setSensitive(true)
            .setMinLength(8)
            .addTag("network")
            .addTag("wifi")
            .addTag("security"),
            
        ConfigSchema::Rule()
            .setKey("network.mqtt.broker")
            .setType(ConfigValueType::STRING)
            .setFormat("url")
            .setDefault("mqtt://localhost:1883")
            .addTag("network")
            .addTag("iot"),
            
        ConfigSchema::Rule()
            .setKey("network.mqtt.client_id")
            .setType(ConfigValueType::STRING)
            .setRequired(true)
            .addTag("network")
            .addTag("iot"),
            
        // Security configuration
        ConfigSchema::Rule()
            .setKey("security.ssl.enabled")
            .setType(ConfigValueType::BOOLEAN)
            .setDefault("true")
            .addTag("security"),
            
        ConfigSchema::Rule()
            .setKey("security.ssl.cert_file")
            .setType(ConfigValueType::STRING)
            .setRequired(false)
            .setSensitive(true)
            .addTag("security"),
            
        // Logging configuration
        ConfigSchema::Rule()
            .setKey("logging.level")
            .setType(ConfigValueType::STRING)
            .setEnum({"DEBUG", "INFO", "WARN", "ERROR", "FATAL"})
            .setDefault("INFO")
            .addTag("logging"),
            
        ConfigSchema::Rule()
            .setKey("logging.file_path")
            .setType(ConfigValueType::STRING)
            .setFormat("path")
            .setDefault("/var/log/app.log")
            .addTag("logging")
    };
    
    // Add rules to schema
    schema.addRules(rules);
    
    // Load configuration
    std::string configFile = "/etc/app/config.json";
    if (config.loadConfig(configFile, "json")) {
        std::cout << "Configuration loaded successfully" << std::endl;
    } else {
        std::cout << "Failed to load configuration, using defaults" << std::endl;
        config.loadConfig("", "json"); // Empty config loads defaults
    }
    
    // Validate configuration
    Json::Value configJson;
    // Get configuration as JSON (implementation omitted for brevity)
    ValidationResult result = schema.validate(configJson);
    if (!result.valid) {
        std::cerr << "Configuration validation failed: " 
                  << result.errorMessage << std::endl;
        return;
    }
    
    // Apply defaults for missing values
    schema.applyDefaults(configJson);
    
    // Use configuration values
    std::string hostname = config.get<std::string>("system.hostname", "localhost");
    int timeout = config.get<int>("system.timeout", 5000);
    bool imuEnabled = config.get<bool>("sensors.imu.enabled", true);
    int imuRate = config.get<int>("sensors.imu.sampling_rate", 100);
    std::string broker = config.get<std::string>("network.mqtt.broker", "mqtt://localhost:1883");
    std::string logLevel = config.get<std::string>("logging.level", "INFO");
    
    std::cout << "Hostname: " << hostname << std::endl;
    std::cout << "Timeout: " << timeout << "ms" << std::endl;
    std::cout << "IMU Enabled: " << (imuEnabled ? "yes" : "no") << std::endl;
    std::cout << "IMU Rate: " << imuRate << "Hz" << std::endl;
    std::cout << "MQTT Broker: " << broker << std::endl;
    std::cout << "Log Level: " << logLevel << std::endl;
    
    // Watch for configuration changes
    config.watch("system.*", [](const std::string& key, 
                               const Json::Value& oldVal, 
                               const Json::Value& newVal) {
        std::cout << "Config changed: " << key << std::endl;
        // Handle configuration change
    });
    
    // Save configuration
    config.saveConfig("/etc/app/config.updated.json", "json");
}

// Example of using configuration in a sensor service
class SensorService {
private:
    std::string devicePath;
    int samplingRate;
    bool enabled;
    
public:
    void initialize() {
        auto& config = ConfigManager::getInstance();
        
        // Load configuration with fallbacks
        enabled = config.get<bool>("sensors.imu.enabled", true);
        samplingRate = config.get<int>("sensors.imu.sampling_rate", 100);
        devicePath = config.get<std::string>("sensors.imu.device", "/dev/i2c-0");
        
        // Watch for runtime changes
        config.watch("sensors.imu.sampling_rate", [this](const std::string& key,
                                                         const Json::Value& oldVal,
                                                         const Json::Value& newVal) {
            if (newVal.isInt()) {
                this->samplingRate = newVal.asInt();
                this->onSamplingRateChanged();
            }
        });
    }
    
    void onSamplingRateChanged() {
        std::cout << "Sampling rate changed to: " << samplingRate << " Hz" << std::endl;
        // Reconfigure hardware
    }
};
