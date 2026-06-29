#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace common {
namespace utils {

/**
 * @brief String manipulation utilities
 */
class StringUtils {
public:
    /**
     * @brief Trim whitespace from beginning and end of string
     */
    static std::string trim(const std::string& str);
    
    /**
     * @brief Trim whitespace from beginning of string
     */
    static std::string trimLeft(const std::string& str);
    
    /**
     * @brief Trim whitespace from end of string
     */
    static std::string trimRight(const std::string& str);
    
    /**
     * @brief Split string by delimiter
     * @param str String to split
     * @param delimiter Delimiter character(s)
     * @param maxSplit Maximum number of splits (0 = unlimited)
     * @return Vector of substrings
     */
    static std::vector<std::string> split(const std::string& str, 
                                         const std::string& delimiter,
                                         size_t maxSplit = 0);
    
    /**
     * @brief Join strings with delimiter
     * @param strings Vector of strings
     * @param delimiter Delimiter to insert between strings
     * @return Joined string
     */
    static std::string join(const std::vector<std::string>& strings,
                           const std::string& delimiter);
    
    /**
     * @brief Replace all occurrences of a substring
     */
    static std::string replace(const std::string& str,
                              const std::string& from,
                              const std::string& to);
    
    /**
     * @brief Replace substring with regex pattern
     */
    static std::string replaceRegex(const std::string& str,
                                   const std::string& pattern,
                                   const std::string& replacement);
    
    /**
     * @brief Check if string starts with prefix
     */
    static bool startsWith(const std::string& str, const std::string& prefix);
    
    /**
     * @brief Check if string ends with suffix
     */
    static bool endsWith(const std::string& str, const std::string& suffix);
    
    /**
     * @brief Convert string to lowercase
     */
    static std::string toLower(const std::string& str);
    
    /**
     * @brief Convert string to uppercase
     */
    static std::string toUpper(const std::string& str);
    
    /**
     * @brief Convert string to title case
     */
    static std::string toTitleCase(const std::string& str);
    
    /**
     * @brief Convert string to camel case
     */
    static std::string toCamelCase(const std::string& str);
    
    /**
     * @brief Convert string to snake case
     */
    static std::string toSnakeCase(const std::string& str);
    
    /**
     * @brief Convert string to kebab case
     */
    static std::string toKebabCase(const std::string& str);
    
    /**
     * @brief Check if string contains substring
     */
    static bool contains(const std::string& str, const std::string& substr);
    
    /**
     * @brief Check if string matches regex pattern
     */
    static bool matches(const std::string& str, const std::string& pattern);
    
    /**
     * @brief Extract regex groups from string
     */
    static std::vector<std::string> extractGroups(const std::string& str,
                                                  const std::string& pattern);
    
    /**
     * @brief Format string with arguments (printf-style)
     */
    template<typename... Args>
    static std::string format(const std::string& format, Args... args);
    
    /**
     * @brief Format string with named placeholders
     * Example: formatNamed("Hello {name}", {{"name", "World"}})
     */
    static std::string formatNamed(const std::string& format,
                                   const std::map<std::string, std::string>& values);
    
    /**
     * @brief Pad string to specified length
     */
    static std::string padLeft(const std::string& str, size_t length, char padChar = ' ');
    static std::string padRight(const std::string& str, size_t length, char padChar = ' ');
    
    /**
     * @brief Truncate string to specified length
     */
    static std::string truncate(const std::string& str, size_t length, 
                               const std::string& suffix = "...");
    
    /**
     * @brief Get string length in bytes
     */
    static size_t byteLength(const std::string& str);
    
    /**
     * @brief Check if string is empty or whitespace
     */
    static bool isBlank(const std::string& str);
    
    /**
     * @brief Check if string is a valid email address
     */
    static bool isValidEmail(const std::string& email);
    
    /**
     * @brief Check if string is a valid URL
     */
    static bool isValidUrl(const std::string& url);
    
    /**
     * @brief Check if string is a valid IP address
     */
    static bool isValidIp(const std::string& ip);
    
    /**
     * @brief Check if string is a valid UUID
     */
    static bool isValidUuid(const std::string& uuid);
    
    /**
     * @brief Check if string is a valid phone number
     */
    static bool isValidPhone(const std::string& phone);
    
    /**
     * @brief Generate random string
     * @param length Length of random string
     * @param includeSpecial Include special characters
     * @return Random string
     */
    static std::string random(size_t length = 16, bool includeSpecial = false);
    
    /**
     * @brief Hash string using SHA-256
     */
    static std::string hashSHA256(const std::string& str);
    
    /**
     * @brief Encode string to Base64
     */
    static std::string toBase64(const std::string& str);
    
    /**
     * @brief Decode string from Base64
     */
    static std::string fromBase64(const std::string& str);
    
    /**
     * @brief URL encode string
     */
    static std::string urlEncode(const std::string& str);
    
    /**
     * @brief URL decode string
     */
    static std::string urlDecode(const std::string& str);
    
    /**
     * @brief Escape HTML/XML special characters
     */
    static std::string escapeHtml(const std::string& str);
    
    /**
     * @brief Unescape HTML/XML special characters
     */
    static std::string unescapeHtml(const std::string& str);
    
    /**
     * @brief Escape JSON string
     */
    static std::string escapeJson(const std::string& str);
    
    /**
     * @brief Get string similarity (Levenshtein distance)
     */
    static double similarity(const std::string& str1, const std::string& str2);
    
    /**
     * @brief Get Levenshtein distance between strings
     */
    static int levenshteinDistance(const std::string& str1, const std::string& str2);

private:
    static const std::string BASE64_CHARS;
};

} // namespace utils
} // namespace common

#endif // STRING_UTILS_H
