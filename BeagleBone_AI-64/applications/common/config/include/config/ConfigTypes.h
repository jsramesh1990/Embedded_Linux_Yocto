#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <cstdint>
#include <variant>
#include <optional>

namespace common {

/**
 * @brief Configuration value types supported by the system
 */
enum class ConfigValueType {
    STRING,
    INTEGER,
    FLOAT,
    BOOLEAN,
    ARRAY,
    OBJECT,
    NULL_VALUE,
    TIMESTAMP,
    DURATION,
    BYTES,
    IP_ADDRESS,
    URL,
    EMAIL,
    PHONE,
    REGEX,
    PATH,
    ENUM
};

/**
 * @brief Configuration value variant
 */
using ConfigValue = std::variant<
    std::string,
    int64_t,
    double,
    bool,
    std::vector<ConfigValue>,
    std::map<std::string, ConfigValue>,
    std::nullptr_t
>;

/**
 * @brief Configuration metadata
 */
struct ConfigMetadata {
    std::string description;
    std::string defaultValue;
    ConfigValueType type;
    bool required;
    bool sensitive;      // For passwords, keys, etc.
    bool readOnly;
    std::string minValue;
    std::string maxValue;
    std::vector<std::string> enumValues;
    std::string pattern;    // Regex pattern for validation
    std::vector<std::string> tags;
    int priority;          // For merging configurations
};

/**
 * @brief Configuration source types
 */
enum class ConfigSource {
    DEFAULT,
    FILE,
    ENVIRONMENT,
    COMMAND_LINE,
    REMOTE,
    RUNTIME,
    USER
};

/**
 * @brief Configuration change event
 */
struct ConfigChangeEvent {
    std::string key;
    ConfigValue oldValue;
    ConfigValue newValue;
    ConfigSource source;
    std::chrono::system_clock::time_point timestamp;
    std::string userId;
    bool isRollback;
};

/**
 * @brief Configuration validation result
 */
struct ValidationResult {
    bool valid;
    std::string errorMessage;
    std::string errorPath;
    std::vector<std::string> warnings;
    
    ValidationResult() : valid(true) {}
    ValidationResult(bool valid, const std::string& error = "") 
        : valid(valid), errorMessage(error) {}
    
    static ValidationResult success() {
        return ValidationResult(true);
    }
    
    static ValidationResult failure(const std::string& error, const std::string& path = "") {
        ValidationResult result(false, error);
        result.errorPath = path;
        return result;
    }
};

/**
 * @brief Configuration section
 */
struct ConfigSection {
    std::string name;
    std::string description;
    std::vector<std::string> keys;
    std::map<std::string, ConfigMetadata> metadata;
    std::vector<ConfigSection> subsections;
};

/**
 * @brief Configuration profile
 */
struct ConfigProfile {
    std::string name;
    std::string description;
    std::map<std::string, ConfigValue> values;
    std::vector<std::string> inheritedProfiles;
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point modified;
    std::string createdBy;
    bool active;
};

/**
 * @brief Configuration encryption settings
 */
struct ConfigEncryption {
    bool enabled;
    std::string algorithm;  // "AES-256-GCM", "ChaCha20-Poly1305"
    std::string keyId;
    std::vector<std::string> encryptedFields;
    bool requireMasterKey;
};

/**
 * @brief Configuration backup
 */
struct ConfigBackup {
    std::string id;
    std::chrono::system_clock::time_point timestamp;
    std::string description;
    std::map<std::string, ConfigValue> values;
    ConfigSource source;
    std::string userId;
};

/**
 * @brief Configuration diff
 */
struct ConfigDiff {
    struct Change {
        std::string key;
        ConfigValue oldValue;
        ConfigValue newValue;
        bool isAdded;
        bool isRemoved;
        bool isModified;
    };
    
    std::vector<Change> changes;
    bool hasChanges() const { return !changes.empty(); }
};

/**
 * @brief Configuration watcher filter
 */
struct ConfigWatcherFilter {
    std::string keyPattern;        // Supports wildcards and regex
    std::vector<std::string> tags;
    ConfigSource source;
    bool includeSubkeys;
    bool onlySensitive;
    std::string userId;
    std::chrono::seconds minInterval;
};

/**
 * @brief Configuration import/export format
 */
enum class ConfigFormat {
    JSON,
    YAML,
    XML,
    TOML,
    INI,
    PROPERTIES,
    ENV,
    CSV
};

/**
 * @brief Configuration scope
 */
enum class ConfigScope {
    GLOBAL,
    SYSTEM,
    USER,
    SESSION,
    APPLICATION,
    COMPONENT
};

/**
 * @brief Configuration value with metadata
 */
struct ConfigEntry {
    ConfigValue value;
    ConfigMetadata metadata;
    ConfigSource source;
    ConfigScope scope;
    std::chrono::system_clock::time_point lastModified;
    std::chrono::system_clock::time_point expires;
    std::string modifiedBy;
    int version;
    bool isValid;
    
    ConfigEntry() : version(1), isValid(true) {}
};

/**
 * @brief Configuration query
 */
struct ConfigQuery {
    std::string key;
    bool recursive;
    bool includeMetadata;
    bool includeSource;
    ConfigValueType expectedType;
    std::vector<std::string> tags;
    ConfigScope scope;
    std::string version;
    std::chrono::milliseconds timeout;
    
    ConfigQuery() : recursive(true), includeMetadata(false), 
                   includeSource(false), timeout(std::chrono::seconds(5)) {}
};

/**
 * @brief Configuration statistics
 */
struct ConfigStats {
    size_t totalKeys;
    size_t modifiedKeys;
    size_t defaultKeys;
    size_t sensitiveKeys;
    size_t invalidKeys;
    size_t watchedKeys;
    std::chrono::system_clock::time_point lastLoadTime;
    std::chrono::system_clock::time_point lastSaveTime;
    size_t loadCount;
    size_t saveCount;
    size_t validationErrors;
    std::map<ConfigSource, size_t> sourceCounts;
    std::map<ConfigScope, size_t> scopeCounts;
};

/**
 * @brief Configuration permission
 */
struct ConfigPermission {
    std::string keyPattern;
    bool canRead;
    bool canWrite;
    bool canDelete;
    bool canWatch;
    std::vector<std::string> allowedUsers;
    std::vector<std::string> allowedGroups;
    std::vector<std::string> allowedRoles;
};

} // namespace common

#endif // CONFIG_TYPES_H
