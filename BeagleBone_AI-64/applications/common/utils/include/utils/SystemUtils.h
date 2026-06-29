#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace common {
namespace utils {

/**
 * @brief System utilities
 * 
 * Provides system information, process management,
 * and hardware detection utilities
 */
class SystemUtils {
public:
    /**
     * @brief System information
     */
    struct SystemInfo {
        std::string hostname;
        std::string osName;
        std::string osVersion;
        std::string kernelVersion;
        std::string architecture;
        std::string machine;
        std::string domainName;
        std::string timezone;
        uint64_t uptimeSeconds;
    };
    
    /**
     * @brief CPU information
     */
    struct CPUInfo {
        std::string model;
        std::string vendor;
        int cores;
        int threads;
        double frequencyMHz;
        double maxFrequencyMHz;
        double minFrequencyMHz;
        double loadAverage1m;
        double loadAverage5m;
        double loadAverage15m;
        double usagePercent;
        std::vector<double> coreUsagePercent;
        uint64_t userTime;
        uint64_t niceTime;
        uint64_t systemTime;
        uint64_t idleTime;
        uint64_t iowaitTime;
        uint64_t irqTime;
        uint64_t softirqTime;
        uint64_t stealTime;
    };
    
    /**
     * @brief Memory information
     */
    struct MemoryInfo {
        uint64_t totalBytes;
        uint64_t availableBytes;
        uint64_t usedBytes;
        uint64_t freeBytes;
        uint64_t cachedBytes;
        uint64_t buffersBytes;
        uint64_t swapTotalBytes;
        uint64_t swapUsedBytes;
        uint64_t swapFreeBytes;
        double usedPercent;
        double swapUsedPercent;
    };
    
    /**
     * @brief Disk information
     */
    struct DiskInfo {
        std::string path;
        std::string device;
        std::string filesystem;
        uint64_t totalBytes;
        uint64_t usedBytes;
        uint64_t freeBytes;
        double usedPercent;
        uint64_t readBytesPerSec;
        uint64_t writeBytesPerSec;
        uint64_t readOpsPerSec;
        uint64_t writeOpsPerSec;
    };
    
    /**
     * @brief Process information
     */
    struct ProcessInfo {
        int pid;
        int ppid;
        int pgid;
        int sid;
        std::string name;
        std::string cmdline;
        std::string user;
        std::string group;
        uint64_t memoryRSS;
        uint64_t memoryVMS;
        double cpuPercent;
        double memoryPercent;
        std::chrono::system_clock::time_point startTime;
        uint64_t userTime;
        uint64_t systemTime;
        int priority;
        int nice;
        std::string state;
        std::vector<int> children;
    };
    
    /**
     * @brief Network information
     */
    struct NetworkStats {
        uint64_t rxBytes;
        uint64_t txBytes;
        uint64_t rxPackets;
        uint64_t txPackets;
        uint64_t rxErrors;
        uint64_t txErrors;
        uint64_t rxDropped;
        uint64_t txDropped;
        double rxBytesPerSec;
        double txBytesPerSec;
        uint64_t rxPacketsPerSec;
        uint64_t txPacketsPerSec;
    };
    
    /**
     * @brief Get system information
     */
    static SystemInfo getSystemInfo();
    
    /**
     * @brief Get CPU information
     */
    static CPUInfo getCPUInfo();
    
    /**
     * @brief Get memory information
     */
    static MemoryInfo getMemoryInfo();
    
    /**
     * @brief Get disk information
     * @param path Mount point path
     */
    static DiskInfo getDiskInfo(const std::string& path = "/");
    
    /**
     * @brief Get all disk information
     */
    static std::vector<DiskInfo> getAllDiskInfo();
    
    /**
     * @brief Get process information
     * @param pid Process ID (0 for current process)
     */
    static ProcessInfo getProcessInfo(int pid = 0);
    
    /**
     * @brief Get all processes
     */
    static std::vector<ProcessInfo> getAllProcesses();
    
    /**
     * @brief Get network statistics for interface
     * @param interface Network interface name
     */
    static NetworkStats getNetworkStats(const std::string& interface);
    
    /**
     * @brief Get all network interfaces
     */
    static std::vector<std::string> getNetworkInterfaces();
    
    /**
     * @brief Get system load average
     */
    static std::tuple<double, double, double> getLoadAverage();
    
    /**
     * @brief Get system uptime in seconds
     */
    static uint64_t getUptime();
    
    /**
     * @brief Get boot time
     */
    static std::chrono::system_clock::time_point getBootTime();
    
    /**
     * @brief Get current process ID
     */
    static int getPID();
    
    /**
     * @brief Get parent process ID
     */
    static int getPPID();
    
    /**
     * @brief Check if process is running
     * @param pid Process ID
     */
    static bool isProcessRunning(int pid);
    
    /**
     * @brief Kill process
     * @param pid Process ID
     * @param signal Signal to send (SIGTERM by default)
     * @return true on success
     */
    static bool killProcess(int pid, int signal = 15);
    
    /**
     * @brief Wait for process to exit
     * @param pid Process ID
     * @param timeoutMs Timeout in milliseconds (0 = infinite)
     * @return true if process exited
     */
    static bool waitForProcess(int pid, int timeoutMs = 0);
    
    /**
     * @brief Execute command and get output
     * @param command Command string
     * @param output Output from command
     * @param errorOutput Error output from command
     * @param timeoutMs Timeout in milliseconds
     * @return Exit code
     */
    static int executeCommand(const std::string& command,
                             std::string& output,
                             std::string& errorOutput,
                             int timeoutMs = 30000);
    
    /**
     * @brief Execute command in background
     * @param command Command string
     * @return PID of child process, or -1 on error
     */
    static int executeAsync(const std::string& command);
    
    /**
     * @brief Check if user has root privileges
     */
    static bool isRoot();
    
    /**
     * @brief Get current user name
     */
    static std::string getUserName();
    
    /**
     * @brief Get current group name
     */
    static std::string getGroupName();
    
    /**
     * @brief Get current user ID
     */
    static int getUserID();
    
    /**
     * @brief Get current group ID
     */
    static int getGroupID();
    
    /**
     * @brief Daemonize current process
     * @param chdir Change to root directory
     * @param closeAll Close all file descriptors
     * @return PID of daemon
     */
    static int daemonize(bool chdir = true, bool closeAll = true);
    
    /**
     * @brief Get environment variable
     */
    static std::string getEnv(const std::string& name);
    
    /**
     * @brief Set environment variable
     */
    static bool setEnv(const std::string& name, const std::string& value, bool overwrite = true);
    
    /**
     * @brief Get all environment variables
     */
    static std::map<std::string, std::string> getEnvVars();
    
    /**
     * @brief Get system temperature (in Celsius)
     * @return Temperature or -1 if not available
     */
    static double getSystemTemperature();
    
    /**
     * @brief Get CPU temperature (in Celsius)
     * @param core Core index (-1 for average)
     * @return Temperature or -1 if not available
     */
    static double getCPUTemperature(int core = -1);
    
    /**
     * @brief Get GPU temperature (in Celsius)
     * @return Temperature or -1 if not available
     */
    static double getGPUTemperature();
    
    /**
     * @brief Check if running in Docker container
     */
    static bool isInDocker();
    
    /**
     * @brief Check if running in VM
     */
    static bool isInVM();
    
    /**
     * @brief Get system serial number
     */
    static std::string getSystemSerialNumber();
    
    /**
     * @brief Get system manufacturer
     */
    static std::string getSystemManufacturer();
    
    /**
     * @brief Get system model
     */
    static std::string getSystemModel();

private:
    static std::string readProcFile(const std::string& path);
    static std::vector<std::string> readProcFiles(const std::string& pattern);
    static void parseCPUStat(CPUInfo& info);
    static void parseMemoryStat(MemoryInfo& info);
    static void parseDiskStat(DiskInfo& info);
};

} // namespace utils
} // namespace common

#endif // SYSTEM_UTILS_H
