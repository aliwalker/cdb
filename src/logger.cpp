#include <ctime>
#include <iostream>
#include <iomanip>
#include "logger.hpp"

namespace cdb {

static const char red[] = {0x1b, '[', '1', ';', '3', '1', 'm', 0};
static const char yellow[] = {0x1b, '[', '1', ';', '3', '3', 'm', 0};
static const char blue[] = {0x1b, '[', '1', ';', '3', '4', 'm', 0};
static const char normal[] = {0x1b, '[', '0', ';', '3', '9', 'm', 0};

logger *default_logger()
{
    static logger default_logger_{logger::level::debug};
    return &default_logger_;
}

void logger::error(const std::string &msg, const std::string &file, std::size_t line)
{
    if (level_ >= level::error)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::cout << std::put_time(&tm, "%d/%m/%Y %H:%M:%S") 
                << "[" << red << "ERROR" << normal << "]"
                << "[" << file << ":" << line << "] "
                << msg << std::endl;
    }
}

void logger::warn(const std::string &msg, const std::string &file, std::size_t line)
{
    if (level_ >= level::warn)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::cout << std::put_time(&tm, "%d/%m/%Y %H:%M:%S")
                 << "[" << yellow << "WARN" << normal << "]"
                << "[" << file << ":" << line << "] "
                << msg << std::endl;
    }
}

void logger::info(const std::string &msg, const std::string &file, std::size_t line)
{
    if (level_ >= level::info)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::cout << std::put_time(&tm, "%d/%m/%Y %H:%M:%S")
                 << "[" << blue << "INFO" << normal << "]"
                << "[" << file << ":" << line << "] "
                << msg << std::endl;
    }
}

void logger::debug(const std::string &msg, const std::string &file, std::size_t line)
{
    if (level_ >= level::debug)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::cout << std::put_time(&tm, "%d/%m/%Y %H:%M:%S")
                << "[DEBUG]"
                << "[" << file << ":" << line << "] "
                << msg << std::endl;
    }
}

void error(const std::string &msg, const std::string &file, std::size_t line)
{
    if (default_logger())
        default_logger()->error(msg, file, line);
}

void warn(const std::string &msg, const std::string &file, std::size_t line)
{
    if (default_logger())
        default_logger()->warn(msg, file, line);
}

void info(const std::string &msg, const std::string &file, std::size_t line)
{
    if (default_logger())
        default_logger()->info(msg, file, line);
}

void debug(const std::string &msg, const std::string &file, std::size_t line)
{
    if (default_logger())
        default_logger()->debug(msg, file, line);
}

} // namespace cdb

