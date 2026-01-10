#pragma once

#include "../defines.h"
#include "../common/singleton.h"
#include <format>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <string.h>

enum class LogLevel {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

static const char* level_str[] = {
    "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL"
};

class MT_API mtLoggerSystem : public Singleton<mtLoggerSystem> {
public:
    mtLoggerSystem() = default;
    ~mtLoggerSystem() = default;

    b8 initialize() override;
    b8 shutdown() override;

#if __GNUC__ > 15
    template<typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) { // Use std::format_string for compile-time checked format strings
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        // Log the message with the specified log level
        std::cout << "[" << level_str[static_cast<int>(level)] << "] " << message << std::endl;
    }
#else
    void log(LogLevel level, const char* fmt, ...) {
        char msgbuf[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
        va_end(ap);
        std::cout << "[" << level_str[static_cast<int>(level)] << "] " << msgbuf << std::endl;
    }
#endif
       
};
