#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <json/json.h>
#include <yaml-cpp/yaml.h>

namespace common {

/**
 * @brief Configuration Manager for the application
 * 
 * Supports multiple config formats (JSON, YAML, INI)
 * with live reload and change notifications
 */
class ConfigManager {
public:
    using ChangeCallback = std::function<void(const std::string& key, const Json::Value& oldValue, const Json::Value& newValue)>;

    static ConfigManager& getInstance();
    
    /**
     * @brief Load configuration from file
     * @param filename Path to config file
     * @param format File format (json/yaml/ini)
     * @return true if load successful
     */
    bool loadConfig(const std::string& filename, const std::string& format = "json");
    
    /**
     * @brief Get configuration value as a specific type
     * @param key Dot-notation key path (e.g., "system.timeout")
     * @param defaultValue Default value if key not found
     * @return Config value
     */
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const;
    
    /**
     * @brief Set configuration value
     * @param key Dot-notation key path
     * @param value Value to set
     */
    template<typename T>
    void set(const std::string& key, const T& value);
    
    /**
     * @brief Register callback for config changes
     * @param key Key to watch (can include wildcards)
     * @param callback Function to call on changes
     */
    void watch(const std::string& key, ChangeCallback callback);
    
    /**
     * @brief Reload configuration from file
     */
    bool reload();
    
    /**
     * @brief Save current configuration to file
     * @param filename Output file path
     * @param format Output format
     */
    bool saveConfig(const std::string& filename, const std::string& format = "json");
    
    /**
     * @brief Enable/disable live reload
     */
    void enableLiveReload(bool enable);

private:
    ConfigManager();
    ~ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    Json::Value root;
    std::string configFile;
    std::string configFormat;
    mutable std::mutex mutex;
    std::map<std::string, std::vector<ChangeCallback>> watchers;
    bool liveReloadEnabled;
    int fileDescriptor;
    time_t lastModified;
    
    void parseYAML(const YAML::Node& node, Json::Value& json, const std::string& prefix = "");
    Json::Value getValue(const std::string& key) const;
    void setValue(const std::string& key, const Json::Value& value);
    void notifyWatchers(const std::string& key, const Json::Value& oldVal, const Json::Value& newVal);
    void watchFile();
    void checkForFileChanges();
};

} // namespace common

#endif // CONFIG_MANAGER_H
