#include "utils/NetworkUtils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <fstream>
#include <regex>
#include <cstring>
#include <iostream>

using namespace common::utils;

const std::vector<std::string> NetworkUtils::PUBLIC_IP_SERVICES = {
    "http://api.ipify.org",
    "http://icanhazip.com",
    "http://checkip.amazonaws.com",
    "http://ipv4.icanhazip.com"
};

std::vector<std::string> NetworkUtils::getLocalIPs(IpType type) {
    std::vector<std::string> ips;
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        return ips;
    }
    
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        int family = ifa->ifa_addr->sa_family;
        if (type == IpType::IPv4 && family != AF_INET) continue;
        if (type == IpType::IPv6 && family != AF_INET6) continue;
        
        char ip[INET6_ADDRSTRLEN];
        if (family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
        } else if (family == AF_INET6) {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)ifa->ifa_addr;
            inet_ntop(AF_INET6, &addr->sin6_addr, ip, sizeof(ip));
        } else {
            continue;
        }
        
        // Skip loopback
        if (std::string(ifa->ifa_name) == "lo") continue;
        if (std::string(ip).find("127.") == 0) continue;
        if (std::string(ip) == "::1") continue;
        
        ips.push_back(ip);
    }
    
    freeifaddrs(ifaddr);
    return ips;
}

std::string NetworkUtils::getHostname() {
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        return std::string(buffer);
    }
    return "";
}

bool NetworkUtils::setHostname(const std::string& hostname) {
    return sethostname(hostname.c_str(), hostname.length()) == 0;
}

std::string NetworkUtils::getMACAddress(const std::string& interface) {
    struct ifreq ifr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return "";
    
    std::string iface = interface.empty() ? "eth0" : interface;
    strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
    
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
        char mac[18];
        snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
        close(sock);
        return std::string(mac);
    }
    
    close(sock);
    return "";
}

std::vector<NetworkUtils::NetworkInterface> NetworkUtils::getNetworkInterfaces() {
    std::vector<NetworkInterface> interfaces;
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        return interfaces;
    }
    
    std::map<std::string, NetworkInterface> ifaceMap;
    
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        std::string name = ifa->ifa_name;
        if (ifaceMap.find(name) == ifaceMap.end()) {
            NetworkInterface ni;
            ni.name = name;
            ni.isUp = (ifa->ifa_flags & IFF_UP) != 0;
            ni.isLoopback = (ifa->ifa_flags & IFF_LOOPBACK) != 0;
            ni.rxBytes = 0;
            ni.txBytes = 0;
            ni.rxPackets = 0;
            ni.txPackets = 0;
            ni.rxErrors = 0;
            ni.txErrors = 0;
            ni.rxDropped = 0;
            ni.txDropped = 0;
            ifaceMap[name] = ni;
        }
        
        int family = ifa->ifa_addr->sa_family;
        char ip[INET6_ADDRSTRLEN];
        
        if (family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
            ifaceMap[name].ipv4Address = ip;
            
            if (ifa->ifa_netmask) {
                struct sockaddr_in* netmask = (struct sockaddr_in*)ifa->ifa_netmask;
                inet_ntop(AF_INET, &netmask->sin_addr, ip, sizeof(ip));
                ifaceMap[name].netmask = ip;
            }
        } else if (family == AF_INET6) {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)ifa->ifa_addr;
            inet_ntop(AF_INET6, &addr->sin6_addr, ip, sizeof(ip));
            ifaceMap[name].ipv6Address = ip;
        }
        
        // Get MAC address
        if (ifaceMap[name].macAddress.empty()) {
            ifaceMap[name].macAddress = getMACAddress(name);
        }
    }
    
    freeifaddrs(ifaddr);
    
    for (auto& pair : ifaceMap) {
        interfaces.push_back(pair.second);
    }
    
    return interfaces;
}

bool NetworkUtils::ping(const std::string& host, int timeoutMs) {
    std::string cmd = "ping -c 1 -W " + std::to_string(timeoutMs / 1000) + " " + host + " > /dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

std::string NetworkUtils::dnsLookup(const std::string& hostname, IpType type) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = (type == IpType::IPv4) ? AF_INET : AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(hostname.c_str(), nullptr, &hints, &result) != 0) {
        return "";
    }
    
    char ip[INET6_ADDRSTRLEN];
    if (type == IpType::IPv4) {
        struct sockaddr_in* addr = (struct sockaddr_in*)result->ai_addr;
        inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    } else {
        struct sockaddr_in6* addr = (struct sockaddr_in6*)result->ai_addr;
        inet_ntop(AF_INET6, &addr->sin6_addr, ip, sizeof(ip));
    }
    
    freeaddrinfo(result);
    return std::string(ip);
}

std::string NetworkUtils::reverseDnsLookup(const std::string& ip) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    char host[NI_MAXHOST];
    if (getnameinfo((struct sockaddr*)&addr, sizeof(addr), host, sizeof(host), 
                    nullptr, 0, 0) == 0) {
        return std::string(host);
    }
    return "";
}

bool NetworkUtils::isPortOpen(const std::string& host, int port, int timeoutMs) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    
    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);
    
    return result == 0;
}

std::string NetworkUtils::getPublicIP() {
    std::string ip;
    std::string cmd = "curl -s " + PUBLIC_IP_SERVICES[0] + " 2>/dev/null";
    char buffer[128];
    std::string result;
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    // Trim whitespace
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

std::string NetworkUtils::ipToMAC(const std::string& ip) {
    std::string cmd = "arp -n " + ip + " | grep -i " + ip + " | awk '{print $3}'";
    char buffer[128];
    std::string result;
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

bool NetworkUtils::isValidIP(const std::string& ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 0;
}

bool NetworkUtils::isValidMAC(const std::string& mac) {
    std::regex pattern(R"(^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$)");
    return std::regex_match(mac, pattern);
}

uint32_t NetworkUtils::ipToInt(const std::string& ip) {
    struct sockaddr_in sa;
    if (inet_pton(AF_INET, ip.c_str(), &sa.sin_addr) == 0) {
        return 0;
    }
    return ntohl(sa.sin_addr.s_addr);
}

std::string NetworkUtils::intToIP(uint32_t ip) {
    struct in_addr addr;
    addr.s_addr = htonl(ip);
    char buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string NetworkUtils::cidrToNetmask(int cidr) {
    if (cidr < 0 || cidr > 32) return "";
    uint32_t mask = 0xFFFFFFFF << (32 - cidr);
    return intToIP(mask);
}

int NetworkUtils::netmaskToCidr(const std::string& netmask) {
    uint32_t mask = ipToInt(netmask);
    if (mask == 0) return 0;
    
    int cidr = 0;
    for (int i = 31; i >= 0; --i) {
        if ((mask >> i) & 1) {
            cidr++;
        } else {
            break;
        }
    }
    return cidr;
}
