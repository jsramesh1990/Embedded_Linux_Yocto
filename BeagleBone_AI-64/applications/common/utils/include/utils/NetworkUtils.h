#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace common {
namespace utils {

/**
 * @brief Network utilities
 */
class NetworkUtils {
public:
    /**
     * @brief IP address type
     */
    enum class IpType {
        IPv4,
        IPv6,
        INVALID
    };
    
    /**
     * @brief Network interface information
     */
    struct NetworkInterface {
        std::string name;
        std::string macAddress;
        std::string ipv4Address;
        std::string ipv6Address;
        std::string netmask;
        std::string gateway;
        bool isUp;
        bool isLoopback;
        uint64_t rxBytes;
        uint64_t txBytes;
        uint64_t rxPackets;
        uint64_t txPackets;
        uint64_t rxErrors;
        uint64_t txErrors;
        uint64_t rxDropped;
        uint64_t txDropped;
    };
    
    /**
     * @brief Get local IP addresses
     */
    static std::vector<std::string> getLocalIPs(IpType type = IpType::IPv4);
    
    /**
     * @brief Get hostname
     */
    static std::string getHostname();
    
    /**
     * @brief Set hostname
     */
    static bool setHostname(const std::string& hostname);
    
    /**
     * @brief Get MAC address for interface
     */
    static std::string getMACAddress(const std::string& interface = "");
    
    /**
     * @brief Get network interfaces
     */
    static std::vector<NetworkInterface> getNetworkInterfaces();
    
    /**
     * @brief Check if host is reachable (ping)
     * @param host Hostname or IP
     * @param timeoutMs Timeout in milliseconds
     * @return true if host is reachable
     */
    static bool ping(const std::string& host, int timeoutMs = 5000);
    
    /**
     * @brief Perform DNS lookup
     * @param hostname Hostname to resolve
     * @param type IP type
     * @return IP address or empty string
     */
    static std::string dnsLookup(const std::string& hostname, IpType type = IpType::IPv4);
    
    /**
     * @brief Reverse DNS lookup
     * @param ip IP address
     * @return Hostname or empty string
     */
    static std::string reverseDnsLookup(const std::string& ip);
    
    /**
     * @brief Check if port is open
     * @param host Hostname or IP
     * @param port Port number
     * @param timeoutMs Timeout in milliseconds
     * @return true if port is open
     */
    static bool isPortOpen(const std::string& host, int port, int timeoutMs = 3000);
    
    /**
     * @brief Get public IP address (via external service)
     */
    static std::string getPublicIP();
    
    /**
     * @brief Get MAC address from IP
     * @param ip IP address
     * @return MAC address or empty string
     */
    static std::string ipToMAC(const std::string& ip);
    
    /**
     * @brief Validate IP address
     */
    static bool isValidIP(const std::string& ip);
    
    /**
     * @brief Validate MAC address
     */
    static bool isValidMAC(const std::string& mac);
    
    /**
     * @brief Convert IP to integer
     */
    static uint32_t ipToInt(const std::string& ip);
    
    /**
     * @brief Convert integer to IP
     */
    static std::string intToIP(uint32_t ip);
    
    /**
     * @brief Get network mask for CIDR
     */
    static std::string cidrToNetmask(int cidr);
    
    /**
     * @brief Get CIDR from netmask
     */
    static int netmaskToCidr(const std::string& netmask);

private:
    static const std::vector<std::string> PUBLIC_IP_SERVICES;
};

} // namespace utils
} // namespace common

#endif // NETWORK_UTILS_H
