#include "utils/FileUtils.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <regex>
#include <chrono>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

using namespace common::utils;

const std::map<std::string, std::string> FileUtils::MIME_TYPES = {
    {".txt", "text/plain"},
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".pdf", "application/pdf"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".svg", "image/svg+xml"},
    {".mp3", "audio/mpeg"},
    {".wav", "audio/wav"},
    {".mp4", "video/mp4"},
    {".avi", "video/x-msvideo"},
    {".zip", "application/zip"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"},
    {".sh", "application/x-sh"},
    {".bin", "application/octet-stream"}
};

bool FileUtils::exists(const std::string& path) {
    return fs::exists(path);
}

bool FileUtils::isDirectory(const std::string& path) {
    return fs::is_directory(path);
}

bool FileUtils::isFile(const std::string& path) {
    return fs::is_regular_file(path);
}

bool FileUtils::isSymlink(const std::string& path) {
    return fs::is_symlink(path);
}

size_t FileUtils::getSize(const std::string& path) {
    try {
        return fs::file_size(path);
    } catch (...) {
        return 0;
    }
}

std::chrono::system_clock::time_point FileUtils::getModificationTime(const std::string& path) {
    try {
        auto ftime = fs::last_write_time(path);
        return fs::file_time_type::clock::to_sys(ftime);
    } catch (...) {
        return std::chrono::system_clock::time_point();
    }
}

std::chrono::system_clock::time_point FileUtils::getCreationTime(const std::string& path) {
    // Not all filesystems support creation time
    try {
        auto ftime = fs::last_write_time(path); // Fallback to modification time
        return fs::file_time_type::clock::to_sys(ftime);
    } catch (...) {
        return std::chrono::system_clock::time_point();
    }
}

std::chrono::system_clock::time_point FileUtils::getAccessTime(const std::string& path) {
    try {
        auto ftime = fs::last_write_time(path); // Fallback to modification time
        return fs::file_time_type::clock::to_sys(ftime);
    } catch (...) {
        return std::chrono::system_clock::time_point();
    }
}

uint32_t FileUtils::getPermissions(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return 0;
    }
    return st.st_mode & 0777;
}

bool FileUtils::setPermissions(const std::string& path, uint32_t permissions) {
    return chmod(path.c_str(), permissions) == 0;
}

FileUtils::FileInfo FileUtils::getFileInfo(const std::string& path) {
    FileInfo info;
    info.path = path;
    info.name = getFileName(path);
    info.extension = getExtension(path);
    info.size = getSize(path);
    info.isDirectory = isDirectory(path);
    info.isFile = isFile(path);
    info.isSymlink = isSymlink(path);
    info.created = getCreationTime(path);
    info.modified = getModificationTime(path);
    info.accessed = getAccessTime(path);
    info.permissions = getPermissions(path);
    return info;
}

std::string FileUtils::readToString(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::string content;
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    file.close();
    return content;
}

std::vector<uint8_t> FileUtils::readToBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    
    std::vector<uint8_t> data;
    file.seekg(0, std::ios::end);
    data.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), data.size());
    file.close();
    return data;
}

bool FileUtils::writeString(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.write(content.c_str(), content.size());
    file.close();
    return true;
}

bool FileUtils::writeBinary(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    return true;
}

bool FileUtils::appendString(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        return false;
    }
    file.write(content.c_str(), content.size());
    file.close();
    return true;
}

bool FileUtils::copy(const std::string& source, const std::string& destination) {
    try {
        fs::copy(source, destination, fs::copy_options::overwrite_existing);
        return true;
    } catch (...) {
        return false;
    }
}

bool FileUtils::move(const std::string& source, const std::string& destination) {
    try {
        fs::rename(source, destination);
        return true;
    } catch (...) {
        return false;
    }
}

bool FileUtils::remove(const std::string& path) {
    try {
        return fs::remove(path);
    } catch (...) {
        return false;
    }
}

bool FileUtils::createDirectory(const std::string& path, uint32_t permissions) {
    try {
        return fs::create_directories(path);
    } catch (...) {
        return false;
    }
}

bool FileUtils::removeDirectory(const std::string& path) {
    try {
        return fs::remove_all(path) > 0;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> FileUtils::listFiles(const std::string& path,
                                             const std::string& pattern,
                                             bool recursive) {
    std::vector<std::string> result;
    
    try {
        std::regex regex(pattern);
        auto iterator = recursive ? fs::recursive_directory_iterator(path) 
                                   : fs::directory_iterator(path);
        
        for (const auto& entry : iterator) {
            if (fs::is_regular_file(entry)) {
                std::string name = entry.path().filename().string();
                if (std::regex_match(name, regex)) {
                    result.push_back(entry.path().string());
                }
            }
        }
    } catch (...) {}
    
    return result;
}

std::vector<std::string> FileUtils::listDirectories(const std::string& path, bool recursive) {
    std::vector<std::string> result;
    
    try {
        auto iterator = recursive ? fs::recursive_directory_iterator(path) 
                                   : fs::directory_iterator(path);
        
        for (const auto& entry : iterator) {
            if (fs::is_directory(entry)) {
                result.push_back(entry.path().string());
            }
        }
    } catch (...) {}
    
    return result;
}

std::string FileUtils::getBaseName(const std::string& path) {
    std::string name = getFileName(path);
    size_t pos = name.find_last_of('.');
    if (pos != std::string::npos) {
        return name.substr(0, pos);
    }
    return name;
}

std::string FileUtils::getFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

std::string FileUtils::getExtension(const std::string& path) {
    std::string name = getFileName(path);
    size_t pos = name.find_last_of('.');
    if (pos != std::string::npos) {
        return name.substr(pos);
    }
    return "";
}

std::string FileUtils::getDirectory(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return ".";
}

std::string FileUtils::getAbsolutePath(const std::string& path) {
    try {
        return fs::absolute(path).string();
    } catch (...) {
        return path;
    }
}

std::string FileUtils::getCanonicalPath(const std::string& path) {
    try {
        return fs::canonical(path).string();
    } catch (...) {
        return path;
    }
}

std::string FileUtils::getCurrentDirectory() {
    try {
        return fs::current_path().string();
    } catch (...) {
        return "";
    }
}

bool FileUtils::setCurrentDirectory(const std::string& path) {
    try {
        fs::current_path(path);
        return true;
    } catch (...) {
        return false;
    }
}

std::string FileUtils::getTempDirectory() {
    const char* tmpdir = std::getenv("TMPDIR");
    if (tmpdir) return tmpdir;
    return "/tmp";
}

std::string FileUtils::createTempFile(const std::string& prefix,
                                     const std::string& suffix,
                                     const std::string& directory) {
    std::string dir = directory.empty() ? getTempDirectory() : directory;
    std::string template_str = dir + "/" + prefix + "XXXXXX" + suffix;
    
    std::vector<char> buffer(template_str.begin(), template_str.end());
    buffer.push_back('\0');
    
    int fd = mkstemp(buffer.data());
    if (fd < 0) {
        return "";
    }
    
    close(fd);
    return std::string(buffer.data());
}

std::string FileUtils::createTempDirectory(const std::string& prefix,
                                          const std::string& directory) {
    std::string dir = directory.empty() ? getTempDirectory() : directory;
    std::string template_str = dir + "/" + prefix + "XXXXXX";
    
    std::vector<char> buffer(template_str.begin(), template_str.end());
    buffer.push_back('\0');
    
    char* result = mkdtemp(buffer.data());
    if (!result) {
        return "";
    }
    
    return std::string(result);
}

std::string FileUtils::getMimeType(const std::string& path) {
    std::string ext = getExtension(path);
    auto it = MIME_TYPES.find(ext);
    if (it != MIME_TYPES.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

bool FileUtils::isReadable(const std::string& path) {
    return access(path.c_str(), R_OK) == 0;
}

bool FileUtils::isWritable(const std::string& path) {
    return access(path.c_str(), W_OK) == 0;
}

bool FileUtils::isExecutable(const std::string& path) {
    return access(path.c_str(), X_OK) == 0;
}

void FileUtils::watchFile(const std::string& path,
                         std::function<void()> callback,
                         int interval,
                         std::atomic<bool>* stopFlag) {
    auto lastModified = getModificationTime(path);
    
    while (!stopFlag || !stopFlag->load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        
        auto currentModified = getModificationTime(path);
        if (currentModified != lastModified) {
            lastModified = currentModified;
            if (callback) {
                callback();
            }
        }
    }
}
