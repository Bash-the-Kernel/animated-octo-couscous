#pragma once

#include <iostream>
#include <sstream>
#include <string_view>

namespace hft {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    
    void set_level(LogLevel level) { level_ = level; }
    
    template<typename... Args>
    void log(LogLevel level, Args&&... args) {
        if (level < level_) return;
        std::ostringstream oss;
        oss << "[" << level_str(level) << "] ";
        (oss << ... << std::forward<Args>(args));
        std::cout << oss.str() << std::endl;
    }
    
private:
    LogLevel level_{LogLevel::INFO};
    
    static constexpr std::string_view level_str(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }
};

#define LOG_DEBUG(...) hft::Logger::instance().log(hft::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) hft::Logger::instance().log(hft::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARN(...) hft::Logger::instance().log(hft::LogLevel::WARN, __VA_ARGS__)
#define LOG_ERROR(...) hft::Logger::instance().log(hft::LogLevel::ERROR, __VA_ARGS__)

} // namespace hft
