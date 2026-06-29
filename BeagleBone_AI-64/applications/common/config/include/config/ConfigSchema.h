#ifndef CONFIG_SCHEMA_H
#define CONFIG_SCHEMA_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <regex>
#include <json/json.h>
#include "ConfigTypes.h"

namespace common {

/**
 * @brief Configuration schema validation and management
 * 
 * Provides comprehensive schema validation for configuration
 * with support for complex validation rules and custom validators
 */
class ConfigSchema {
public:
    /**
     * @brief Validation rule for a configuration key
     */
    struct Rule {
        std::string key;                    // Key path (supports dot notation)
        ConfigValueType type;               // Expected value type
        bool required;                      // Is this key required?
        bool nullable;                      // Can this be null?
        std::vector<std::string> enumValues; // Allowed values
        std::string pattern;                // Regex pattern for string validation
        std::optional<int64_t> min;         // Minimum value (integer)
        std::optional<int64_t> max;         // Maximum value (integer)
        std::optional<double> minFloat;     // Minimum value (float)
        std::optional<double> maxFloat;     // Maximum value (float)
        std::optional<size_t> minLength;    // Minimum string length
        std::optional<size_t> maxLength;    // Maximum string length
        std::optional<size_t> minItems;     // Minimum array items
        std::optional<size_t> maxItems;     // Maximum array items
        std::optional<size_t> minProperties; // Minimum object properties
        std::optional<size_t> maxProperties; // Maximum object properties
        std::string format;                 // Format: "email", "url", "ip", "uuid", etc.
        std::string customMessage;          // Custom error message
        std::vector<Rule> nestedRules;      // Rules for nested objects
        std::map<std::string, Rule> properties; // Object property rules
        Rule arrayItemRule;                 // Rule for array items
        std::function<bool(const Json::Value&)> customValidator; // Custom validation function
        std::string dependsOn;              // This key depends on another key
        std::vector<std::string> requiredWith; // Required if this key is present
        std::vector<std::string> requiredWithout; // Required if this key is absent
        bool sensitive;                     // Sensitive data (password, etc.)
        bool readOnly;                      // Read-only configuration
        std::string defaultValue;           // Default value as string
        std::vector<std::string> tags;      // Tags for categorization
        
        // Constructor with defaults
        Rule() : type(ConfigValueType::STRING), required(false), nullable(false),
                sensitive(false), readOnly(false) {}
        
        // Builder pattern methods
        Rule& setKey(const std::string& k) { key = k; return *this; }
        Rule& setType(ConfigValueType t) { type = t; return *this; }
        Rule& setRequired(bool r) { required = r; return *this; }
        Rule& setNullable(bool n) { nullable = n; return *this; }
        Rule& setEnum(const std::vector<std::string>& values) { enumValues = values; return *this; }
        Rule& setPattern(const std::string& p) { pattern = p; return *this; }
        Rule& setMin(int64_t m) { min = m; return *this; }
        Rule& setMax(int64_t m) { max = m; return *this; }
        Rule& setMinFloat(double m) { minFloat = m; return *this; }
        Rule& setMaxFloat(double m) { maxFloat = m; return *this; }
        Rule& setMinLength(size_t m) { minLength = m; return *this; }
        Rule& setMaxLength(size_t m) { maxLength = m; return *this; }
        Rule& setFormat(const std::string& f) { format = f; return *this; }
        Rule& setSensitive(bool s) { sensitive = s; return *this; }
        Rule& setReadOnly(bool ro) { readOnly = ro; return *this; }
        Rule& setDefault(const std::string& d) { defaultValue = d; return *this; }
        Rule& addTag(const std::string& tag) { tags.push_back(tag); return *this; }
    };
    
    ConfigSchema();
    ~ConfigSchema();
    
    /**
     * @brief Add a validation rule
     * @param rule The rule to add
     * @return Reference to this schema
     */
    ConfigSchema& addRule(const Rule& rule);
    
    /**
     * @brief Add multiple rules at once
     * @param rules Vector of rules
     * @return Reference to this schema
     */
    ConfigSchema& addRules(const std::vector<Rule>& rules);
    
    /**
     * @brief Validate configuration against schema
     * @param config JSON configuration object
     * @param strict Strict validation (additional properties not allowed)
     * @return Validation result
     */
    ValidationResult validate(const Json::Value& config, bool strict = false) const;
    
    /**
     * @brief Validate configuration from file
     * @param configFile Path to configuration file
     * @param strict Strict validation
     * @return Validation result
     */
    ValidationResult validateFile(const std::string& configFile, bool strict = false) const;
    
    /**
     * @brief Validate a specific key
     * @param key Key path
     * @param value Value to validate
     * @return Validation result
     */
    ValidationResult validateKey(const std::string& key, const Json::Value& value) const;
    
    /**
     * @brief Get schema for a specific key
     * @param key Key path
     * @return Rule for the key, if exists
     */
    std::optional<Rule> getRule(const std::string& key) const;
    
    /**
     * @brief Check if key exists in schema
     * @param key Key path
     * @return true if key exists
     */
    bool hasKey(const std::string& key) const;
    
    /**
     * @brief Get all root-level keys
     * @return Vector of key names
     */
    std::vector<std::string> getKeys() const;
    
    /**
     * @brief Get all keys recursively
     * @param prefix Prefix for nested keys
     * @return Vector of full key paths
     */
    std::vector<std::string> getAllKeys(const std::string& prefix = "") const;
    
    /**
     * @brief Get keys by tag
     * @param tag Tag to filter by
     * @return Vector of keys with the tag
     */
    std::vector<std::string> getKeysByTag(const std::string& tag) const;
    
    /**
     * @brief Get sensitive keys
     * @return Vector of sensitive keys
     */
    std::vector<std::string> getSensitiveKeys() const;
    
    /**
     * @brief Get required keys
     * @return Vector of required keys
     */
    std::vector<std::string> getRequiredKeys() const;
    
    /**
     * @brief Apply default values to configuration
     * @param config Configuration to update
     * @param overwrite Overwrite existing values
     */
    void applyDefaults(Json::Value& config, bool overwrite = false) const;
    
    /**
     * @brief Generate default configuration
     * @return JSON object with default values
     */
    Json::Value generateDefaultConfig() const;
    
    /**
     * @brief Export schema to JSON schema format
     * @return JSON schema as string
     */
    std::string exportJsonSchema() const;
    
    /**
     * @brief Load schema from JSON schema file
     * @param schemaFile Path to JSON schema file
     * @return true if loaded successfully
     */
    bool loadJsonSchema(const std::string& schemaFile);
    
    /**
     * @brief Load schema from YAML file
     * @param schemaFile Path to YAML schema file
     * @return true if loaded successfully
     */
    bool loadYamlSchema(const std::string& schemaFile);
    
    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const { return lastError; }
    
    /**
     * @brief Clear all rules
     */
    void clear();
    
    /**
     * @brief Get schema version
     * @return Schema version string
     */
    std::string getVersion() const { return version; }
    
    /**
     * @brief Set schema version
     * @param ver Version string
     */
    void setVersion(const std::string& ver) { version = ver; }
    
    /**
     * @brief Enable/disable strict validation
     * @param strict Enable strict mode
     */
    void setStrict(bool strict) { strictMode = strict; }
    
    /**
     * @brief Register custom type validator
     * @param typeName Name of the custom type
     * @param validator Validation function
     */
    void registerCustomType(const std::string& typeName, 
                           std::function<bool(const Json::Value&)> validator);
    
    /**
     * @brief Get schema as string (JSON format)
     * @return JSON representation of schema
     */
    std::string toString() const;
    
    /**
     * @brief Validate dependencies between keys
     * @param config Configuration to validate
     * @return Validation result
     */
    ValidationResult validateDependencies(const Json::Value& config) const;

private:
    std::vector<Rule> rules;
    std::map<std::string, Rule> ruleMap;
    std::map<std::string, std::function<bool(const Json::Value&)>> customTypes;
    std::string version;
    bool strictMode;
    mutable std::string lastError;
    
    // Internal validation methods
    ValidationResult validateNode(const Json::Value& value, const Rule& rule, 
                                 const std::string& path) const;
    ValidationResult validateType(const Json::Value& value, ConfigValueType type) const;
    ValidationResult validateEnum(const Json::Value& value, const std::vector<std::string>& enumValues) const;
    ValidationResult validatePattern(const std::string& value, const std::string& pattern) const;
    ValidationResult validateRange(const Json::Value& value, const Rule& rule) const;
    ValidationResult validateFormat(const std::string& value, const std::string& format) const;
    ValidationResult validateArray(const Json::Value& array, const Rule& rule, 
                                  const std::string& path) const;
    ValidationResult validateObject(const Json::Value& object, const Rule& rule, 
                                   const std::string& path, bool strict) const;
    ValidationResult validateDependenciesNode(const Json::Value& config, 
                                            const Rule& rule, 
                                            const std::string& path) const;
    ValidationResult validateCustomType(const Json::Value& value, const std::string& typeName) const;
    
    // Helper methods
    bool isFormatValid(const std::string& value, const std::string& format) const;
    std::string getTypeName(ConfigValueType type) const;
    Json::Value getDefaultValue(const Rule& rule) const;
    ConfigValueType inferType(const Json::Value& value) const;
    void buildRuleMap();
    std::vector<std::string> splitKey(const std::string& key) const;
    const Rule* findRule(const std::string& key, const std::string& path = "") const;
    bool matchPattern(const std::string& value, const std::string& pattern) const;
    void collectKeys(const Rule& rule, const std::string& prefix, 
                    std::vector<std::string>& keys) const;
};

} // namespace common

#endif // CONFIG_SCHEMA_H
