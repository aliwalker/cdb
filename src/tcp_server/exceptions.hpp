#ifndef TCP_SERVER_EXCEPTIONS_HPP
#define TCP_SERVER_EXCEPTIONS_HPP

#include <string>
#include <stdexcept>

#define __TCP_THROW(msg)                                                    \
    {                                                                   \
        throw tcp_server_lib::tcp_exception((msg), __FILE__, __LINE__);     \
    }

namespace tcp_server_lib
{

/// TCP specific exception.
class tcp_exception : public std::runtime_error
{
public:
    tcp_exception(const std::string msg, const std::string &file, std::size_t line);

public:
    const std::string &file() const;
    std::size_t line() const;

private:
    std::string file_;
    std::size_t line_;
};

} // namespace tcp_server_lib


#endif