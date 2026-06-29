#include "utils/SystemUtils.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <regex>
#include <chrono>
#include <thread>
#include <array>

using namespace common::utils;

SystemUtils::SystemInfo SystemUtils::getSystemInfo() {
    SystemInfo info;
    struct utsname uts;
    
    if (uname(&uts) == 0) {
        info.hostname = uts.nodename;
        info.kernelVersion = uts.release;
        info.architecture = uts.machine;
        info.machine = uts.machine;
    }
    
    // OS Info
    std::ifstream osRelease("/etc/os-release");
    if (osRelease.is_open()) {
        std::string line;
        while (std::getline(osRelease, line)) {
            if (line.find("PRETTY_NAME=") == 0) {
                info.osName = line.substr(13);
                info.osName.erase(info.osName.find_last_not_of("\"") + 1);
                info.osName.erase(0, info.osName.find_first_not_of("\""));
            } else if (line.find("VERSION_ID=") == 0) {
                info.osVersion = line.substr(11);
                info.osVersion.erase(info.osVersion.find_last_not_of("\"") + 1);
                info.osVersion.erase(0, info.osVersion.find_first_not_of("\""));
            }
        }
        osRelease.close();
    }
    
    // Domain name
    char domain[256];
    if (getdomainname(domain, sizeof(domain)) == 0) {
        info.domainName = domain;
    }
    
    // Timezone
    std::ifstream timezone("/etc/timezone");
    if (timezone.is_open()) {
        std::getline(timezone, info.timezone);
        timezone.close();
    }
    
    info.uptimeSeconds = getUptime();
    
    return info;
}

SystemUtils::CPUInfo SystemUtils::getCPUInfo() {
    CPUInfo info;
    
    // Read /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        int coreCount = 0;
        
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                info.model = line.substr(line.find(":") + 2);
            } else if (line.find("vendor_id") != std::string::npos) {
                info.vendor = line.substr(line.find(":") + 2);
            } else if (line.find("cpu cores") != std::string::npos) {
                info.cores = std::stoi(line.substr(line.find(":") + 2));
            } else if (line.find("cpu MHz") != std::string::npos) {
                info.frequencyMHz = std::stod(line.substr(line.find(":") + 2));
            } else if (line.find("siblings") != std::string::npos) {
                info.threads = std::stoi(line.substr(line.find(":") + 2));
            } else if (line.find("processor") != std::string::npos) {
                coreCount++;
            }
        }
        cpuinfo.close();
        
        if (info.cores == 0) info.cores = coreCount;
        if (info.threads == 0) info.threads = coreCount;
    }
    
    // Get CPU frequency from sysfs
    std::ifstream maxFreq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
    if (maxFreq.is_open()) {
        std::string freq;
        std::getline(maxFreq, freq);
        info.maxFrequencyMHz = std::stod(freq) / 1000.0;
        maxFreq.close();
    }
    
    std::ifstream minFreq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
    if (minFreq.is_open()) {
        std::string freq;
        std::getline(minFreq, freq);
        info.minFrequencyMHz = std::stod(freq) / 1000.0;
        minFreq.close();
    }
    
    // Get load average
    auto load = getLoadAverage();
    info.loadAverage1m = std::get<0>(load);
    info.loadAverage5m = std::get<1>(load);
    info.loadAverage15m = std::get<2>(load);
    
    // Get CPU usage
    parseCPUStat(info);
    
    return info;
}

void SystemUtils::parseCPUStat(CPUInfo& info) {
    std::ifstream stat("/proc/stat");
    if (!stat.is_open()) return;
    
    std::string line;
    std::getline(stat, line);
    stat.close();
    
    std::istringstream iss(line);
    std::string cpu;
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    info.userTime = user;
    info.niceTime = nice;
    info.systemTime = system;
    info.idleTime = idle;
    info.iowaitTime = iowait;
    info.irqTime = irq;
    info.softirqTime = softirq;
    info.stealTime = steal;
    
    uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
    info.usagePercent = (total - idle) * 100.0 / total;
}

SystemUtils::MemoryInfo SystemUtils::getMemoryInfo() {
    MemoryInfo info;
    
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) return info;
    
    std::string line;
    while (std::getline(meminfo, line)) {
        std::string key, value;
        std::istringstream iss(line);
        iss >> key >> value;
        
        uint64_t bytes = std::stoull(value) * 1024;
        
        if (key == "MemTotal:") info.totalBytes = bytes;
        else if (key == "MemFree:") info.freeBytes = bytes;
        else if (key == "MemAvailable:") info.availableBytes = bytes;
        else if (key == "Cached:") info.cachedBytes = bytes;
        else if (key == "Buffers:") info.buffersBytes = bytes;
        else if (key == "SwapTotal:") info.swapTotalBytes = bytes;
        else if (key == "SwapFree:") info.swapFreeBytes = bytes;
        else if (key == "SwapCached:") {
            // Not used directly
        }
    }
    meminfo.close();
    
    info.usedBytes = info.totalBytes - info.freeBytes - info.cachedBytes - info.buffersBytes;
    info.swapUsedBytes = info.swapTotalBytes - info.swapFreeBytes;
    
    if (info.totalBytes > 0) {
        info.usedPercent = info.usedBytes * 100.0 / info.totalBytes;
    }
    if (info.swapTotalBytes > 0) {
        info.swapUsedPercent = info.swapUsedBytes * 100.0 / info.swapTotalBytes;
    }
    
    return info;
}

SystemUtils::DiskInfo SystemUtils::getDiskInfo(const std::string& path) {
    DiskInfo info;
    info.path = path;
    
    struct statvfs stats;
    if (statvfs(path.c_str(), &stats) != 0) {
        return info;
    }
    
    info.totalBytes = stats.f_blocks * stats.f_frsize;
    info.freeBytes = stats.f_bfree * stats.f_frsize;
    info.usedBytes = info.totalBytes - info.freeBytes;
    
    if (info.totalBytes > 0) {
        info.usedPercent = info.usedBytes * 100.0 / info.totalBytes;
    }
    
    // Get device name
    std::ifstream mounts("/proc/mounts");
    if (mounts.is_open()) {
        std::string line;
        while (std::getline(mounts, line)) {
            std::istringstream iss(line);
            std::string dev, mount, fs;
            iss >> dev >> mount >> fs;
            if (mount == path) {
                info.device = dev;
                info.filesystem = fs;
                break;
            }
        }
        mounts.close();
    }
    
    return info;
}

std::vector<SystemUtils::DiskInfo> SystemUtils::getAllDiskInfo() {
    std::vector<DiskInfo> disks;
    
    std::ifstream mounts("/proc/mounts");
    if (!mounts.is_open()) return disks;
    
    std::string line;
    while (std::getline(mounts, line)) {
        std::istringstream iss(line);
        std::string dev, mount, fs;
        iss >> dev >> mount >> fs;
        
        // Skip non-physical devices
        if (dev.find("/dev/") != 0) continue;
        if (fs == "tmpfs" || fs == "devtmpfs") continue;
        if (fs == "proc" || fs == "sysfs") continue;
        
        DiskInfo info = getDiskInfo(mount);
        if (info.totalBytes > 0) {
            disks.push_back(info);
        }
    }
    mounts.close();
    
    return disks;
}

uint64_t SystemUtils::getUptime() {
    std::ifstream uptime("/proc/uptime");
    if (!uptime.is_open()) return 0;
    
    double uptimeSeconds;
    uptime >> uptimeSeconds;
    uptime.close();
    
    return static_cast<uint64_t>(uptimeSeconds);
}

std::chrono::system_clock::time_point SystemUtils::getBootTime() {
    uint64_t uptime = getUptime();
    auto now = std::chrono::system_clock::now();
    auto bootTime = now - std::chrono::seconds(uptime);
    return bootTime;
}

int SystemUtils::getPID() {
    return getpid();
}

int SystemUtils::getPPID() {
    return getppid();
}

bool SystemUtils::isProcessRunning(int pid) {
    return kill(pid, 0) == 0;
}

bool SystemUtils::killProcess(int pid, int signal) {
    return kill(pid, signal) == 0;
}

bool SystemUtils::waitForProcess(int pid, int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    while (isProcessRunning(pid)) {
        if (timeoutMs > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);
            if (elapsed.count() > timeoutMs) {
                return false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

int SystemUtils::executeCommand(const std::string& command, std::string& output,
                               std::string& errorOutput, int timeoutMs) {
    std::string cmd = command + " 2>&1";
    std::array<char, 128> buffer;
    std::string result;
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return -1;
    
    auto start = std::chrono::steady_clock::now();
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
        
        if (timeoutMs > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);
            if (elapsed.count() > timeoutMs) {
                pclose(pipe);
                output = result;
                return -1;
            }
        }
    }
    
    int status = pclose(pipe);
    output = result;
    return WEXITSTATUS(status);
}

std::string SystemUtils::getUserName() {
    struct passwd* pwd = getpwuid(getuid());
    return pwd ? pwd->pw_name : "";
}

std::string SystemUtils::getGroupName() {
    struct group* grp = getgrgid(getgid());
    return grp ? grp->gr_name : "";
}

int SystemUtils::getUserID() {
    return getuid();
}

int SystemUtils::getGroupID() {
    return getgid();
}

int SystemUtils::daemonize(bool chdir, bool closeAll) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) return pid; // Parent
    
    // Child
    if (setsid() < 0) return -1;
    
    pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) return pid; // Parent of second child
    
    // Daemon
    if (chdir) {
        if (chdir("/") < 0) return -1;
    }
    
    if (closeAll) {
        // Close all file descriptors
        for (int fd = 0; fd < sysconf(_SC_OPEN_MAX); ++fd) {
            close(fd);
        }
    }
    
    // Open standard file descriptors
    open("/dev/null", O_RDWR);  // stdin
    open("/dev/null", O_RDWR);  // stdout
    open("/dev/null", O_RDWR);  // stderr
    
    return 0;
}

std::string SystemUtils::getEnv(const std::string& name) {
    const char* value = getenv(name.c_str());
    return value ? std::string(value) : "";
}

bool SystemUtils::setEnv(const std::string& name, const std::string& value, bool overwrite) {
    return setenv(name.c_str(), value.c_str(), overwrite ? 1 : 0) == 0;
}

std::map<std::string, std::string> SystemUtils::getEnvVars() {
    std::map<std::string, std::string> env;
    extern char** environ;
    
    for (char** envp = environ; *envp; ++envp) {
        std::string entry = *envp;
        size_t pos = entry.find('=');
        if (pos != std::string::npos) {
            env[entry.substr(0, pos)] = entry.substr(pos + 1);
        }
    }
    
    return env;
}

double SystemUtils::getSystemTemperature() {
    std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
    if (tempFile.is_open()) {
        int temp;
        tempFile >> temp;
        tempFile.close();
        return temp / 1000.0;
    }
    return -1.0;
}

double SystemUtils::getCPUTemperature(int core) {
    std::string path = "/sys/class/thermal/thermal_zone" + 
                       std::to_string(core >= 0 ? core : 0) + "/temp";
    std::ifstream tempFile(path);
    if (tempFile.is_open()) {
        int temp;
        tempFile >> temp;
        tempFile.close();
        return temp / 1000.0;
    }
    return -1.0;
}

double SystemUtils::getGPUTemperature() {
    // Try common GPU temperature paths
    std::vector<std::string> paths = {
        "/sys/class/drm/card0/device/hwmon/hwmon*/temp1_input",
        "/sys/class/drm/card0/device/temp1_input",
        "/sys/class/drm/card1/device/temp1_input"
    };
    
    for (const auto& path : paths) {
        std::ifstream tempFile(path);
        if (tempFile.is_open()) {
            int temp;
            tempFile >> temp;
            tempFile.close();
            return temp / 1000.0;
        }
    }
    
    return -1.0;
}

bool SystemUtils::isInDocker() {
    std::ifstream proc("/proc/1/cgroup");
    if (proc.is_open()) {
        std::string line;
        while (std::getline(proc, line)) {
            if (line.find("docker") != std::string::npos) {
                proc.close();
                return true;
            }
        }
        proc.close();
    }
    return false;
}

bool SystemUtils::isInVM() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string content;
        std::string line;
        while (std::getline(cpuinfo, line)) {
            content += line + "\n";
        }
        cpuinfo.close();
        
        // Check for hypervisor flags
        if (content.find("hypervisor") != std::string::npos) {
            return true;
        }
        
        // Check for specific vendor strings
        std::vector<std::string> vmVendors = {
            "VMware", "VirtualBox", "KVM", "Xen", "QEMU", "Microsoft"
        };
        
        for (const auto& vendor : vmVendors) {
            if (content.find(vendor) != std::string::npos) {
                return true;
            }
        }
    }
    
    // Check DMI data
    std::ifstream sysVendor("/sys/devices/virtual/dmi/id/sys_vendor");
    if (sysVendor.is_open()) {
        std::string vendor;
        std::getline(sysVendor, vendor);
        sysVendor.close();
        
        std::vector<std::string> vmVendors = {
            "VMware", "VirtualBox", "QEMU", "Xen", "KVM"
        };
        for (const auto& vmVendor : vmVendors) {
            if (vendor.find(vmVendor) != std::string::npos) {
                return true;
            }
        }
    }
    
    return false;
}

std::string SystemUtils::getSystemSerialNumber() {
    std::vector<std::string> paths = {
        "/sys/devices/virtual/dmi/id/product_serial",
        "/sys/firmware/devicetree/base/serial-number",
        "/proc/device-tree/serial-number"
    };
    
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::string serial;
            std::getline(file, serial);
            file.close();
            if (!serial.empty()) {
                return serial;
            }
        }
    }
    
    return "";
}

std::string SystemUtils::getSystemManufacturer() {
    std::vector<std::string> paths = {
        "/sys/devices/virtual/dmi/id/sys_vendor",
        "/sys/firmware/devicetree/base/manufacturer"
    };
    
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::string vendor;
            std::getline(file, vendor);
            file.close();
            if (!vendor.empty()) {
                return vendor;
            }
        }
    }
    
    return "";
}

std::string SystemUtils::getSystemModel() {
    std::vector<std::string> paths = {
        "/sys/devices/virtual/dmi/id/product_name",
        "/sys/firmware/devicetree/base/model"
    };
    
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::string model;
            std::getline(file, model);
            file.close();
            if (!model.empty()) {
                return model;
            }
        }
    }
    
    return "";
}
