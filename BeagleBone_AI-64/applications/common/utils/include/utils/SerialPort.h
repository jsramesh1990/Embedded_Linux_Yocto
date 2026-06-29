#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

namespace common {
namespace utils {

/**
 * @brief Serial port communication class
 * 
 * Provides cross-platform serial port communication
 * Supports baud rates, parity, flow control, and async reads
 */
class SerialPort {
public:
    /**
     * @brief Baud rates
     */
    enum class BaudRate {
        BAUD_50 = 50,
        BAUD_75 = 75,
        BAUD_110 = 110,
        BAUD_134 = 134,
        BAUD_150 = 150,
        BAUD_200 = 200,
        BAUD_300 = 300,
        BAUD_600 = 600,
        BAUD_1200 = 1200,
        BAUD_1800 = 1800,
        BAUD_2400 = 2400,
        BAUD_4800 = 4800,
        BAUD_9600 = 9600,
        BAUD_19200 = 19200,
        BAUD_38400 = 38400,
        BAUD_57600 = 57600,
        BAUD_115200 = 115200,
        BAUD_230400 = 230400,
        BAUD_460800 = 460800,
        BAUD_921600 = 921600
    };
    
    /**
     * @brief Data bits
     */
    enum class DataBits {
        FIVE = 5,
        SIX = 6,
        SEVEN = 7,
        EIGHT = 8
    };
    
    /**
     * @brief Parity
     */
    enum class Parity {
        NONE,
        ODD,
        EVEN,
        MARK,
        SPACE
    };
    
    /**
     * @brief Stop bits
     */
    enum class StopBits {
        ONE = 1,
        ONE_POINT_FIVE = 15,
        TWO = 2
    };
    
    /**
     * @brief Flow control
     */
    enum class FlowControl {
        NONE,
        HARDWARE,
        SOFTWARE
    };
    
    /**
     * @brief Callback type for data received
     */
    using DataCallback = std::function<void(const std::vector<uint8_t>& data)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    /**
     * @brief Constructor
     */
    SerialPort();
    
    /**
     * @brief Destructor
     */
    ~SerialPort();
    
    /**
     * @brief Open serial port
     * @param port Port name (e.g., "/dev/ttyS0", "COM1")
     * @param baudRate Baud rate
     * @param dataBits Data bits
     * @param parity Parity
     * @param stopBits Stop bits
     * @param flowControl Flow control
     * @return true if opened successfully
     */
    bool open(const std::string& port,
              BaudRate baudRate = BaudRate::BAUD_9600,
              DataBits dataBits = DataBits::EIGHT,
              Parity parity = Parity::NONE,
              StopBits stopBits = StopBits::ONE,
              FlowControl flowControl = FlowControl::NONE);
    
    /**
     * @brief Close serial port
     */
    void close();
    
    /**
     * @brief Check if port is open
     */
    bool isOpen() const { return isOpen_; }
    
    /**
     * @brief Write data to port
     * @param data Data to write
     * @param timeoutMs Timeout in milliseconds
     * @return Number of bytes written
     */
    size_t write(const std::vector<uint8_t>& data, int timeoutMs = 5000);
    
    /**
     * @brief Write string to port
     * @param str String to write
     * @param timeoutMs Timeout in milliseconds
     * @return Number of bytes written
     */
    size_t write(const std::string& str, int timeoutMs = 5000);
    
    /**
     * @brief Read data from port
     * @param buffer Buffer to read into
     * @param count Number of bytes to read
     * @param timeoutMs Timeout in milliseconds
     * @return Number of bytes read
     */
    size_t read(std::vector<uint8_t>& buffer, size_t count, int timeoutMs = 5000);
    
    /**
     * @brief Read data until delimiter
     * @param buffer Buffer to read into
     * @param delimiter Delimiter byte
     * @param timeoutMs Timeout in milliseconds
     * @return Number of bytes read
     */
    size_t readUntil(std::vector<uint8_t>& buffer, uint8_t delimiter, int timeoutMs = 5000);
    
    /**
     * @brief Read line (until newline)
     * @param line String to read into
     * @param timeoutMs Timeout in milliseconds
     * @return true if line read successfully
     */
    bool readLine(std::string& line, int timeoutMs = 5000);
    
    /**
     * @brief Set data callback for asynchronous reading
     * @param callback Called when data is received
     */
    void setDataCallback(DataCallback callback);
    
    /**
     * @brief Set error callback
     * @param callback Called on errors
     */
    void setErrorCallback(ErrorCallback callback);
    
    /**
     * @brief Start asynchronous reading
     */
    void startAsyncReading();
    
    /**
     * @brief Stop asynchronous reading
     */
    void stopAsyncReading();
    
    /**
     * @brief Set read timeout
     */
    void setReadTimeout(int timeoutMs) { readTimeoutMs = timeoutMs; }
    
    /**
     * @brief Get read timeout
     */
    int getReadTimeout() const { return readTimeoutMs; }
    
    /**
     * @brief Set write timeout
     */
    void setWriteTimeout(int timeoutMs) { writeTimeoutMs = timeoutMs; }
    
    /**
     * @brief Get write timeout
     */
    int getWriteTimeout() const { return writeTimeoutMs; }
    
    /**
     * @brief Clear buffers
     */
    void flush(bool input = true, bool output = true);
    
    /**
     * @brief Get available bytes
     */
    size_t bytesAvailable();
    
    /**
     * @brief Get port name
     */
    std::string getPort() const { return port; }
    
    /**
     * @brief Get baud rate
     */
    BaudRate getBaudRate() const { return baudRate; }
    
    /**
     * @brief Get data bits
     */
    DataBits getDataBits() const { return dataBits; }
    
    /**
     * @brief Get parity
     */
    Parity getParity() const { return parity; }
    
    /**
     * @brief Get stop bits
     */
    StopBits getStopBits() const { return stopBits; }
    
    /**
     * @brief Get flow control
     */
    FlowControl getFlowControl() const { return flowControl; }
    
    /**
     * @brief List available serial ports
     */
    static std::vector<std::string> listPorts();

private:
    // Port settings
    std::string port;
    BaudRate baudRate;
    DataBits dataBits;
    Parity parity;
    StopBits stopBits;
    FlowControl flowControl;
    std::atomic<bool> isOpen_;
    int readTimeoutMs = 5000;
    int writeTimeoutMs = 5000;
    
    // OS-specific file descriptor
    int fd;
    
    // Callbacks
    DataCallback dataCallback;
    ErrorCallback errorCallback;
    
    // Async reading
    std::atomic<bool> asyncReading;
    std::thread readThread;
    std::mutex mutex;
    
    // Internal methods
    bool configurePort();
    void readLoop();
    void notifyError(const std::string& error);
    void notifyData(const std::vector<uint8_t>& data);
    
    // OS-specific implementations
    bool openPort(const std::string& port, int baudRateValue);
    void closePort();
    size_t writePort(const uint8_t* data, size_t length);
    size_t readPort(uint8_t* buffer, size_t length);
    bool setBaudRate(int speed);
    bool setParity(int parity);
    bool setDataBits(int bits);
    bool setStopBits(int bits);
    bool setFlowControl(int flow);
    void flushPort(bool input, bool output);
    size_t bytesAvailablePort();
};

} // namespace utils
} // namespace common

#endif // SERIAL_PORT_H
