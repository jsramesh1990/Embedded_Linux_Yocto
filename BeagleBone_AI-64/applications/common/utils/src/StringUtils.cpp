#include "utils/StringUtils.h"
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

using namespace common::utils;

const std::string StringUtils::BASE64_CHARS = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string StringUtils::trim(const std::string& str) {
    return trimLeft(trimRight(str));
}

std::string StringUtils::trimLeft(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), 
        [](unsigned char ch) { return std::isspace(ch); });
    return std::string(start, str.end());
}

std::string StringUtils::trimRight(const std::string& str) {
    auto end = std::find_if_not(str.rbegin(), str.rend(),
        [](unsigned char ch) { return std::isspace(ch); }).base();
    return std::string(str.begin(), end);
}

std::vector<std::string> StringUtils::split(const std::string& str, 
                                           const std::string& delimiter,
                                           size_t maxSplit) {
    std::vector<std::string> result;
    if (str.empty()) return result;
    
    size_t start = 0;
    size_t end = 0;
    size_t count = 0;
    
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        if (maxSplit > 0 && count >= maxSplit) {
            break;
        }
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        count++;
    }
    
    result.push_back(str.substr(start));
    return result;
}

std::string StringUtils::join(const std::vector<std::string>& strings,
                             const std::string& delimiter) {
    if (strings.empty()) return "";
    
    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter + strings[i];
    }
    return result;
}

std::string StringUtils::replace(const std::string& str,
                                const std::string& from,
                                const std::string& to) {
    if (from.empty()) return str;
    
    std::string result = str;
    size_t startPos = 0;
    while ((startPos = result.find(from, startPos)) != std::string::npos) {
        result.replace(startPos, from.length(), to);
        startPos += to.length();
    }
    return result;
}

std::string StringUtils::replaceRegex(const std::string& str,
                                     const std::string& pattern,
                                     const std::string& replacement) {
    std::regex regex(pattern);
    return std::regex_replace(str, regex, replacement);
}

bool StringUtils::startsWith(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length()) return false;
    return str.compare(0, prefix.length(), prefix) == 0;
}

bool StringUtils::endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

std::string StringUtils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string StringUtils::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string StringUtils::toTitleCase(const std::string& str) {
    std::string result = toLower(str);
    bool capitalize = true;
    for (char& c : result) {
        if (std::isspace(c)) {
            capitalize = true;
        } else if (capitalize && std::isalpha(c)) {
            c = std::toupper(c);
            capitalize = false;
        }
    }
    return result;
}

std::string StringUtils::toCamelCase(const std::string& str) {
    std::string result;
    bool capitalize = false;
    
    for (char c : str) {
        if (std::isalnum(c)) {
            if (capitalize && std::isalpha(c)) {
                result += std::toupper(c);
                capitalize = false;
            } else {
                result += std::tolower(c);
            }
        } else {
            capitalize = true;
        }
    }
    return result;
}

std::string StringUtils::toSnakeCase(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (std::isupper(c)) {
            if (!result.empty()) {
                result += '_';
            }
            result += std::tolower(c);
        } else if (std::isalnum(c)) {
            result += c;
        } else {
            result += '_';
        }
    }
    return result;
}

std::string StringUtils::toKebabCase(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (std::isupper(c)) {
            if (!result.empty()) {
                result += '-';
            }
            result += std::tolower(c);
        } else if (std::isalnum(c)) {
            result += c;
        } else {
            result += '-';
        }
    }
    return result;
}

bool StringUtils::contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

bool StringUtils::matches(const std::string& str, const std::string& pattern) {
    try {
        std::regex regex(pattern);
        return std::regex_match(str, regex);
    } catch (const std::regex_error&) {
        return false;
    }
}

std::vector<std::string> StringUtils::extractGroups(const std::string& str,
                                                   const std::string& pattern) {
    std::vector<std::string> result;
    try {
        std::regex regex(pattern);
        std::smatch match;
        if (std::regex_match(str, match, regex)) {
            for (size_t i = 1; i < match.size(); ++i) {
                result.push_back(match[i]);
            }
        }
    } catch (const std::regex_error&) {}
    return result;
}

std::string StringUtils::formatNamed(const std::string& format,
                                    const std::map<std::string, std::string>& values) {
    std::string result = format;
    for (const auto& [key, value] : values) {
        std::string placeholder = "{" + key + "}";
        result = replace(result, placeholder, value);
    }
    return result;
}

std::string StringUtils::padLeft(const std::string& str, size_t length, char padChar) {
    if (str.length() >= length) return str;
    return std::string(length - str.length(), padChar) + str;
}

std::string StringUtils::padRight(const std::string& str, size_t length, char padChar) {
    if (str.length() >= length) return str;
    return str + std::string(length - str.length(), padChar);
}

std::string StringUtils::truncate(const std::string& str, size_t length,
                                 const std::string& suffix) {
    if (str.length() <= length) return str;
    return str.substr(0, length - suffix.length()) + suffix;
}

size_t StringUtils::byteLength(const std::string& str) {
    return str.length();
}

bool StringUtils::isBlank(const std::string& str) {
    return trim(str).empty();
}

bool StringUtils::isValidEmail(const std::string& email) {
    std::regex pattern(R"(([\w\.\-_]+)@([\w\.\-_]+)\.([a-zA-Z]{2,}))");
    return std::regex_match(email, pattern);
}

bool StringUtils::isValidUrl(const std::string& url) {
    std::regex pattern(R"(^(https?|ftp)://[^\s/$.?#].[^\s]*$)");
    return std::regex_match(url, pattern);
}

bool StringUtils::isValidIp(const std::string& ip) {
    std::regex ipv4(R"(^(\d{1,3}\.){3}\d{1,3}$)");
    std::regex ipv6(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$)");
    return std::regex_match(ip, ipv4) || std::regex_match(ip, ipv6);
}

bool StringUtils::isValidUuid(const std::string& uuid) {
    std::regex pattern(R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})");
    return std::regex_match(toLower(uuid), pattern);
}

bool StringUtils::isValidPhone(const std::string& phone) {
    std::regex pattern(R"(^\+?[\d\s\-()]{7,20}$)");
    return std::regex_match(phone, pattern);
}

std::string StringUtils::random(size_t length, bool includeSpecial) {
    static const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const std::string special = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    std::string allChars = chars + (includeSpecial ? special : "");
    if (allChars.empty()) return "";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, allChars.length() - 1);
    
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += allChars[dis(gen)];
    }
    return result;
}

std::string StringUtils::hashSHA256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.length());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::string StringUtils::toBase64(const std::string& str) {
    BIO* bio = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    bio = BIO_push(bio, bmem);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, str.c_str(), str.length());
    BIO_flush(bio);
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);
    
    std::string result(bptr->data, bptr->length);
    BIO_free_all(bio);
    return result;
}

std::string StringUtils::fromBase64(const std::string& str) {
    BIO* bio = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new_mem_buf(str.c_str(), str.length());
    bio = BIO_push(bio, bmem);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    
    std::vector<char> buffer(str.length());
    int len = BIO_read(bio, buffer.data(), buffer.size());
    BIO_free_all(bio);
    
    return std::string(buffer.data(), len);
}

std::string StringUtils::urlEncode(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else {
            std::stringstream ss;
            ss << '%' << std::hex << std::uppercase << std::setw(2) 
               << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
            result += ss.str();
        }
    }
    return result;
}

std::string StringUtils::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            std::string hex = str.substr(i + 1, 2);
            char decoded = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += decoded;
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string StringUtils::escapeHtml(const std::string& str) {
    std::string result = replace(str, "&", "&amp;");
    result = replace(result, "<", "&lt;");
    result = replace(result, ">", "&gt;");
    result = replace(result, "\"", "&quot;");
    result = replace(result, "'", "&#39;");
    return result;
}

std::string StringUtils::unescapeHtml(const std::string& str) {
    std::string result = replace(str, "&amp;", "&");
    result = replace(result, "&lt;", "<");
    result = replace(result, "&gt;", ">");
    result = replace(result, "&quot;", "\"");
    result = replace(result, "&#39;", "'");
    return result;
}

std::string StringUtils::escapeJson(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::stringstream ss;
                    ss << "\\u" << std::hex << std::setw(4) 
                       << std::setfill('0') << static_cast<int>(c);
                    result += ss.str();
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

int StringUtils::levenshteinDistance(const std::string& str1, const std::string& str2) {
    const size_t len1 = str1.length();
    const size_t len2 = str2.length();
    
    std::vector<int> row(len2 + 1);
    for (size_t i = 0; i <= len2; ++i) {
        row[i] = i;
    }
    
    for (size_t i = 1; i <= len1; ++i) {
        int prev = row[0];
        row[0] = i;
        for (size_t j = 1; j <= len2; ++j) {
            int cost = (str1[i-1] == str2[j-1]) ? 0 : 1;
            int min1 = std::min(row[j] + 1, prev + 1);
            int min2 = std::min(row[j-1] + cost, min1);
            prev = row[j];
            row[j] = min2;
        }
    }
    
    return row[len2];
}

double StringUtils::similarity(const std::string& str1, const std::string& str2) {
    if (str1.empty() && str2.empty()) return 1.0;
    if (str1.empty() || str2.empty()) return 0.0;
    
    int distance = levenshteinDistance(str1, str2);
    int maxLen = std::max(str1.length(), str2.length());
    return 1.0 - static_cast<double>(distance) / maxLen;
}
