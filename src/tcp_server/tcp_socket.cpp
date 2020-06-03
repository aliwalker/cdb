#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include "exceptions.hpp"
#include "tcp_socket.hpp"

namespace tcp_server_lib
{

tcp_socket::tcp_socket()
    : fd_(-1)
    , host_("")
    , port_(0)
    , type_(type::UNKNOWN) {}

tcp_socket::tcp_socket(int fd, const std::string &host, std::uint32_t port, type t)
    : fd_(fd)
    , host_(host)
    , port_(port)
    , type_(t) {}

tcp_socket& tcp_socket::operator=(tcp_socket&& sock)
{
    if (&sock != this)
    {
        this->fd_ = sock.fd_;
        this->host_ = sock.host_;
        this->port_ = sock.port_;
        this->type_ = sock.type_;
        sock.fd_ = -1;
        sock.type_ = type::UNKNOWN;
    }
    return *this;
}

tcp_socket::tcp_socket(tcp_socket &&sock)
    : fd_(sock.fd_)
    , host_(sock.host_)
    , port_(sock.port_)
    , type_(sock.type_)
{
    sock.fd_ = -1;
    sock.type_ = type::UNKNOWN;
}

/*
CLIENT operations
*/

std::vector<char> tcp_socket::recv(std::size_t size)
{
    ensure_fd();
    ensure_type(type::CLIENT);

    std::vector<char> data(size, 0);
    char *data_ptr = data.data();
    ssize_t data_len = ::recv(fd_, data_ptr, size, 0);
    if (data_len < 0)
        // Throw to close the connection.
        __TCP_THROW("error recv()");

    if (data_len == 0)
        // Throw to close the connection.
        __TCP_THROW("nothing to read");

    /* Do not use the following code because it blocks. */

    // std::size_t bytes_left = size;
    // while (bytes_left)
    // {
    //     int n;
    //     if ((n = ::recv(fd_, data_ptr, bytes_left, 0)) < 0)
    //     {
    //         if (errno == EINTR)
    //             break;
    //         else
    //             __TCP_THROW("error recv()");
    //     }
    //     else if (n == 0)
    //         break;

    //     data_ptr += n;
    //     data_len += n;
    //     bytes_left -= n;
    // }

    // if (data_len == 0)
    //     __TCP_THROW("recv() 0 bytes");

    data.resize(data_len);
    return data;
}

void tcp_socket::send(const std::vector<char> &data)
{
    ensure_fd();
    ensure_type(type::CLIENT);

    const char *data_ptr = data.data();
    std::size_t bytes_left = data.size();
    while (bytes_left)
    {
#ifdef __linux__
        // Prevent SIGPIPE from terminating the process.
        int n = ::send(fd_, data_ptr, bytes_left, MSG_NOSIGNAL);
#else
        int n = ::send(fd_, data_ptr, bytes_left, 0);
#endif
        if (n < 0)
        {
            if (errno == EINTR)
                n = 0;
            else
                __TCP_THROW("error send()");
        }

        data_ptr += n;
        bytes_left -= n;
    }
}

void tcp_socket::connect(const std::string &host, std::uint32_t port)
{
    host_ = host;
    port_ = port;

    ensure_fd();
    ensure_type(type::CLIENT);

    // Resolve [host:port].
    struct sockaddr_in server_addr;
    struct addrinfo hints;
    struct addrinfo *result;

    std::memset(&hints, 0, sizeof(hints));
    std::memset(&server_addr, 0, sizeof(server_addr));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0)
        __TCP_THROW("error getaddrinfo()");

    server_addr.sin_addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr)->sin_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    freeaddrinfo(result);

    if (::connect(fd_, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        close();
        perror("error connect");
        __TCP_THROW("error connect()");
    }
}

/*
SERVER operations
*/

void tcp_socket::bind(const std::string &host, std::uint32_t port)
{
    host_ = host;
    port_ = port;

    ensure_fd();
    ensure_type(type::SERVER);

    struct sockaddr_in server_addr;
    struct addrinfo *result;
    if (getaddrinfo(host.c_str(), nullptr, nullptr, &result) != 0)
        __TCP_THROW("error getaddrinfo()");

    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr)->sin_addr;
    server_addr.sin_port = htons(port);

    if (::bind(fd_, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1)
        __TCP_THROW("error bind()");
}

void tcp_socket::listen(std::size_t backlog)
{
    ensure_fd();
    ensure_type(type::SERVER);

    if (::listen(fd_, backlog) == -1)
        __TCP_THROW("error listen()");
}

tcp_socket tcp_socket::accept()
{
    ensure_fd();
    ensure_type(type::SERVER);

    struct sockaddr_in client_info;
    socklen_t client_info_len = sizeof(client_info);
    int client_fd = ::accept(fd_, reinterpret_cast<struct sockaddr*>(&client_info), &client_info_len);
    if (client_fd == -1)
        __TCP_THROW("error accept()");

    return {client_fd, inet_ntoa(client_info.sin_addr), client_info.sin_port, type::CLIENT};
}

void tcp_socket::close()
{
    if (fd_ != -1)
    {
        ::close(fd_);
    }

    fd_ = -1;
    type_ = type::UNKNOWN;
}

void tcp_socket::ensure_fd()
{
    if (fd_ != -1)
        return;

    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    type_ = type::UNKNOWN;

    int optval = 1;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

#ifdef __APPLE__
    // Prevent SIGPIPE from terminating the process.
    ::setsockopt(fd_, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval));
#endif
    if (fd_ == -1)
        __TCP_THROW("error socket()");
}

void tcp_socket::ensure_type(tcp_socket::type t)
{
    if (type_ == type::UNKNOWN)
    {
        type_ = t;
    }

    if (type_ != t)
        __TCP_THROW("unmatch socket type for performing operations");
}

bool tcp_socket::operator==(const tcp_socket &rhs) const
{
    return fd_ == rhs.fd_ && type_ == rhs.type_;
}

bool tcp_socket::operator!=(const tcp_socket &rhs) const
{
    return !operator==(rhs);
}

} // namespace tcp_server_lib
