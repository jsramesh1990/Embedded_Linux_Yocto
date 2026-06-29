#include "config/ConfigSchema.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <algorithm>
#include <yaml-cpp/yaml.h>

using namespace common;

ConfigSchema::ConfigSchema() : version("1.0"), strictMode(false) {
    // Register built-in formats
    customTypes["email"] = [](const Json::Value& value) -> bool {
        if (!value.isString()) return false;
        std::regex emailRegex(R"(([\w\.\-_]+)@([\w\.\-_]+)\.([a-zA-Z]{2,}))");
        return std::regex_match(value.asString(), emailRegex);
    };
    
    customTypes["url"] = [](const Json::Value& value) -> bool {
        if (!value.isString()) return false;
        std::regex urlRegex(R"(^(https?|ftp)://[^\s/$.?#].[^\s]*$)");
        return std::regex_match(value.asString(), urlRegex);
    };
    
    customTypes["ip"] = [](const Json::Value& value) -> bool {
        if (!value.isString()) return false;
        std::regex ipRegex(R"(^(\d{1,3}\.){3}\d{1,3}$)");
        return std::regex_match(value.asString(), ipRegex);
    };
    
    customTypes["uuid"] = [](const Json::Value& value) -> bool {
        if (!value.isString()) return false;
        std::regex uuidRegex(R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})");
        return std::regex_match(value.asString(), uuidRegex);
    };
}

ConfigSchema::~ConfigSchema() {}

ConfigSchema& ConfigSchema::addRule(const Rule& rule) {
    rules.push_back(rule);
    buildRuleMap();
    return *this;
}

ConfigSchema& ConfigSchema::addRules(const std::vector<Rule>& newRules) {
    for (const auto& rule : newRules) {
        rules.push_back(rule);
    }
    buildRuleMap();
    return *this;
}

void ConfigSchema::buildRuleMap() {
    ruleMap.clear();
    for (const auto& rule : rules) {
        ruleMap[rule.key] = rule;
        
        // Also add nested rules
        for (const auto& nestedRule : rule.nestedRules) {
            std::string fullKey = rule.key + "." + nestedRule.key;
            ruleMap[fullKey] = nestedRule;
        }
        
        // Add property rules
        for (const auto& [propKey, propRule] : rule.properties) {
            std::string fullKey = rule.key + "." + propKey;
            ruleMap[fullKey] = propRule;
        }
    }
}

ValidationResult ConfigSchema::validate(const Json::Value& config, bool strict) const {
    lastError.clear();
    
    // Validate each rule
    for (const auto& rule : rules) {
        // Check if key exists in config
        bool hasKey = false;
        Json::Value value;
        
        if (config.isObject()) {
            // For root-level keys, check directly
            if (config.isMember(rule.key)) {
                hasKey = true;
                value = config[rule.key];
            }
        } else {
            // For nested keys, traverse
            std::vector<std::string> parts = splitKey(rule.key);
            const Json::Value* current = &config;
            
            for (size_t i = 0; i < parts.size(); ++i) {
                if (!current->isObject() || !current->isMember(parts[i])) {
                    hasKey = false;
                    break;
                }
                if (i == parts.size() - 1) {
                    hasKey = true;
                    value = (*current)[parts[i]];
                } else {
                    current = &((*current)[parts[i]]);
                }
            }
        }
        
        if (!hasKey) {
            if (rule.required) {
                return ValidationResult::failure(
                    "Required key '" + rule.key + "' is missing",
                    rule.key
                );
            }
            continue;
        }
        
        // Validate the value
        ValidationResult result = validateNode(value, rule, rule.key);
        if (!result.valid) {
            return result;
        }
    }
    
    // Validate dependencies
    if (strict) {
        ValidationResult depResult = validateDependencies(config);
        if (!depResult.valid) {
            return depResult;
        }
    }
    
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validateNode(const Json::Value& value, const Rule& rule, 
                                           const std::string& path) const {
    // Check if null
    if (value.isNull()) {
        if (rule.nullable) {
            return ValidationResult::success();
        }
        return ValidationResult::failure("Value cannot be null", path);
    }
    
    // Validate type
    ValidationResult typeResult = validateType(value, rule.type);
    if (!typeResult.valid) {
        return typeResult;
    }
    
    // Validate enum
    if (!rule.enumValues.empty()) {
        ValidationResult enumResult = validateEnum(value, rule.enumValues);
        if (!enumResult.valid) {
            return enumResult;
        }
    }
    
    // Validate pattern (for strings)
    if (value.isString() && !rule.pattern.empty()) {
        ValidationResult patternResult = validatePattern(value.asString(), rule.pattern);
        if (!patternResult.valid) {
            return patternResult;
        }
    }
    
    // Validate range
    ValidationResult rangeResult = validateRange(value, rule);
    if (!rangeResult.valid) {
        return rangeResult;
    }
    
    // Validate format (for strings)
    if (value.isString() && !rule.format.empty()) {
        ValidationResult formatResult = validateFormat(value.asString(), rule.format);
        if (!formatResult.valid) {
            return formatResult;
        }
    }
    
    // Validate array items
    if (value.isArray() && rule.arrayItemRule.type != ConfigValueType::NULL_VALUE) {
        ValidationResult arrayResult = validateArray(value, rule, path);
        if (!arrayResult.valid) {
            return arrayResult;
        }
    }
    
    // Validate object properties
    if (value.isObject() && !rule.properties.empty()) {
        ValidationResult objectResult = validateObject(value, rule, path, strictMode);
        if (!objectResult.valid) {
            return objectResult;
        }
    }
    
    // Custom validator
    if (rule.customValidator) {
        try {
            if (!rule.customValidator(value)) {
                return ValidationResult::failure(
                    rule.customMessage.empty() ? "Custom validation failed" : rule.customMessage,
                    path
                );
            }
        } catch (const std::exception& e) {
            return ValidationResult::failure("Custom validator threw exception: " + std::string(e.what()), path);
        }
    }
    
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validateType(const Json::Value& value, ConfigValueType type) const {
    bool valid = false;
    
    switch (type) {
        case ConfigValueType::STRING:
            valid = value.isString();
            break;
        case ConfigValueType::INTEGER:
            valid = value.isInt64();
            break;
        case ConfigValueType::FLOAT:
            valid = value.isDouble();
            break;
        case ConfigValueType::BOOLEAN:
            valid = value.isBool();
            break;
        case ConfigValueType::ARRAY:
            valid = value.isArray();
            break;
        case ConfigValueType::OBJECT:
            valid = value.isObject();
            break;
        case ConfigValueType::NULL_VALUE:
            valid = value.isNull();
            break;
        default:
            valid = true; // Unknown type, skip validation
            break;
    }
    
    if (!valid) {
        return ValidationResult::failure(
            "Expected type '" + getTypeName(type) + "' but got '" + 
            std::string(value.isString() ? "string" : 
                       value.isInt64() ? "integer" :
                       value.isDouble() ? "float" :
                       value.isBool() ? "boolean" :
                       value.isArray() ? "array" :
                       value.isObject() ? "object" : "unknown") + "'"
        );
    }
    
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validateEnum(const Json::Value& value, 
                                           const std::vector<std::string>& enumValues) const {
    if (!value.isString()) {
        return ValidationResult::failure("Value must be a string for enum validation");
    }
    
    std::string strValue = value.asString();
    if (std::find(enumValues.begin(), enumValues.end(), strValue) == enumValues.end()) {
        std::string validValues;
        for (size_t i = 0; i < enumValues.size(); ++i) {
            if (i > 0) validValues += ", ";
            validValues += enumValues[i];
        }
        return ValidationResult::failure(
            "Value '" + strValue + "' is not in allowed values: " + validValues
        );
    }
    
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validatePattern(const std::string& value, 
                                              const std::string& pattern) const {
    if (!matchPattern(value, pattern)) {
        return ValidationResult::failure(
            "Value '" + value + "' does not match pattern: " + pattern
        );
    }
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validateRange(const Json::Value& value, const Rule& rule) const {
    if (value.isInt64()) {
        int64_t numValue = value.asInt64();
        if (rule.min.has_value() && numValue < rule.min.value()) {
            return ValidationResult::failure(
                "Value " + std::to_string(numValue) + " is less than minimum " + 
                std::to_string(rule.min.value())
            );
        }
        if (rule.max.has_value() && numValue > rule.max.value()) {
            return ValidationResult::failure(
                "Value " + std::to_string(numValue) + " is greater than maximum " + 
                std::to_string(rule.max.value())
            );
        }
    }
    
    if (value.isDouble()) {
        double numValue = value.asDouble();
        if (rule.minFloat.has_value() && numValue < rule.minFloat.value()) {
            return ValidationResult::failure(
                "Value " + std::to_string(numValue) + " is less than minimum " + 
                std::to_string(rule.minFloat.value())
            );
        }
        if (rule.maxFloat.has_value() && numValue > rule.maxFloat.value()) {
            return ValidationResult::failure(
                "Value " + std::to_string(numValue) + " is greater than maximum " + 
                std::to_string(rule.maxFloat.value())
            );
        }
    }
    
    if (value.isString()) {
        std::string strValue = value.asString();
        if (rule.minLength.has_value() && strValue.length() < rule.minLength.value()) {
            return ValidationResult::failure(
                "String length " + std::to_string(strValue.length()) + 
                " is less than minimum " + std::to_string(rule.minLength.value())
            );
        }
        if (rule.maxLength.has_value() && strValue.length() > rule.maxLength.value()) {
            return ValidationResult::failure(
                "String length " + std::to_string(strValue.length()) + 
                " is greater than maximum " + std::to_string(rule.maxLength.value())
            );
        }
    }
    
    if (value.isArray()) {
        if (rule.minItems.has_value() && value.size() < rule.minItems.value()) {
            return ValidationResult::failure(
                "Array size " + std::to_string(value.size()) + 
                " is less than minimum " + std::to_string(rule.minItems.value())
            );
        }
        if (rule.maxItems.has_value() && value.size() > rule.maxItems.value()) {
            return ValidationResult::failure(
                "Array size " + std::to_string(value.size()) + 
                " is greater than maximum " + std::to_string(rule.maxItems.value())
            );
        }
    }
    
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validateFormat(const std::string& value, const std::string& format) const {
    auto it = customTypes.find(format);
    if (it != customTypes.end()) {
        Json::Value jsonValue(value);
        if (!it->second(jsonValue)) {
            return ValidationResult::failure("Invalid " + format + " format: " + value);
        }
        return ValidationResult::success();
    }
    
    // Built-in format validation
    if (!isFormatValid(value, format)) {
        return ValidationResult::failure("Invalid " + format + " format: " + value);
    }
    
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validateArray(const Json::Value& array, const Rule& rule, 
                                            const std::string& path) const {
    if (!array.isArray()) {
        return ValidationResult::failure("Expected array", path);
    }
    
    for (Json::ArrayIndex i = 0; i < array.size(); ++i) {
        std::string itemPath = path + "[" + std::to_string(i) + "]";
        ValidationResult result = validateNode(array[i], rule.arrayItemRule, itemPath);
        if (!result.valid) {
            return result;
        }
    }
    
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validateObject(const Json::Value& object, const Rule& rule, 
                                             const std::string& path, bool strict) const {
    if (!object.isObject()) {
        return ValidationResult::failure("Expected object", path);
    }
    
    std::set<std::string> validKeys;
    for (const auto& [key, propRule] : rule.properties) {
        validKeys.insert(key);
        
        if (object.isMember(key)) {
            std::string propPath = path + "." + key;
            ValidationResult result = validateNode(object[key], propRule, propPath);
            if (!result.valid) {
                return result;
            }
        } else if (propRule.required) {
            return ValidationResult::failure("Required property '" + key + "' missing", path + "." + key);
        }
    }
    
    // Check for additional properties in strict mode
    if (strict) {
        std::vector<std::string> keys = object.getMemberNames();
        for (const auto& key : keys) {
            if (validKeys.find(key) == validKeys.end()) {
                return ValidationResult::failure("Additional property '" + key + "' not allowed", path + "." + key);
            }
        }
    }
    
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validateDependencies(const Json::Value& config) const {
    for (const auto& rule : rules) {
        ValidationResult result = validateDependenciesNode(config, rule, rule.key);
        if (!result.valid) {
            return result;
        }
    }
    return ValidationResult::success();
}

ValidationResult ConfigSchema::validateDependenciesNode(const Json::Value& config, 
                                                       const Rule& rule, 
                                                       const std::string& path) const {
    // Check if key exists
    bool hasKey = false;
    if (config.isObject() && config.isMember(rule.key)) {
        hasKey = true;
    }
    
    // Check requiredWith
    if (hasKey) {
        for (const auto& reqKey : rule.requiredWith) {
            bool hasReqKey = false;
            if (config.isObject() && config.isMember(reqKey)) {
                hasReqKey = true;
            }
            if (!hasReqKey) {
                return ValidationResult::failure(
                    "Key '" + rule.key + "' requires '" + reqKey + "' to be present"
                );
            }
        }
    }
    
    // Check requiredWithout
    if (!hasKey) {
        for (const auto& reqKey : rule.requiredWithout) {
            bool hasReqKey = false;
            if (config.isObject() && config.isMember(reqKey)) {
                hasReqKey = true;
            }
            if (!hasReqKey) {
                return ValidationResult::failure(
                    "Either '" + rule.key + "' or '" + reqKey + "' must be present"
                );
            }
        }
    }
    
    return ValidationResult::success();
}

bool ConfigSchema::matchPattern(const std::string& value, const std::string& pattern) const {
    try {
        std::regex regex(pattern);
        return std::regex_match(value, regex);
    } catch (const std::regex_error&) {
        return false;
    }
}

bool ConfigSchema::isFormatValid(const std::string& value, const std::string& format) const {
    if (format == "email") {
        std::regex emailRegex(R"(([\w\.\-_]+)@([\w\.\-_]+)\.([a-zA-Z]{2,}))");
        return std::regex_match(value, emailRegex);
    }
    if (format == "url") {
        std::regex urlRegex(R"(^(https?|ftp)://[^\s/$.?#].[^\s]*$)");
        return std::regex_match(value, urlRegex);
    }
    if (format == "ip") {
        std::regex ipRegex(R"(^(\d{1,3}\.){3}\d{1,3}$)");
        return std::regex_match(value, ipRegex);
    }
    if (format == "uuid") {
        std::regex uuidRegex(R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})");
        return std::regex_match(value, uuidRegex);
    }
    if (format == "hostname") {
        std::regex hostRegex(R"(^[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?)*$)");
        return std::regex_match(value, hostRegex);
    }
    if (format == "path") {
        // Simple path validation
        return !value.empty() && value.find("..") == std::string::npos;
    }
    return true;
}

std::string ConfigSchema::getTypeName(ConfigValueType type) const {
    switch (type) {
        case ConfigValueType::STRING: return "string";
        case ConfigValueType::INTEGER: return "integer";
        case ConfigValueType::FLOAT: return "float";
        case ConfigValueType::BOOLEAN: return "boolean";
        case ConfigValueType::ARRAY: return "array";
        case ConfigValueType::OBJECT: return "object";
        case ConfigValueType::NULL_VALUE: return "null";
        default: return "unknown";
    }
}

std::vector<std::string> ConfigSchema::splitKey(const std::string& key) const {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = key.find('.');
    
    while (end != std::string::npos) {
        parts.push_back(key.substr(start, end - start));
        start = end + 1;
        end = key.find('.', start);
    }
    
    parts.push_back(key.substr(start));
    return parts;
}

void ConfigSchema::applyDefaults(Json::Value& config, bool overwrite) const {
    for (const auto& rule : rules) {
        if (!rule.defaultValue.empty()) {
            std::vector<std::string> parts = splitKey(rule.key);
            Json::Value* current = &config;
            
            for (size_t i = 0; i < parts.size(); ++i) {
                if (i == parts.size() - 1) {
                    // Last part - check if key exists
                    if (!current->isMember(parts[i]) || overwrite) {
                        (*current)[parts[i]] = getDefaultValue(rule);
                    }
                } else {
                    // Create intermediate objects if needed
                    if (!current->isObject() || !current->isMember(parts[i])) {
                        (*current)[parts[i]] = Json::objectValue;
                    }
                    current = &((*current)[parts[i]]);
                }
            }
        }
    }
}

Json::Value ConfigSchema::getDefaultValue(const Rule& rule) const {
    if (rule.defaultValue.empty()) {
        // Return type-appropriate default
        switch (rule.type) {
            case ConfigValueType::STRING: return "";
            case ConfigValueType::INTEGER: return 0;
            case ConfigValueType::FLOAT: return 0.0;
            case ConfigValueType::BOOLEAN: return false;
            case ConfigValueType::ARRAY: return Json::arrayValue;
            case ConfigValueType::OBJECT: return Json::objectValue;
            case ConfigValueType::NULL_VALUE: return Json::nullValue;
            default: return Json::nullValue;
        }
    }
    
    // Try to parse the default value
    try {
        if (rule.type == ConfigValueType::INTEGER) {
            return std::stoll(rule.defaultValue);
        } else if (rule.type == ConfigValueType::FLOAT) {
            return std::stod(rule.defaultValue);
        } else if (rule.type == ConfigValueType::BOOLEAN) {
            return rule.defaultValue == "true" || rule.defaultValue == "1";
        } else {
            return rule.defaultValue;
        }
    } catch (...) {
        return Json::nullValue;
    }
}

std::vector<std::string> ConfigSchema::getKeys() const {
    std::vector<std::string> keys;
    for (const auto& rule : rules) {
        keys.push_back(rule.key);
    }
    return keys;
}

std::vector<std::string> ConfigSchema::getAllKeys(const std::string& prefix) const {
    std::vector<std::string> keys;
    for (const auto& rule : rules) {
        collectKeys(rule, prefix, keys);
    }
    return keys;
}

void ConfigSchema::collectKeys(const Rule& rule, const std::string& prefix, 
                              std::vector<std::string>& keys) const {
    std::string fullKey = prefix.empty() ? rule.key : prefix + "." + rule.key;
    keys.push_back(fullKey);
    
    for (const auto& nestedRule : rule.nestedRules) {
        collectKeys(nestedRule, fullKey, keys);
    }
    
    for (const auto& [propKey, propRule] : rule.properties) {
        collectKeys(propRule, fullKey, keys);
    }
}

std::vector<std::string> ConfigSchema::getKeysByTag(const std::string& tag) const {
    std::vector<std::string> keys;
    for (const auto& rule : rules) {
        if (std::find(rule.tags.begin(), rule.tags.end(), tag) != rule.tags.end()) {
            keys.push_back(rule.key);
        }
    }
    return keys;
}

std::vector<std::string> ConfigSchema::getSensitiveKeys() const {
    std::vector<std::string> keys;
    for (const auto& rule : rules) {
        if (rule.sensitive) {
            keys.push_back(rule.key);
        }
    }
    return keys;
}

std::vector<std::string> ConfigSchema::getRequiredKeys() const {
    std::vector<std::string> keys;
    for (const auto& rule : rules) {
        if (rule.required) {
            keys.push_back(rule.key);
        }
    }
    return keys;
}

Json::Value ConfigSchema::generateDefaultConfig() const {
    Json::Value config(Json::objectValue);
    applyDefaults(config, true);
    return config;
}

std::string ConfigSchema::exportJsonSchema() const {
    Json::Value schema;
    schema["$schema"] = "http://json-schema.org/draft-07/schema#";
    schema["title"] = "Configuration Schema";
    schema["version"] = version;
    schema["type"] = "object";
    schema["properties"] = Json::objectValue;
    
    for (const auto& rule : rules) {
        Json::Value prop;
        prop["type"] = getTypeName(rule.type);
        
        if (rule.required) {
            if (!schema.isMember("required")) {
                schema["required"] = Json::arrayValue;
            }
            schema["required"].append(rule.key);
        }
        
        if (!rule.enumValues.empty()) {
            Json::Value enumArray(Json::arrayValue);
            for (const auto& val : rule.enumValues) {
                enumArray.append(val);
            }
            prop["enum"] = enumArray;
        }
        
        if (!rule.pattern.empty()) {
            prop["pattern"] = rule.pattern;
        }
        
        if (rule.min.has_value()) {
            prop["minimum"] = rule.min.value();
        }
        if (rule.max.has_value()) {
            prop["maximum"] = rule.max.value();
        }
        
        if (rule.minLength.has_value()) {
            prop["minLength"] = Json::Value::UInt64(rule.minLength.value());
        }
        if (rule.maxLength.has_value()) {
            prop["maxLength"] = Json::Value::UInt64(rule.maxLength.value());
        }
        
        if (!rule.defaultValue.empty()) {
            try {
                if (rule.type == ConfigValueType::INTEGER) {
                    prop["default"] = std::stoll(rule.defaultValue);
                } else if (rule.type == ConfigValueType::FLOAT) {
                    prop["default"] = std::stod(rule.defaultValue);
                } else if (rule.type == ConfigValueType::BOOLEAN) {
                    prop["default"] = rule.defaultValue == "true" || rule.defaultValue == "1";
                } else {
                    prop["default"] = rule.defaultValue;
                }
            } catch (...) {
                prop["default"] = rule.defaultValue;
            }
        }
        
        schema["properties"][rule.key] = prop;
    }
    
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "  ";
    return Json::writeString(builder, schema);
}

bool ConfigSchema::loadJsonSchema(const std::string& schemaFile) {
    std::ifstream file(schemaFile);
    if (!file.is_open()) {
        lastError = "Failed to open schema file: " + schemaFile;
        return false;
    }
    
    Json::Value schema;
    Json::CharReaderBuilder builder;
    std::string errs;
    
    if (!Json::parseFromStream(builder, file, &schema, &errs)) {
        lastError = "Failed to parse JSON schema: " + errs;
        return false;
    }
    
    // Parse JSON schema and convert to internal rules
    if (schema.isMember("properties")) {
        Json::Value properties = schema["properties"];
        std::vector<std::string> requiredKeys;
        if (schema.isMember("required") && schema["required"].isArray()) {
            for (const auto& key : schema["required"]) {
                requiredKeys.push_back(key.asString());
            }
        }
        
        for (const auto& key : properties.getMemberNames()) {
            Json::Value prop = properties[key];
            Rule rule;
            rule.key = key;
            
            if (prop.isMember("type")) {
                std::string type = prop["type"].asString();
                if (type == "string") {
                    rule.type = ConfigValueType::STRING;
                    if (prop.isMember("pattern")) {
                        rule.pattern = prop["pattern"].asString();
                    }
                    if (prop.isMember("minLength")) {
                        rule.minLength = prop["minLength"].asUInt64();
                    }
                    if (prop.isMember("maxLength")) {
                        rule.maxLength = prop["maxLength"].asUInt64();
                    }
                } else if (type == "integer") {
                    rule.type = ConfigValueType::INTEGER;
                    if (prop.isMember("minimum")) {
                        rule.min = prop["minimum"].asInt64();
                    }
                    if (prop.isMember("maximum")) {
                        rule.max = prop["maximum"].asInt64();
                    }
                } else if (type == "number") {
                    rule.type = ConfigValueType::FLOAT;
                    if (prop.isMember("minimum")) {
                        rule.minFloat = prop["minimum"].asDouble();
                    }
                    if (prop.isMember("maximum")) {
                        rule.maxFloat = prop["maximum"].asDouble();
                    }
                } else if (type == "boolean") {
                    rule.type = ConfigValueType::BOOLEAN;
                } else if (type == "array") {
                    rule.type = ConfigValueType::ARRAY;
                    if (prop.isMember("items")) {
                        // Parse array items
                    }
                } else if (type == "object") {
                    rule.type = ConfigValueType::OBJECT;
                    if (prop.isMember("properties")) {
                        // Parse nested properties
                    }
                }
            }
            
            if (prop.isMember("enum") && prop["enum"].isArray()) {
                for (const auto& val : prop["enum"]) {
                    rule.enumValues.push_back(val.asString());
                }
            }
            
            if (prop.isMember("default")) {
                rule.defaultValue = prop["default"].asString();
            }
            
            rule.required = std::find(requiredKeys.begin(), requiredKeys.end(), key) != requiredKeys.end();
            
            addRule(rule);
        }
    }
    
    return true;
}

std::string ConfigSchema::toString() const {
    return exportJsonSchema();
}

ValidationResult ConfigSchema::validateKey(const std::string& key, const Json::Value& value) const {
    auto it = ruleMap.find(key);
    if (it == ruleMap.end()) {
        return ValidationResult::failure("No rule found for key: " + key);
    }
    
    return validateNode(value, it->second, key);
}

std::optional<ConfigSchema::Rule> ConfigSchema::getRule(const std::string& key) const {
    auto it = ruleMap.find(key);
    if (it != ruleMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool ConfigSchema::hasKey(const std::string& key) const {
    return ruleMap.find(key) != ruleMap.end();
}

void ConfigSchema::clear() {
    rules.clear();
    ruleMap.clear();
}

void ConfigSchema::registerCustomType(const std::string& typeName, 
                                     std::function<bool(const Json::Value&)> validator) {
    customTypes[typeName] = validator;
}

ValidationResult ConfigSchema::validateFile(const std::string& configFile, bool strict) const {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        return ValidationResult::failure("Failed to open config file: " + configFile);
    }
    
    Json::Value config;
    Json::CharReaderBuilder builder;
    std::string errs;
    
    if (!Json::parseFromStream(builder, file, &config, &errs)) {
        return ValidationResult::failure("Failed to parse config file: " + errs);
    }
    
    return validate(config, strict);
}

bool ConfigSchema::loadYamlSchema(const std::string& schemaFile) {
    try {
        YAML::Node yaml = YAML::LoadFile(schemaFile);
        
        if (yaml["properties"]) {
            for (const auto& item : yaml["properties"]) {
                std::string key = item.first.as<std::string>();
                YAML::Node prop = item.second;
                
                Rule rule;
                rule.key = key;
                
                if (prop["type"]) {
                    std::string type = prop["type"].as<std::string>();
                    if (type == "string") {
                        rule.type = ConfigValueType::STRING;
                        if (prop["pattern"]) {
                            rule.pattern = prop["pattern"].as<std::string>();
                        }
                        if (prop["minLength"]) {
                            rule.minLength = prop["minLength"].as<size_t>();
                        }
                        if (prop["maxLength"]) {
                            rule.maxLength = prop["maxLength"].as<size_t>();
                        }
                    } else if (type == "integer") {
                        rule.type = ConfigValueType::INTEGER;
                        if (prop["minimum"]) {
                            rule.min = prop["minimum"].as<int64_t>();
                        }
                        if (prop["maximum"]) {
                            rule.max = prop["maximum"].as<int64_t>();
                        }
                    } else if (type == "number") {
                        rule.type = ConfigValueType::FLOAT;
                        if (prop["minimum"]) {
                            rule.minFloat = prop["minimum"].as<double>();
                        }
                        if (prop["maximum"]) {
                            rule.maxFloat = prop["maximum"].as<double>();
                        }
                    } else if (type == "boolean") {
                        rule.type = ConfigValueType::BOOLEAN;
                    } else if (type == "array") {
                        rule.type = ConfigValueType::ARRAY;
                    } else if (type == "object") {
                        rule.type = ConfigValueType::OBJECT;
                    }
                }
                
                if (prop["enum"]) {
                    for (const auto& val : prop["enum"]) {
                        rule.enumValues.push_back(val.as<std::string>());
                    }
                }
                
                if (prop["required"]) {
                    rule.required = prop["required"].as<bool>();
                }
                
                if (prop["default"]) {
                    rule.defaultValue = prop["default"].as<std::string>();
                }
                
                if (prop["sensitive"]) {
                    rule.sensitive = prop["sensitive"].as<bool>();
                }
                
                if (prop["readOnly"]) {
                    rule.readOnly = prop["readOnly"].as<bool>();
                }
                
                if (prop["format"]) {
                    rule.format = prop["format"].as<std::string>();
                }
                
                addRule(rule);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        lastError = "Failed to load YAML schema: " + std::string(e.what());
        return false;
    }
}

ValidationResult ConfigSchema::validateCustomType(const Json::Value& value, 
                                                 const std::string& typeName) const {
    auto it = customTypes.find(typeName);
    if (it != customTypes.end()) {
        if (!it->second(value)) {
            return ValidationResult::failure("Invalid custom type: " + typeName);
        }
    }
    return ValidationResult::success();
}
