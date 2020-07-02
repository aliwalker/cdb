#ifndef CDB_LOGGER_HPP
#define CDB_LOGGER_HPP

#include <string>
#include <memory>
#include <mutex>

namespace cdb {

/// Base logger class.
class logger {
public:
    /// Logging level.
    enum class level {
        disabled = 0,
        error = 1,
        warn = 2,
        info = 3,
        debug = 4
    };

public:
    /// Ctor.
    logger(level l = level::info) : level_(l) {}

    void error(const std::string &msg, const std::string &file, std::size_t line);
    void warn(const std::string &msg, const std::string &file, std::size_t line);
    void info(const std::string &msg, const std::string &file, std::size_t line);
    void debug(const std::string &msg, const std::string &file, std::size_t line);

private:
    level level_;

    /// Prevent racing.
    std::mutex mutex_;
};

/// Global default logger.
logger* default_logger();

/// Logging functions that utilize [default_logger].
void error(const std::string &msg, const std::string &file, std::size_t line);
void warn(const std::string &msg, const std::string &file, std::size_t line);
void info(const std::string &msg, const std::string &file, std::size_t line);
void debug(const std::string &msg, const std::string &file, std::size_t line);

/// Helper macro to record line and file of the source.
#define __CDB_LOG(l, msg)   cdb::l(msg, __FILE__, __LINE__)

} // namespace cdb


#endif