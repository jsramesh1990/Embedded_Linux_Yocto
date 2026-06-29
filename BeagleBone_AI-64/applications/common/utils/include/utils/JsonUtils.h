#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <json/json.h>

namespace common {
namespace utils {

/**
 * @brief JSON utility functions
 */
class JsonUtils {
public:
    /**
     * @brief Parse JSON string
     * @param json JSON string
     * @return JSON value
     */
    static Json::Value parse(const std::string& json);
    
    /**
     * @brief Parse JSON file
     * @param path File path
     * @return JSON value
     */
    static Json::Value parseFile(const std::string& path);
    
    /**
     * @brief Serialize JSON to string
     * @param json JSON value
     * @param pretty Pretty print
     * @return JSON string
     */
    static std::string serialize(const Json::Value& json, bool pretty = true);
    
    /**
     * @brief Serialize JSON to file
     * @param path File path
     * @param json JSON value
     * @param pretty Pretty print
     * @return true on success
     */
    static bool serializeFile(const std::string& path, const Json::Value& json, bool pretty = true);
    
    /**
     * @brief Validate JSON schema
     * @param json JSON value
     * @param schema JSON schema
     * @return true if valid
     */
    static bool validateSchema(const Json::Value& json, const Json::Value& schema);
    
    /**
     * @brief Merge two JSON objects
     * @param base Base JSON
     * @param override Override JSON
     * @param recursive Recursive merge
     * @return Merged JSON
     */
    static Json::Value merge(const Json::Value& base, const Json::Value& override, bool recursive = true);
    
    /**
     * @brief Diff between two JSON objects
     * @param left First JSON
     * @param right Second JSON
     * @return Differences as JSON
     */
    static Json::Value diff(const Json::Value& left, const Json::Value& right);
    
    /**
     * @brief Check if JSON contains key
     */
    static bool hasKey(const Json::Value& json, const std::string& key);
    
    /**
     * @brief Get value with default
     */
    template<typename T>
    static T getOrDefault(const Json::Value& json, const std::string& key, const T& defaultValue);
    
    /**
     * @brief Convert JSON to map
     */
    static std::map<std::string, Json::Value> toMap(const Json::Value& json);
    
    /**
     * @brief Convert map to JSON
     */
    static Json::Value fromMap(const std::map<std::string, Json::Value>& map);
    
    /**
     * @brief Convert JSON to vector
     */
    static std::vector<Json::Value> toVector(const Json::Value& json);
    
    /**
     * @brief Convert vector to JSON
     */
    static Json::Value fromVector(const std::vector<Json::Value>& vec);
    
    /**
     * @brief Patch JSON (RFC 6902)
     * @param target JSON to patch
     * @param patch Patch operations
     * @return Patched JSON
     */
    static Json::Value patch(const Json::Value& target, const Json::Value& patch);
    
    /**
     * @brief Pretty print JSON
     */
    static std::string prettyPrint(const Json::Value& json);
    
    /**
     * @brief Minify JSON
     */
    static std::string minify(const Json::Value& json);

private:
    static bool validateNode(const Json::Value& node, const Json::Value& schema, std::string& error);
};

} // namespace utils
} // namespace common

#endif // JSON_UTILS_H
