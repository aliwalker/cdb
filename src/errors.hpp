#ifndef ERRORS_HPP
#define ERRORS_HPP

#include <string>
#include <stdexcept>

#define __RECORD_THROW(msg)                                                             \
    {                                                                                   \
        throw cdb::_log_error((msg), __FILE__, __LINE__);                               \
    }

#define __CONF_THROW(msg)                                                               \
    {                                                                                   \
        throw cdb::_config_error((msg), __FILE__, __LINE__);                            \
    }

#define __PARSER_THROW_SYNTAX(msg)                                                      \
    {                                                                                   \
        throw cdb::_parse_syntax_error((msg), __FILE__, __LINE__);                      \
    }

#define __PARSER_THROW_INCOMPLETE(msg)                                                  \
    {                                                                                   \
        throw cdb::_parse_incomplete_error((msg), __FILE__, __LINE__);                  \
    }

#define __SERVER_THROW(msg)                                                             \
    {                                                                                   \
        throw cdb::_server_error((msg), __FILE__, __LINE__);                            \
    }

#define DECLARE_RUNTIME_ERROR(name)                                                     \
    class _##name : public std::runtime_error                                           \
    {                                                                                   \
    public:                                                                             \
        _##name(const std::string &msg, const std::string &file, std::size_t line);     \
        const std::string &file() const;                                                \
        std::size_t line() const;                                                       \
    private:                                                                            \
        std::string file_;                                                              \
        std::size_t line_;                                                              \
    }

#define DEFINE_RUNTIME_ERROR(name)                                                      \
    _##name::_##name(const std::string &msg, const std::string &file, std::size_t line) \
        : std::runtime_error(msg)                                                       \
        , file_(file)                                                                   \
        , line_(line) {}                                                                \
    const std::string& _##name::file() const { return file_; }                          \
    std::size_t _##name::line() const { return line_; }

namespace cdb {

DECLARE_RUNTIME_ERROR(config_error);
DECLARE_RUNTIME_ERROR(parse_incomplete_error);
DECLARE_RUNTIME_ERROR(parse_syntax_error);
DECLARE_RUNTIME_ERROR(server_error);
DECLARE_RUNTIME_ERROR(log_error);

} // namespace cdb


#endif