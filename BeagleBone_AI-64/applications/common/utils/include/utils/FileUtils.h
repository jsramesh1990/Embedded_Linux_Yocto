#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>
#include <fstream>
#include <functional>
#include <chrono>
#include <cstdint>

namespace common {
namespace utils {

/**
 * @brief File system utilities
 */
class FileUtils {
public:
    /**
     * @brief File permissions
     */
    enum class Permissions {
        OWNER_READ = 0400,
        OWNER_WRITE = 0200,
        OWNER_EXEC = 0100,
        GROUP_READ = 0040,
        GROUP_WRITE = 0020,
        GROUP_EXEC = 0010,
        OTHER_READ = 0004,
        OTHER_WRITE = 0002,
        OTHER_EXEC = 0001,
        OWNER_ALL = 0700,
        GROUP_ALL = 0070,
        OTHER_ALL = 0007,
        ALL = 0777
    };
    
    /**
     * @brief File information
     */
    struct FileInfo {
        std::string path;
        std::string name;
        std::string extension;
        size_t size;
        bool isDirectory;
        bool isFile;
        bool isSymlink;
        std::chrono::system_clock::time_point created;
        std::chrono::system_clock::time_point modified;
        std::chrono::system_clock::time_point accessed;
        uint32_t permissions;
    };
    
    /**
     * @brief Check if file exists
     */
    static bool exists(const std::string& path);
    
    /**
     * @brief Check if path is a directory
     */
    static bool isDirectory(const std::string& path);
    
    /**
     * @brief Check if path is a file
     */
    static bool isFile(const std::string& path);
    
    /**
     * @brief Check if path is a symbolic link
     */
    static bool isSymlink(const std::string& path);
    
    /**
     * @brief Get file size in bytes
     */
    static size_t getSize(const std::string& path);
    
    /**
     * @brief Get file modification time
     */
    static std::chrono::system_clock::time_point getModificationTime(const std::string& path);
    
    /**
     * @brief Get file creation time
     */
    static std::chrono::system_clock::time_point getCreationTime(const std::string& path);
    
    /**
     * @brief Get file access time
     */
    static std::chrono::system_clock::time_point getAccessTime(const std::string& path);
    
    /**
     * @brief Get file permissions
     */
    static uint32_t getPermissions(const std::string& path);
    
    /**
     * @brief Set file permissions
     */
    static bool setPermissions(const std::string& path, uint32_t permissions);
    
    /**
     * @brief Get file information
     */
    static FileInfo getFileInfo(const std::string& path);
    
    /**
     * @brief Read entire file as string
     */
    static std::string readToString(const std::string& path);
    
    /**
     * @brief Read entire file as binary
     */
    static std::vector<uint8_t> readToBinary(const std::string& path);
    
    /**
     * @brief Write string to file
     */
    static bool writeString(const std::string& path, const std::string& content);
    
    /**
     * @brief Write binary data to file
     */
    static bool writeBinary(const std::string& path, const std::vector<uint8_t>& data);
    
    /**
     * @brief Append string to file
     */
    static bool appendString(const std::string& path, const std::string& content);
    
    /**
     * @brief Copy file
     */
    static bool copy(const std::string& source, const std::string& destination);
    
    /**
     * @brief Move/rename file
     */
    static bool move(const std::string& source, const std::string& destination);
    
    /**
     * @brief Delete file
     */
    static bool remove(const std::string& path);
    
    /**
     * @brief Create directory (recursive)
     */
    static bool createDirectory(const std::string& path, uint32_t permissions = 0755);
    
    /**
     * @brief Delete directory (recursive)
     */
    static bool removeDirectory(const std::string& path);
    
    /**
     * @brief List files in directory
     * @param path Directory path
     * @param pattern File pattern (regex)
     * @param recursive Include subdirectories
     * @return Vector of file paths
     */
    static std::vector<std::string> listFiles(const std::string& path,
                                             const std::string& pattern = "*",
                                             bool recursive = false);
    
    /**
     * @brief List directories in path
     */
    static std::vector<std::string> listDirectories(const std::string& path,
                                                   bool recursive = false);
    
    /**
     * @brief Get base name (filename without extension)
     */
    static std::string getBaseName(const std::string& path);
    
    /**
     * @brief Get file name with extension
     */
    static std::string getFileName(const std::string& path);
    
    /**
     * @brief Get file extension
     */
    static std::string getExtension(const std::string& path);
    
    /**
     * @brief Get directory path
     */
    static std::string getDirectory(const std::string& path);
    
    /**
     * @brief Get absolute path
     */
    static std::string getAbsolutePath(const std::string& path);
    
    /**
     * @brief Get canonical path (resolves symlinks)
     */
    static std::string getCanonicalPath(const std::string& path);
    
    /**
     * @brief Get current working directory
     */
    static std::string getCurrentDirectory();
    
    /**
     * @brief Change current working directory
     */
    static bool setCurrentDirectory(const std::string& path);
    
    /**
     * @brief Get temporary directory path
     */
    static std::string getTempDirectory();
    
    /**
     * @brief Create temporary file
     * @param prefix File prefix
     * @param suffix File suffix
     * @param directory Directory to create file in
     * @return Path to temporary file
     */
    static std::string createTempFile(const std::string& prefix = "tmp",
                                     const std::string& suffix = "",
                                     const std::string& directory = "");
    
    /**
     * @brief Create temporary directory
     */
    static std::string createTempDirectory(const std::string& prefix = "tmp",
                                          const std::string& directory = "");
    
    /**
     * @brief Watch file for changes
     * @param path File path
     * @param callback Function called on changes
     * @param interval Polling interval in milliseconds
     * @param stopFlag Stop watching when true
     */
    static void watchFile(const std::string& path,
                         std::function<void()> callback,
                         int interval = 1000,
                         std::atomic<bool>* stopFlag = nullptr);
    
    /**
     * @brief Lock file for exclusive access
     */
    static bool lockFile(const std::string& path, int timeoutMs = 30000);
    
    /**
     * @brief Unlock file
     */
    static bool unlockFile(const std::string& path);
    
    /**
     * @brief Get free disk space in bytes
     */
    static uint64_t getFreeSpace(const std::string& path);
    
    /**
     * @brief Get total disk space in bytes
     */
    static uint64_t getTotalSpace(const std::string& path);
    
    /**
     * @brief Get file MIME type
     */
    static std::string getMimeType(const std::string& path);
    
    /**
     * @brief Check if file is readable
     */
    static bool isReadable(const std::string& path);
    
    /**
     * @brief Check if file is writable
     */
    static bool isWritable(const std::string& path);
    
    /**
     * @brief Check if file is executable
     */
    static bool isExecutable(const std::string& path);

private:
    static const std::map<std::string, std::string> MIME_TYPES;
};

} // namespace utils
} // namespace common

#endif // FILE_UTILS_H
