#include "exceptions.hpp"

namespace cdb_tcp_server
{

tcp_exception::tcp_exception(const std::string msg, const std::string &file, std::size_t line)
    : std::runtime_error(msg)
    , file_(file)
    , line_(line) {}

const std::string &tcp_exception::file() const
{
    return file_;
}

std::size_t tcp_exception::line() const
{
    return line_;
}

} // namespace cdb_tcp_server
