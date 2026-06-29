#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <functional>
#include <map>
#include <csignal>
#include <atomic>
#include <mutex>

namespace common {
namespace utils {

/**
 * @brief Signal handler for graceful shutdown and signal management
 * 
 * Provides a unified interface for handling POSIX signals
 * with support for multiple callbacks and clean shutdown
 */
class SignalHandler {
public:
    /**
     * @brief Signal type
     */
    enum class Signal {
        INT = SIGINT,
        TERM = SIGTERM,
        HUP = SIGHUP,
        QUIT = SIGQUIT,
        ABRT = SIGABRT,
        FPE = SIGFPE,
        ILL = SIGILL,
        SEGV = SIGSEGV,
        PIPE = SIGPIPE,
        ALRM = SIGALRM,
        USR1 = SIGUSR1,
        USR2 = SIGUSR2,
        CHLD = SIGCHLD,
        CONT = SIGCONT,
        STOP = SIGSTOP,
        TSTP = SIGTSTP,
        TTIN = SIGTTIN,
        TTOU = SIGTTOU,
        BUS = SIGBUS,
        POLL = SIGPOLL,
        PROF = SIGPROF,
        SYS = SIGSYS,
        TRAP = SIGTRAP,
        URG = SIGURG,
        VTALRM = SIGVTALRM,
        XCPU = SIGXCPU,
        XFSZ = SIGXFSZ
    };
    
    /**
     * @brief Callback type for signal handlers
     * @param signal Signal that was received
     * @param info Additional signal info
     */
    using SignalCallback = std::function<void(Signal signal, const siginfo_t* info)>;
    
    /**
     * @brief Get singleton instance
     */
    static SignalHandler& getInstance();
    
    /**
     * @brief Register signal handler
     * @param signal Signal to handle
     * @param callback Callback function
     * @param priority Handler priority (higher = called first)
     * @return true on success
     */
    bool registerHandler(Signal signal, SignalCallback callback, int priority = 0);
    
    /**
     * @brief Unregister signal handler
     * @param signal Signal to unregister
     * @param callback Callback to remove (if nullptr, remove all)
     */
    void unregisterHandler(Signal signal, SignalCallback callback = nullptr);
    
    /**
     * @brief Register signal with default behavior
     * @param signal Signal to set default
     */
    void setDefaultHandler(Signal signal);
    
    /**
     * @brief Ignore signal
     * @param signal Signal to ignore
     */
    void ignore(Signal signal);
    
    /**
     * @brief Block signal
     * @param signal Signal to block
     */
    void block(Signal signal);
    
    /**
     * @brief Unblock signal
     * @param signal Signal to unblock
     */
    void unblock(Signal signal);
    
    /**
     * @brief Check if signal is blocked
     */
    bool isBlocked(Signal signal) const;
    
    /**
     * @brief Wait for signal (blocking)
     * @param timeoutMs Timeout in milliseconds (0 = infinite)
     * @return Signal received, or Signal::INT if timeout
     */
    Signal waitForSignal(int timeoutMs = 0);
    
    /**
     * @brief Send signal to process
     * @param pid Process ID
     * @param signal Signal to send
     * @return true on success
     */
    bool sendSignal(int pid, Signal signal);
    
    /**
     * @brief Send signal to current process
     * @param signal Signal to send
     * @return true on success
     */
    bool raiseSignal(Signal signal);
    
    /**
     * @brief Check if shutdown was requested (SIGTERM/SIGINT)
     */
    bool shutdownRequested() const { return shutdownRequested_; }
    
    /**
     * @brief Reset shutdown flag
     */
    void resetShutdownFlag() { shutdownRequested_ = false; }
    
    /**
     * @brief Get last received signal
     */
    Signal getLastSignal() const { return lastSignal_; }
    
    /**
     * @brief Get signal name as string
     */
    static std::string signalToString(Signal signal);
    
    /**
     * @brief Get signal description
     */
    static std::string signalDescription(Signal signal);

private:
    // Singleton
    SignalHandler();
    ~SignalHandler();
    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;
    
    // Signal handler structure
    struct HandlerInfo {
        SignalCallback callback;
        int priority;
        bool defaultHandler;
        
        HandlerInfo() : priority(0), defaultHandler(false) {}
        HandlerInfo(SignalCallback cb, int prio = 0) 
            : callback(cb), priority(prio), defaultHandler(false) {}
        HandlerInfo(bool defaultHandler) 
            : priority(0), defaultHandler(defaultHandler) {}
    };
    
    using HandlerMap = std::multimap<int, HandlerInfo>;
    
    // State
    std::map<int, HandlerMap> handlers_;
    std::map<int, bool> blockedSignals_;
    std::atomic<bool> shutdownRequested_{false};
    std::atomic<Signal> lastSignal_{Signal::INT};
    std::mutex mutex_;
    bool initialized_;
    std::map<int, struct sigaction> oldActions_;
    
    // Signal queue for waitForSignal
    std::queue<Signal> signalQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    
    // Static signal handler
    static void signalHandler(int sig, siginfo_t* info, void* context);
    
    // Internal methods
    void setupSignal(Signal signal);
    void restoreSignal(Signal signal);
    void dispatchSignal(Signal signal, const siginfo_t* info);
    bool installHandler(Signal signal);
    int getSignalNumber(Signal signal) const;
    Signal getSignalType(int sig) const;
};

} // namespace utils
} // namespace common

#endif // SIGNAL_HANDLER_H
