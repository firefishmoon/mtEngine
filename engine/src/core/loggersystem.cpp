#include <format>
#include <chrono>
#include "loggersystem.h"

template<> MT_API mtLoggerSystem* Singleton<mtLoggerSystem>::_instance = nullptr;

b8 mtLoggerSystem::initialize() {
    // Initialize logger system (e.g., open log files, set up log levels)
    return true;
}

b8 mtLoggerSystem::shutdown() {
    // Shutdown logger system (e.g., close log files, clean up resources)
    return true;
}

std::string mtLoggerSystem::getTimestamp() {
    // Get current timestamp as a string
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    // 毫秒部分
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    // 本地时间（线程安全）
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec,
        static_cast<int>(ms.count()));
}
