#pragma once

#include "../defines.h"
#include "../common/singleton.h"
#include <format>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <string.h>

// #define STD_FORMAT_SUPPORTED() (__has_include(<format>) && defined(__cpp_lib_format))

enum class LogLevel {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};


namespace Color {
    inline constexpr const char* Reset   = "\033[0m";
    inline constexpr const char* Black   = "\033[30m";
    inline constexpr const char* Red     = "\033[31m";
    inline constexpr const char* Green   = "\033[32m";
    inline constexpr const char* Yellow  = "\033[33m";
    inline constexpr const char* Blue    = "\033[34m";
    inline constexpr const char* Magenta = "\033[35m";
    inline constexpr const char* Cyan    = "\033[36m";
    inline constexpr const char* White   = "\033[37m";
    
    // 背景色
    inline constexpr const char* BgRed   = "\033[41m";
    inline constexpr const char* BgYellow= "\033[43m";
}

static const char* level_str[] = {
    "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL"
};

static const char* level_color[] = {
    Color::Cyan, Color::Blue, Color::Green, Color::Yellow, Color::Red, Color::BgRed
};

class MT_API mtLoggerSystem : public Singleton<mtLoggerSystem> {
public:
    mtLoggerSystem() = default;
    ~mtLoggerSystem() = default;

    b8 initialize() override;
    b8 shutdown() override;

    std::string getTimestamp();

// #if STD_FORMAT_SUPPORTED()
    template<typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) { // Use std::format_string for compile-time checked format strings
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        // Log the message with the specified log level
        std::cout << level_color[static_cast<int>(level)] << "[" << getTimestamp() << "]" << "[" << level_str[static_cast<int>(level)] << "] " << message << Color::Reset << std::endl;
    }
// #else
    // void log(LogLevel level, const char* fmt, ...) {
    //     char msgbuf[1024];
    //     va_list ap;
    //     va_start(ap, fmt);
    //     vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
    //     va_end(ap);
    //     std::cout << "[" << level_str[static_cast<int>(level)] << "] " << msgbuf << std::endl;
    // }
// #endif
       
};
