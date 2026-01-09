#pragma once

#include "../defines.h"
#include "../common/singleton.h"
#include <format>
#include <iostream>
#include <string>

enum class LogLevel {
    TRACE,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class MT_API mtLoggerSystem : public Singleton<mtLoggerSystem> {
public:
    mtLoggerSystem() = default;
    ~mtLoggerSystem() = default;

    b8 initialize() override;
    b8 shutdown() override;

    template<typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        // Use std::format_string for compile-time checked format strings
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        // Log the message with the specified log level
        const char* levelStr = "";
        switch (level) {
            case LogLevel::TRACE:   levelStr = "TRACE"; break;
            case LogLevel::INFO:    levelStr = "INFO"; break;
            case LogLevel::WARNING: levelStr = "WARNING"; break;
            case LogLevel::ERROR:   levelStr = "ERROR"; break;
            case LogLevel::FATAL:   levelStr = "FATAL"; break;
        }
        std::cout << "[" << levelStr << "] " << message << std::endl;
    }
};
