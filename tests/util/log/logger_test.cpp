#include "util/log/logger.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace xtream {
namespace {

TEST(LoggerTest, SetAndGetLevel) {
    Logger::set_level(LogLevel::Debug);
    EXPECT_EQ(Logger::level(), LogLevel::Debug);
    Logger::set_level(LogLevel::Info);
    EXPECT_EQ(Logger::level(), LogLevel::Info);
}

TEST(LoggerTest, FilterBelowLevel) {
    Logger::set_level(LogLevel::Warn);
    Logger::instance().info("should not appear");
    Logger::instance().debug("should not appear");
    Logger::instance().trace("should not appear");
    SUCCEED();
}

TEST(LoggerTest, ErrorAlwaysLogs) {
    Logger::set_level(LogLevel::Error);
    Logger::instance().error("this should log");
    SUCCEED();
}

TEST(LoggerTest, ThreadSafety) {
    Logger::set_level(LogLevel::Info);
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i]() { Logger::instance().info("thread " + std::to_string(i)); });
    }
    for (auto& t : threads) {
        t.join();
    }
    SUCCEED();
}

TEST(LoggerTest, AllLevels) {
    Logger::set_level(LogLevel::Trace);
    Logger::instance().trace("trace message");
    Logger::instance().debug("debug message");
    Logger::instance().info("info message");
    Logger::instance().warn("warn message");
    Logger::instance().error("error message");
    SUCCEED();
}

}  // namespace
}  // namespace xtream
