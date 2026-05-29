#pragma once

#include <atomic>
#include <mutex>
#include <source_location>
#include <string_view>

#include "core/types/types.h"

namespace extream {

enum class LogLevel { Trace = 0, Debug = 1, Info = 2, Warn = 3, Error = 4 };

class Logger {
public:
    static Logger& instance();

    static void set_level(LogLevel level);
    static LogLevel level();

    void log(LogLevel lvl, std::string_view msg,
             std::source_location loc = std::source_location::current());

    void trace(std::string_view msg, std::source_location loc = std::source_location::current());
    void debug(std::string_view msg, std::source_location loc = std::source_location::current());
    void info(std::string_view msg, std::source_location loc = std::source_location::current());
    void warn(std::string_view msg, std::source_location loc = std::source_location::current());
    void error(std::string_view msg, std::source_location loc = std::source_location::current());

private:
    std::atomic<LogLevel> level_{LogLevel::Info};
    std::mutex mutex_;
};

}  // namespace extream
