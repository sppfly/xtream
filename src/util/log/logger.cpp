#include "logger.h"

#include <cstdio>
#include <mutex>

namespace xtream {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::set_level(LogLevel level) {
    instance().level_.store(level);
}

LogLevel Logger::level() {
    return instance().level_.load();
}

void Logger::trace(std::string_view msg, std::source_location loc) {
    log(LogLevel::Trace, msg, loc);
}
void Logger::debug(std::string_view msg, std::source_location loc) {
    log(LogLevel::Debug, msg, loc);
}
void Logger::info(std::string_view msg, std::source_location loc) {
    log(LogLevel::Info, msg, loc);
}
void Logger::warn(std::string_view msg, std::source_location loc) {
    log(LogLevel::Warn, msg, loc);
}
void Logger::error(std::string_view msg, std::source_location loc) {
    log(LogLevel::Error, msg, loc);
}

void Logger::log(LogLevel lvl, std::string_view msg, std::source_location loc) {
    if (lvl < level_.load()) return;

    const char* level_str = "UNKNOWN";
    switch (lvl) {
        case LogLevel::Trace:
            level_str = "TRACE";
            break;
        case LogLevel::Debug:
            level_str = "DEBUG";
            break;
        case LogLevel::Info:
            level_str = "INFO ";
            break;
        case LogLevel::Warn:
            level_str = "WARN ";
            break;
        case LogLevel::Error:
            level_str = "ERROR";
            break;
    }

    std::lock_guard lock(mutex_);
    std::fprintf(stderr, "[%s] [%s:%d] %.*s\n", level_str, loc.file_name(), loc.line(),
                 static_cast<int>(msg.size()), msg.data());
}

}  // namespace xtream
